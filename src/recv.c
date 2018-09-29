
#include "MESA_list_queue.h"
#include "MESA_htable.h"
#include "MESA_handle_logger.h"
#include "my_socket.h"
#include "recv.h"
#include "log.h"
#include "main.h"
#include "digest_detection.h"

extern dd_parameter_t 		g_dd_run;
extern dd_configure_t 		g_dd_cfg;
extern dd_status_t 			g_dd_stat;

void free_recv_data(recv_data_t* recv_data)
{		
	if(NULL!=recv_data)
	{			
		if(NULL!=recv_data->data)
		{
			free(recv_data->data);
			recv_data->data = NULL;
		}				
		free(recv_data);			
	}
}

void proc_data(uint32_t thread_id, uint32_t src_ip, char* packet, int size)
{
	msg_head_t*		mhead = (msg_head_t*)packet;
	msg_meta_t*		minfo = (msg_meta_t*)(packet + sizeof(msg_head_t));
	av_data_t*		mdata = (av_data_t*)(packet + sizeof(msg_head_t));
	char*			data = (char*)mdata + sizeof(av_data_t);	
	char*			opt_ptr = NULL;
	int 			buf_len = 32;
	char			pbuf[32] = {0};
	char			qbuf[32] = {0};
	
	/* Ignore invalid packet */
	if(AV_MAGIC_VALUE != mhead->magic || size <= (int)sizeof(msg_head_t) || size>1600)
	{
		atomic_inc(&g_dd_stat.stat_info[INVALID_RECV][TOTAL_PKTS]);
		atomic_add(&g_dd_stat.stat_info[INVALID_RECV][TOTAL_BYTES],size);
		return;
	}	
	/*prco msg*/
	switch(mhead->m_type)
	{
		case AV_TYPE_META:
			/*write log*/
			if(RLOG_LV_INFO>=g_dd_run.log_level)
			{				
				inet_ntop(AF_INET, &src_ip, pbuf, buf_len);
				inet_ntop(AF_INET, &minfo->capip, qbuf, buf_len);
				MESA_handle_runtime_log(g_dd_run.logger, RLOG_LV_INFO, DD_MODULE_NAME, 
						    			"{%s:%d} %s:%u RECV META [PID:%llu, media_len:%llu, media_type:0x%02x]", 
						    			__FILE__,__LINE__, pbuf, g_dd_cfg.recv_port, *(uint64_t*)minfo->pid, minfo->proglen, minfo->mediatype);
			} 
			/*avoid uncomplete chunk*/
			if(mhead->c_len-sizeof(msg_head_t)>2000)	return;			
			opt_ptr = packet + sizeof(msg_head_t) + sizeof(msg_meta_t);				
			add_media_info(minfo, opt_ptr, src_ip, thread_id);			
			atomic_inc(&g_dd_stat.stat_info[META_RECV][TOTAL_PKTS]);
			atomic_add(&g_dd_stat.stat_info[META_RECV][TOTAL_BYTES],size);
			break;
			
		case AV_TYPE_DATA:	
			/*write log*/		
			if(RLOG_LV_DEBUG>=g_dd_run.log_level)
			{			
				inet_ntop(AF_INET, &src_ip, pbuf, buf_len);				
				{
					MESA_handle_runtime_log(g_dd_run.logger, RLOG_LV_DEBUG, DD_MODULE_NAME, 
							    			"{%s:%d} %s:%u RECV DATA [PID:%llu, offset:%llu, datalen:%u]", 
						   	 				__FILE__,__LINE__, pbuf, g_dd_cfg.recv_port, *(uint64_t*)mdata->pid, mdata->offset, mhead->c_len-sizeof(av_data_t));
				}
			} 				
			if(mhead->c_len-sizeof(av_data_t)>2000)	return;			
			if(mdata->offset>1000000000000)	return;										
			add_frag(*(uint64_t*)mdata->pid, mdata->offset,data, mhead->c_len-sizeof(av_data_t), 0, src_ip, thread_id);
			atomic_inc(&g_dd_stat.stat_info[DATA_RECV][TOTAL_PKTS]);
			atomic_add(&g_dd_stat.stat_info[DATA_RECV][TOTAL_BYTES],size);
			break;	
			      
		default:
			atomic_inc(&g_dd_stat.stat_info[OTHER_RECV][TOTAL_PKTS]);
			atomic_add(&g_dd_stat.stat_info[OTHER_RECV][TOTAL_BYTES],size);
			
			return;
	}
}

