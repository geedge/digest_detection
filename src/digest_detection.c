#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <net/if.h>
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#include "MESA_handle_logger.h"
#include "MESA_prof_load.h"
#include "MESA_list_queue.h"
#include "MESA_htable.h"

#include "digest_detection.h"
#include "cache_evbase_client.h"
#include "log.h"
#include "main.h"
#include "recv.h"
#include "my_socket.h"

extern dd_parameter_t 		g_dd_run;
extern dd_configure_t 		g_dd_cfg;
extern dd_status_t 			g_dd_stat;

void free_media(void* data)
{
	media_t* mdi = (media_t*)data;
	
	if(NULL!=mdi)
	{		
		expire_media_write_to_log(mdi, MEDIA_EXPIRE, NULL);			
		Maat_stream_scan_digest_end(&mdi->digest_stream_para);			
		free(mdi);
	}	
}

void renew_media(media_t* mdi)
{
	if(NULL!=mdi)
	{		
		create_media_write_to_log(mdi, MEDIA_RENEW, NULL);			
		mdi->hit_digest = 0;
	}	
}

long media_create_cb(void *data, const uint8_t *key, uint size, void *user_arg)
{	
	media_t* 			mdi = (media_t*)data;
	meta_t*				meta = (meta_t*)user_arg;
	char				pbuf[32] = {0};		
	int 				buf_len = 32;
		
	if(NULL==mdi)
	{		
		mdi = (media_t*)calloc(1,sizeof(media_t));
		if(0>(MESA_htable_add(g_dd_run.media_hash,key, size,(const void*)mdi)))
		{
			MESA_handle_runtime_log(g_dd_run.logger, RLOG_LV_FATAL, DD_MODULE_NAME, 
									"{%s:%d} create_media_cb MESA_htable_add error: [mid: %llu]", 
									__FILE__,__LINE__, *(uint64_t*)key);
			free_media(mdi);
			return -1;
		}		
		
		/*set mediainfo*/
		mdi->mid = meta->pid;		
		mdi->media_len = meta->proglen;
		mdi->media_type = meta->mediatype; 			
		mdi->proto = meta->protocol;	
		mdi->data_flag = meta->data_flag;	
		mdi->thread_seq = meta->thread_seq;			
		mdi->src_ip = meta->src_ip;	
		
		mdi->create_time = time(NULL);		

		mdi->digest_stream_para = Maat_stream_scan_digest_start(g_dd_run.feather,g_dd_run.digest_tableid,mdi->media_len,g_dd_cfg.thread_num);

		inet_ntop(AF_INET, &mdi->src_ip, pbuf, buf_len);
		create_media_write_to_log(mdi, MEDIA_NEW, pbuf);					
	}
	else
	{
		/*收到重复的节目，不受影响，记录刷新日志*/
		renew_media(mdi);					
	}
	return 0;
}

void add_media_info(msg_meta_t* minfo, char* opt, uint32_t src_ip, int thread_seq)
{
	long  	rec_cb = 0;
	meta_t	meta;

	meta.pid = *(uint64_t*)minfo->pid;		
	meta.proglen = minfo->proglen;
	meta.thread_seq = thread_seq;
	meta.src_ip = src_ip;
	meta.protocol = minfo->protocol;
	meta.mediatype = minfo->mediatype;	
	meta.data_flag = minfo->data_flag;		
	meta.opt_num = minfo->opt_num;	
	
	/*不处理选项*/			
	MESA_htable_search_cb(g_dd_run.media_hash,(const uint8_t *)&meta.pid,sizeof(meta.pid),
						  media_create_cb,(void*)&meta,&rec_cb);			
	return ;
}

void put_future_success(future_result_t* result, void* user)
{
	struct future_pdata *pdata = (struct future_pdata *)user;

	resp_write_to_log(PUT_MINIO_SUCC, NULL, pdata->filename, NULL,0);		
	future_destroy(pdata->future);
	free(pdata);
}

void put_future_failed(enum e_future_error err, const char * what, void * user)
{
	struct future_pdata *pdata = (struct future_pdata *)user;

	resp_write_to_log(PUT_MINIO_FAIL, NULL, pdata->filename, NULL,0);	
	future_destroy(pdata->future);
	free(pdata);
}

