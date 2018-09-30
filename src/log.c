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
#include <inttypes.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <stddef.h>

#include "MESA_htable.h"

#include "field_stat2.h"
#include "log.h"
#include "main.h"
#include "digest_detection.h"

extern dd_parameter_t 		g_dd_run;
extern dd_configure_t 		g_dd_cfg;
extern dd_status_t 			g_dd_stat;

void* thread_stat_output(void *param)
{	
	g_dd_stat.log_column_id[TOTAL_PKTS] = FS_register(g_dd_stat.stat_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"total_pkts");
	g_dd_stat.log_column_id[TOTAL_BYTES] = FS_register(g_dd_stat.stat_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"total_len");
	g_dd_stat.log_column_id[PPS] = FS_register(g_dd_stat.stat_handle,FS_STYLE_COLUMN,FS_CALC_SPEED,"pps");
	g_dd_stat.log_column_id[BPS] = FS_register(g_dd_stat.stat_handle,FS_STYLE_COLUMN,FS_CALC_SPEED,"Bps");
	g_dd_stat.log_column_id[FAIL_PKTS] = FS_register(g_dd_stat.stat_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"fail_pkts");
	g_dd_stat.log_column_id[FAIL_BYTES] = FS_register(g_dd_stat.stat_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"fail_len");
	g_dd_stat.log_column_id[FAIL_PPS] = FS_register(g_dd_stat.stat_handle,FS_STYLE_COLUMN,FS_CALC_SPEED,"fail_pps");
	g_dd_stat.log_column_id[FAIL_BPS] = FS_register(g_dd_stat.stat_handle,FS_STYLE_COLUMN,FS_CALC_SPEED,"fail_Bps");

	g_dd_stat.log_line_id[RECV]=FS_register(g_dd_stat.stat_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"all_recv");	
	g_dd_stat.log_line_id[RECV_DROP]=FS_register(g_dd_stat.stat_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"recv_drop");	
	g_dd_stat.log_line_id[INVALID_RECV]=FS_register(g_dd_stat.stat_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"invalid_recv");
	g_dd_stat.log_line_id[META_RECV]=FS_register(g_dd_stat.stat_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"meta_recv");
	g_dd_stat.log_line_id[DATA_RECV]=FS_register(g_dd_stat.stat_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"data_recv");
	g_dd_stat.log_line_id[OTHER_RECV]=FS_register(g_dd_stat.stat_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"other_recv");
	g_dd_stat.log_line_id[MEDIA_NOMETA]=FS_register(g_dd_stat.stat_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"media_nometa");	
	
	FS_start(g_dd_stat.stat_handle);
	uint32_t i=0, j=0, k=0;
	while(g_dd_stat.stat_interval>0)
	{
		sleep(g_dd_stat.stat_interval);			
		for(i=0;i<LOG_TYPE_MAXNUM;i++)
		{			
			for(j=0,k=0;j<LOG_STAT_MAXNUM;j++)
			{
				switch(j)
				{
					case PPS:
						k=TOTAL_PKTS;
						break;

					case BPS:						
						k=TOTAL_BYTES;
						break;

					case FAIL_PPS:						
						k=FAIL_PKTS;
						break;

					case FAIL_BPS:						
						k=FAIL_BYTES;
						break;

					default:
						k=j;
						break;		
				}				
				FS_operate(g_dd_stat.stat_handle, g_dd_stat.log_line_id[i],g_dd_stat.log_column_id[j],FS_OP_SET, g_dd_stat.stat_info[i][k]);
			}
		}
		FS_passive_output(g_dd_stat.stat_handle);
	}	
	return NULL;	
}

void get_queuelog_count()
{
	uint32_t i=0;
	g_dd_stat.sysinfo_stat[RECV_QUEUE][QUEUE_CURRENT]=0;	
	
	for(i=0;i<g_dd_cfg.thread_num;i++)
	{
		g_dd_stat.sysinfo_stat[RECV_QUEUE][QUEUE_CURRENT] += MESA_lqueue_get_count(g_dd_run.recv_lq[i]);
	}			
}

void get_hashlog_count()
{	
	g_dd_stat.sysinfo_stat[MEDIA_HASH][HASH_CURRENT] = MESA_htable_get_elem_num(g_dd_run.media_hash);
}

void* thread_sysinfo_output(void *param)
{
	g_dd_stat.syslog_column_id[NUM_CUR] = FS_register(g_dd_stat.sysfs_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"queue_cur(hash_cur)");
	g_dd_stat.syslog_column_id[NUM_IN] = FS_register(g_dd_stat.sysfs_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"queue_in(hash_num_expire)");
	g_dd_stat.syslog_column_id[NUM_OUT] = FS_register(g_dd_stat.sysfs_handle,FS_STYLE_COLUMN,FS_CALC_CURRENT,"queue_out(hash_time_expire)");

	
	g_dd_stat.syslog_line_id[RECV_QUEUE] = FS_register(g_dd_stat.sysfs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"recv_queue");	
	g_dd_stat.syslog_line_id[MEDIA_HASH] = FS_register(g_dd_stat.sysfs_handle,FS_STYLE_LINE,FS_CALC_CURRENT,"media_hash");

	FS_start(g_dd_stat.sysfs_handle);
	uint32_t i=0, j=0;
	while(g_dd_stat.sysinfo_interval>0)
	{
		sleep(g_dd_stat.sysinfo_interval);
		get_queuelog_count();		
		get_hashlog_count();	
		for(i=0;i<SYSLOG_TYPE_MAXNUM;i++)
        {
            for(j=0;j<SYSLOG_STAT_MAXNUM;j++)
            {
                FS_operate(g_dd_stat.sysfs_handle, g_dd_stat.syslog_line_id[i],g_dd_stat.syslog_column_id[j],FS_OP_SET, g_dd_stat.sysinfo_stat[i][j]);
            }
        }        
		FS_passive_output(g_dd_stat.sysfs_handle);
	}	
	return NULL;	
	
}

void create_media_write_to_log(media_t* mdi, int type, void* param)
{
	char 		buf[2048] = {0};	
	int 		buflen = 0;
	time_t		cur_time;
	struct timeval 	tv;
	struct timezone tz;
	struct tm	now;
	char		now_time[32] = {0};
	char		day_time[32] = {0};	
	char		filename[MAX_PATH_LEN] = {0};
	FILE*		fp = NULL;
	
	time(&cur_time);
	//localtime_r(&cur_time, &now);
	gettimeofday(&tv, &tz);
	localtime_r(&tv.tv_sec, &now);	
	strftime(now_time, sizeof(now_time), "%Y-%m-%d %H:%M:%S", &now);

	/*init*/
	pthread_mutex_lock(&g_dd_run.media_create_file_lock);
	if(NULL==g_dd_run.media_create_file)
	{
		strftime(day_time, sizeof(day_time), "%Y-%m-%d", &now);		
		snprintf(filename, sizeof(filename), "%s.%s", g_dd_cfg.media_create_filename, day_time);
		g_dd_run.media_create_file = fopen(filename, "a+");			
		g_dd_run.cur_time = cur_time;
		localtime_r(&g_dd_run.cur_time, &g_dd_run.media_create_filetime);
	}	
	if(now.tm_mday!=g_dd_run.media_create_filetime.tm_mday || now.tm_mon!=g_dd_run.media_create_filetime.tm_mon || now.tm_year!=g_dd_run.media_create_filetime.tm_year)
	{
		if(g_dd_run.media_create_file)
		{		
			fclose(g_dd_run.media_create_file);			
			strftime(day_time, sizeof(day_time), "%Y-%m-%d", &now);	
			snprintf(filename, sizeof(filename), "%s.%s", g_dd_cfg.media_create_filename, day_time);
			g_dd_run.media_create_file = fopen(filename, "a+");			
		}		
		g_dd_run.cur_time = cur_time;
                localtime_r(&g_dd_run.cur_time, &g_dd_run.media_create_filetime);
	}	
	pthread_mutex_unlock(&g_dd_run.media_create_file_lock);
	
	switch(type)
	{
		case MEDIA_NEW:
			buflen = snprintf(buf, sizeof(buf), "%20s.%ld,%15s,MID:%" PRIu64 ",media_type:0x%02x,media_len:%10" PRIu64 ",fromIP:%s\n", now_time, tv.tv_usec, "create_media", mdi->mid,mdi->media_type,mdi->media_len, (char*)param);	
			fp = g_dd_run.media_create_file;			
			break;

		case MEDIA_RENEW:			
			buflen = snprintf(buf, sizeof(buf), "%20s.%ld,%15s,MID:%" PRIu64 ",media_type:0x%02x,media_len:%10" PRIu64 "\n", now_time, tv.tv_usec, "renew_media", mdi->mid, mdi->media_type,mdi->media_len);				
			fp = g_dd_run.media_create_file;	
			break;		

		case MEDIA_OFFSET_ZERO:
			buflen = snprintf(buf, sizeof(buf), "%20s.%ld,%15s,MID:%" PRIu64 ",media_type:0x%02x,media_len:%10" PRIu64 ",offset:0\n", now_time, tv.tv_usec, "media_offset_0", mdi->mid, mdi->media_type,mdi->media_len);				
			fp = g_dd_run.media_create_file;			
			break;					
				
		default:
			break;
	}

	if(fp)
	{
		fwrite(buf, buflen, 1, fp);
		fflush(fp);
	}
}

void expire_media_write_to_log(media_t* mdi, int type, void* param)
{
	char 		buf[2048] = {0};	
	int 		buflen = 0;
	time_t		cur_time;
	struct tm	now;
	struct tm	time_cache;
	char		now_time[32] = {0};
	char		create_time[32] = {0};	
	char		day_time[32] = {0};	
	char		filename[MAX_PATH_LEN] = {0};	
	FILE*		fp = NULL;
	
	time(&cur_time);
	localtime_r(&cur_time, &now);
	strftime(now_time, sizeof(now_time), "%Y-%m-%d %H:%M:%S", &now);

	/*init*/
	pthread_mutex_lock(&g_dd_run.media_expire_file_lock);
	if(NULL==g_dd_run.media_expire_file)
	{
		strftime(day_time, sizeof(day_time), "%Y-%m-%d", &now);		
		snprintf(filename, sizeof(filename), "%s.%s", g_dd_cfg.media_expire_filename, day_time);
		g_dd_run.media_expire_file = fopen(filename, "a+");			
		g_dd_run.cur_time = cur_time;
		localtime_r(&g_dd_run.cur_time, &g_dd_run.media_expire_filetime);
	}	
	if(now.tm_mday!=g_dd_run.media_expire_filetime.tm_mday || now.tm_mon!=g_dd_run.media_expire_filetime.tm_mon || now.tm_year!=g_dd_run.media_expire_filetime.tm_year)
	{
		if(g_dd_run.media_expire_file)
		{		
			fclose(g_dd_run.media_expire_file);				
			strftime(day_time, sizeof(day_time), "%Y-%m-%d", &now);	
			snprintf(filename, sizeof(filename), "%s.%s", g_dd_cfg.media_expire_filename, day_time);
			g_dd_run.media_expire_file = fopen(filename, "a+");			
		}
		g_dd_run.cur_time = cur_time;
        localtime_r(&g_dd_run.cur_time, &g_dd_run.media_expire_filetime);
	}
	pthread_mutex_unlock(&g_dd_run.media_expire_file_lock);

	localtime_r(&mdi->create_time,&time_cache);
	strftime(create_time, sizeof(create_time), "%Y-%m-%d %H:%M:%S", &time_cache);
	fp = g_dd_run.media_expire_file;
	buflen = snprintf(buf, sizeof(buf), "%20s,%12s,MID:%" PRIu64 ",media_type:0x%02x,media_len:%10" PRIu64 ",maxoffset:%10" PRIu64 ",pkt:%7u,byte:%10" PRIu64 ",time:%s/%s\n",
					  now_time,
					  "expire_media",
					  mdi->mid, mdi->media_type,mdi->media_len,mdi->maxoffset,
					  mdi->pkt_in, 
					  mdi->byte_in, 
					  now_time,
					  create_time);	
	
	if(fp)
	{
		fwrite(buf, buflen, 1, fp);
		fflush(fp);
	}
}

void resp_write_to_log(int type, msg_result_t* msi_survey, void* param1, void* param2, uint64_t param3)
{
	struct timeval	tv;
	struct timezone tz;
	char		buf[2048] = {0};	
	int 		buflen = 0;
	time_t		cur_time;
	struct tm	now;
	char		now_time[32] = {0};
	char		day_time[32] = {0}; 
	char		filename[MAX_PATH_LEN] = {0};
	media_t* 	mdi = (media_t*)param2;

	time(&cur_time);
	gettimeofday(&tv, &tz);
	localtime_r(&tv.tv_sec, &now);	
	strftime(now_time, sizeof(now_time), "%Y-%m-%d %H:%M:%S", &now);

	/*init*/
	if(NULL==g_dd_run.resp_file)
	{
		strftime(day_time, sizeof(day_time), "%Y-%m-%d", &now);		
		snprintf(filename, sizeof(filename), "%s.%s", g_dd_cfg.resp_filename, day_time);
		g_dd_run.resp_file = fopen(filename, "a+");			
		g_dd_run.cur_time = cur_time;
		localtime_r(&g_dd_run.cur_time, &g_dd_run.resp_filetime);
	}
	
	if(now.tm_mday!=g_dd_run.resp_filetime.tm_mday || now.tm_mon!=g_dd_run.resp_filetime.tm_mon || now.tm_year!=g_dd_run.resp_filetime.tm_year)
	{
		if(g_dd_run.resp_file)
		{		
			strftime(day_time, sizeof(day_time), "%Y-%m-%d", &now);	
			fclose(g_dd_run.resp_file);					
			snprintf(filename, sizeof(filename), "%s.%s", g_dd_cfg.resp_filename, day_time);
			g_dd_run.resp_file = fopen(filename, "a+");			
		}
		g_dd_run.cur_time = cur_time;
		localtime_r(&g_dd_run.cur_time, &g_dd_run.resp_filetime);
	}

	switch(type)
	{
		case SEND_SURVEY:			
			buflen = snprintf(buf, sizeof(buf), 
							  "%s.%ld,%20s,MID:%" PRIu64 ",service:%d,cfgid:%u,recv_byte:%" PRIu64 ",media_len:%" PRIu64 ",thread_id:%d,DataURL:%s\n",
							  now_time, tv.tv_usec,"send_survey",
							  *(uint64_t*)msi_survey->pid, msi_survey->servicetype, msi_survey->cfgid, mdi->byte_in,mdi->media_len,mdi->thread_seq,(char*)param1);
			
			break;

		case PUT_MINIO_SUCC:			
			buflen = snprintf(buf, sizeof(buf), 
							  "%s.%ld,%20s,DataURL:%s\n",
							  now_time, tv.tv_usec,"put_minio_succ",
							  (char*)param1);
			
			break;

		case PUT_MINIO_FAIL:			
			buflen = snprintf(buf, sizeof(buf), 
							  "%s.%ld,%20s,DataURL:%s\n",
							  now_time, tv.tv_usec,"put_minio_fail",
							  (char*)param1);
			
			break;
		
		default:
			break;			
				
	}
	
	if(g_dd_run.resp_file)
	{
		fwrite(buf, buflen, 1, g_dd_run.resp_file);
		fflush(g_dd_run.resp_file);
	}
}