void* recv_data_from_queue(void *param)
{
	long				tid = (long)param;	
	recv_data_t*		recv_data = NULL;
	long 				recv_data_size = sizeof(recv_data);
	
	while(1)
	{			
		recv_data = NULL;
		int rec = MESA_lqueue_get_head(g_dd_run.recv_lq[tid], &recv_data, &recv_data_size);
		if (rec<0)
		{	
			usleep(10);
			continue;
		}	
		else
		{
			if(NULL!=recv_data && NULL!=recv_data->data)
			{					
				/*get from queue stat */							
				atomic_inc(&g_dd_stat.sysinfo_stat[RECV_QUEUE][QUEUE_OUT]);
				proc_data(tid, recv_data->src_ip, recv_data->data, recv_data->size);				
			}
			free_recv_data(recv_data);
		}
	}
	return NULL;
}

void* udp_recv_data(void *param)
{
	char			buf[BUF_SIZE] = {0};
	int				size = 0;
	uint32_t		src_ip = 0;		
	recv_data_t* 	recv_data = NULL;
	long			tid = 0;	
 	int 			lq_rec = 0;	
	fd_set 			rset;
	
	while(1)
	{	
		FD_ZERO(&rset);
		FD_SET(g_dd_run.recv_data_sd,&rset);
		if(-1==select(g_dd_run.recv_data_sd+1,&rset,NULL,NULL,NULL))
		{			
			continue;
		}		
		size = recv_udp_socket_recv(g_dd_run.recv_data_sd, &src_ip, (unsigned char*)buf, sizeof(buf));			
		if(size>0)
		{
			if(g_dd_stat.sysinfo_stat[RECV_QUEUE][QUEUE_CURRENT]<g_dd_cfg.recv_queue_maxnum)
			{
				recv_data = (recv_data_t*)calloc(1, sizeof(recv_data_t));
				recv_data->size = size;
				recv_data->src_ip = src_ip;			
				recv_data->data = (char*)malloc(size);
				memcpy(recv_data->data, buf, size);			
				/*data[15] = the last byte of PID*/
				tid = recv_data->data[15]%g_dd_cfg.thread_num;				
				lq_rec = MESA_lqueue_join_tail(g_dd_run.recv_lq[tid], &recv_data, sizeof(recv_data));				
				if(lq_rec==MESA_QUEUE_RET_OK)
				{
					/*add to queue stat */
					atomic_inc(&g_dd_stat.sysinfo_stat[RECV_QUEUE][QUEUE_IN]); 
				}
				else
				{
					free_recv_data(recv_data);
					atomic_inc(&g_dd_stat.stat_info[RECV_DROP][TOTAL_PKTS]);
					atomic_add(&g_dd_stat.stat_info[RECV_DROP][TOTAL_BYTES], size);
				}
			}	
			else
			{
				atomic_inc(&g_dd_stat.stat_info[RECV_DROP][TOTAL_PKTS]);
				atomic_add(&g_dd_stat.stat_info[RECV_DROP][TOTAL_BYTES], size);
			}
			atomic_inc(&g_dd_stat.stat_info[RECV][TOTAL_PKTS]);
			atomic_add(&g_dd_stat.stat_info[RECV][TOTAL_BYTES], size);		
		}		
	}
	return NULL;
}