void send_survey(frag_in_t* frg, media_t* mdi, struct Maat_rule_t* scan_result)
{
	int							send_rec = 0;
	int							bufsize = 0;
	char 						buf_survey[BUF_SIZE] = {0};   
	msg_head_t*					mh_survey = (msg_head_t*)buf_survey;
	msg_result_t*				msi_survey = (msg_result_t *)(buf_survey + sizeof(msg_head_t));
	struct tango_cache_meta 	meta;
	struct future_pdata*		pdata;
	char 						key[256] = {0};
	char 						out_fullfilename[256] = {0};
	char						day_time[32] = {0};
	struct timeval				tv;
	struct timezone 			tz;	
	struct tm					now;

	mh_survey->magic = AV_MAGIC_VALUE;   
    mh_survey->m_type = AV_TYPE_RESULT;
    mh_survey->c_len = sizeof(msg_result_t);

	*((uint64_t *)msi_survey->pid) = mdi->mid; 
    msi_survey->servicetype = scan_result->service_id;
    msi_survey->cfgid = scan_result->config_id;
    msi_survey->level = 100;

	gettimeofday(&tv, &tz);
	localtime_r(&tv.tv_sec, &now);
	strftime(day_time, sizeof(day_time), "%Y%m%d", &now);
	snprintf(key, sizeof(key), "%s/%lu", day_time, mdi->mid);
	memset(&meta, 0, sizeof(struct tango_cache_meta));
	meta.url = key;	

	pdata = (struct future_pdata *)calloc(1, sizeof(struct future_pdata));
	pdata->future = future_create(put_future_success, put_future_failed, pdata);	
	promise_set_ctx(future_to_promise(pdata->future), NULL, NULL);
	cache_evbase_upload_once_data(g_dd_run.instance_asyn[mdi->thread_seq], pdata->future, PUT_MEM_COPY, frg->data, frg->datalen, &meta, out_fullfilename, sizeof(out_fullfilename));

	memcpy(buf_survey+sizeof(msg_head_t)+sizeof(msg_result_t), out_fullfilename, strlen(out_fullfilename));	
	memcpy(pdata->filename, out_fullfilename, strlen(out_fullfilename));
	mh_survey->c_len = sizeof(msg_result_t)+strlen(out_fullfilename)+1; /*include '\0'*/

	bufsize = sizeof(msg_head_t)+sizeof(msg_result_t)+strlen(out_fullfilename)+1;
	send_rec = send_udp_socket_send(g_dd_run.send_survey_sd[mdi->thread_seq],mdi->src_ip,htons(g_dd_cfg.survey_send_port),(char*)buf_survey,bufsize);
	if(-1!=send_rec)
	{
		resp_write_to_log(SEND_SURVEY, msi_survey, out_fullfilename, mdi, 0);
	}
}

long media_preproc_cb(void *data, const uint8_t *key, uint size, void *user_arg)
{	
	media_t* 			mdi = (media_t*)data;	
	frag_in_t*			frg = (frag_in_t*)user_arg;
	int					ret = 0;
	struct Maat_rule_t  scan_result[MAX_SCAN_RESULT]; 
			
	if(NULL==mdi)
	{		
		return -1;			
	}	

	/*节目收到的数据长度统计*/
	mdi->pkt_in++;	
	mdi->byte_in += frg->datalen;

	 /*offset==0, log*/
    if(0==frg->offset)
    {          
        create_media_write_to_log(mdi, MEDIA_OFFSET_ZERO, NULL);
    }    
    /*record maxoffset*/
    mdi->maxoffset = MAX(frg->offset, mdi->maxoffset);

	/*debug*/
	//ret = 1;
	/*命中摘要，不需要再扫描*/
	if(!mdi->hit_digest)
	{
		ret = Maat_stream_scan_digest(&mdi->digest_stream_para, frg->data, frg->datalen, frg->offset, scan_result, MAX_SCAN_RESULT, &mdi->scan_mid);
		if(ret>0)
		{
			mdi->hit_digest = 1;
		}
		for(int i=0;i<ret;i++)
		{
			send_survey(frg, mdi, &scan_result[i]);
		}	
	}		
	return 0;
}

void add_frag(uint64_t pid, uint64_t offset, char* data, uint32_t datalen, uint8_t protocol, uint32_t src_ip, int thread_seq)
{
	frag_in_t* 			frg = NULL;
	long				rec = 0;	

	frg = (frag_in_t*)calloc(1, sizeof(frag_in_t)); 
	frg->mid = pid;	
	frg->offset = offset;
	frg->datalen = datalen;
	frg->data = (char*)malloc(frg->datalen);
	memcpy(frg->data, data, datalen);
	frg->thread_seq = thread_seq;
	frg->src_ip = src_ip;

	MESA_htable_search_cb(g_dd_run.media_hash,(const uint8_t *)&frg->mid,sizeof(frg->mid),
							  	  media_preproc_cb,(void*)frg,&rec);		
	
	return ;	
}

