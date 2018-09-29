#ifndef _MAIN_H
#define _MAIN_H

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

#include "Maat_rule.h"
#include "MESA_list_queue.h"
#include "MESA_htable.h"
#include "log.h"

#define			DD_MODULE_NAME						"DIGEST_DECTION"
#define			MAX_PATH_LEN						128
#define			MAX_THREAD_NUM						32


#define			MM_FILE_DIGEST_TABLENAME			"MM_FILE_DIGEST"

#define			MIN(a,b)							((a)<(b)?(a):(b))
#define			MAX(a,b)							((a)>(b)?(a):(b))

typedef struct dd_parameter_s
{	
	void*						logger;
	MESA_lqueue_head*  			recv_lq; 					
	MESA_htable_handle			media_hash; 
	struct cache_evbase_instance* instance_asyn[MAX_THREAD_NUM];	
	Maat_feather_t              feather;  
	
    short                       digest_tableid;  
	short 						log_level;
	int							recv_data_sd;							
	int							send_survey_sd[MAX_THREAD_NUM];								

	FILE*						media_create_file;					//media create log
	FILE*						media_expire_file;					//media expire log
	FILE*						resp_file;							//survey log

	pthread_mutex_t				media_create_file_lock;
	pthread_mutex_t				media_expire_file_lock;
	pthread_mutex_t				resp_file_lock;
	
	struct tm					media_create_filetime;
	struct tm					media_expire_filetime;
	struct tm					resp_filetime;	
	time_t						cur_time;
}dd_parameter_t;

typedef struct dd_configure_s
{		
	uint32_t					thread_num;	
	uint32_t					recv_queue_maxnum;    		
	char						media_create_filename[MAX_PATH_LEN];		//create media
	char						media_expire_filename[MAX_PATH_LEN];		//expire media	
	char						resp_filename[MAX_PATH_LEN];				//about resp msg and monitor and block

	uint32_t					recv_port;	
	uint32_t					survey_send_port;	
}dd_configure_t;

typedef struct dd_status_s
{	
	void*						stat_handle;	
	void*						sysfs_handle;	
 	uint32_t					stat_interval; 	
 	uint32_t					sysinfo_interval; 

	uint64_t					stat_info[LOG_TYPE_MAXNUM][LOG_STAT_MAXNUM];		
	uint64_t					sysinfo_stat[SYSLOG_TYPE_MAXNUM][SYSLOG_STAT_MAXNUM];
	
	/*stat*/	
	int							log_column_id[LOG_STAT_MAXNUM];	
	int							log_line_id[LOG_TYPE_MAXNUM];	
	/*sysinfo*/	
	int							syslog_column_id[SYSLOG_STAT_MAXNUM];	
	int							syslog_line_id[SYSLOG_TYPE_MAXNUM];	
	
}dd_status_t;

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif
