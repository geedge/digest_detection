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
#include "field_stat2.h"
#include "main.h"
#include "recv.h"
#include "digest_detection.h"
#include "my_socket.h"
#include "cache_evbase_client.h"

int DD_VERSION_1_0_20181211 = 0;
const char* dd_version_des = "MESA@iie digest";

void digest_detection_history()
{
	//2018.09.27  v1.0  create the project		
	//2018.09.30  v1.0  update cache	
	//2018.12.11  v1.0  add file suffix		
}

dd_parameter_t 		g_dd_run;
dd_configure_t 		g_dd_cfg;
dd_status_t 		g_dd_stat;

const char* hash_eliminate_type[3] = 
{
	"",
	"ELIMINATE_TYPE_NUM",
	"ELIMINATE_TYPE_TIME"
};

int create_pthread(void *(*worker)(void *), void * params, void* logger)
{
    pthread_t	 	thread_desc;
	pthread_attr_t 	attr;

	memset(&thread_desc, 0, sizeof(thread_desc));
	memset(&attr, 0, sizeof(attr));	
	
	if(0 != pthread_attr_init(&(attr))) 
    {
		MESA_handle_runtime_log(logger, RLOG_LV_FATAL, DD_MODULE_NAME, "pthread_attr_init(): %d %s", errno, strerror(errno));
		return -1;
	}
	if(0 != pthread_attr_setdetachstate(&(attr), PTHREAD_CREATE_DETACHED)) 
    {		
		MESA_handle_runtime_log(logger, RLOG_LV_FATAL, DD_MODULE_NAME, "pthread_attr_setdetachstate(): %d %s", errno, strerror(errno));
		return -1;
	}
	if(0 != pthread_create(&(thread_desc), &(attr), (void *(*)(void *))(worker), (void *)(params))) 
    {
		MESA_handle_runtime_log(logger, RLOG_LV_FATAL, DD_MODULE_NAME, "pthread_create(): %d %s", errno, strerror(errno));
		pthread_attr_destroy(&(attr));
		return -1;
	}
	pthread_attr_destroy(&(attr));
	return 0;
}

void rssb_stats_init(const char* filename)
{
	int 	value = 0;		
	char	conf_buf[MAX_PATH_LEN]={0};		

	memset(conf_buf,0,sizeof(conf_buf));
	g_dd_stat.stat_handle = FS_create_handle();	
	MESA_load_profile_string_def(filename, "LOG", "StatFile", conf_buf, sizeof(conf_buf), "./log/dd_stat.log");
	FS_set_para(g_dd_stat.stat_handle, OUTPUT_DEVICE, conf_buf, strlen(conf_buf)+1);
	value = 1;//flush by date
	FS_set_para(g_dd_stat.stat_handle, FLUSH_BY_DATE, &value, sizeof(value));	
	value = 2;//append
	FS_set_para(g_dd_stat.stat_handle, PRINT_MODE, &value, sizeof(value));	
	MESA_load_profile_short_def(filename, "LOG", "StatCycle", (short*)&g_dd_stat.stat_interval,0);
	FS_set_para(g_dd_stat.stat_handle, STAT_CYCLE, &g_dd_stat.stat_interval, sizeof(g_dd_stat.stat_interval));	
	value = (g_dd_stat.stat_interval!=0) ? 1 : 0;
	FS_set_para(g_dd_stat.stat_handle, PRINT_TRIGGER, &value, sizeof(value));
	value = 0;
	FS_set_para(g_dd_stat.stat_handle, CREATE_THREAD, &value, sizeof(value));			

	memset(conf_buf,0,sizeof(conf_buf));
	g_dd_stat.sysfs_handle = FS_create_handle();	
	MESA_load_profile_string_def(filename, "LOG", "SysinfoFile", conf_buf, sizeof(conf_buf), "./log/dd_sysinfo.log");
	FS_set_para(g_dd_stat.sysfs_handle, OUTPUT_DEVICE, conf_buf, strlen(conf_buf)+1);
	value = 1;//flush by date
	FS_set_para(g_dd_stat.sysfs_handle, FLUSH_BY_DATE, &value, sizeof(value));	
	value = 2;//append
	FS_set_para(g_dd_stat.sysfs_handle, PRINT_MODE, &value, sizeof(value));	
	MESA_load_profile_short_def(filename, "LOG", "SysinfoCycle", (short*)&g_dd_stat.sysinfo_interval,0);
	FS_set_para(g_dd_stat.sysfs_handle, STAT_CYCLE, &g_dd_stat.sysinfo_interval, sizeof(g_dd_stat.sysinfo_interval));	
	value = (g_dd_stat.sysinfo_interval!=0) ? 1 : 0;
	FS_set_para(g_dd_stat.sysfs_handle, PRINT_TRIGGER, &value, sizeof(value));
	value = 0;
	FS_set_para(g_dd_stat.sysfs_handle, CREATE_THREAD, &value, sizeof(value));
}

int expire_media_hash_node(void *data, int eliminate_type)
{
	media_t* mdi = (media_t*)data;
	switch(eliminate_type)
	{
		case ELIMINATE_TYPE_NUM:			
			atomic_inc(&g_dd_stat.sysinfo_stat[MEDIA_HASH][HASH_NUM_EXPIRE]);	
			break;
		case ELIMINATE_TYPE_TIME:
			atomic_inc(&g_dd_stat.sysinfo_stat[MEDIA_HASH][HASH_TIME_EXPIRE]);	
			break;
		default:
			break;

	}
	MESA_handle_runtime_log(g_dd_run.logger, RLOG_LV_INFO, DD_MODULE_NAME, 
							"{%s:%d} expire_media_hash_node %s: [MID: %llu]", 
							__FILE__,__LINE__, 
							hash_eliminate_type[eliminate_type], 
							mdi->mid);	
	
	return 1;
}

int read_conf_and_init(const char* filename)
{
	char						conf_buf[MAX_PATH_LEN]={0}; 
	MESA_htable_create_args_t 	hash_args;
	char						table_info_filename [MAX_PATH_LEN]={0};			
	char						full_index[MAX_PATH_LEN]={0}; 
	char						inc_index[MAX_PATH_LEN]={0}; 	
	uint32_t					hash_thread_safe = 512;
	uint32_t					hash_size = 0;
	uint32_t					hash_max_elem_num = 0;
	uint32_t					hash_expire_time = 0;	
	
	/*main log*/
	memset(conf_buf,0,sizeof(conf_buf));
	MESA_load_profile_short_def(filename, "LOG", "LogLevel", (short*)&g_dd_run.log_level,30);
	MESA_load_profile_string_def(filename, "LOG", "LogPath", conf_buf , sizeof(conf_buf),"./log/runtime.log");
	g_dd_run.logger = MESA_create_runtime_log_handle(conf_buf,g_dd_run.log_level);
	if(NULL==g_dd_run.logger)
	{
		printf("[%s] MESA_create_runtime_log_handle error.\n", DD_MODULE_NAME);
		return -1;
	}
	
	/*resp log*/	
	MESA_load_profile_string_def(filename, "LOG", "RespLogPath", g_dd_cfg.resp_filename, sizeof(g_dd_cfg.resp_filename),"./log/survey.log");

	/*create media log*/	
	MESA_load_profile_string_def(filename, "LOG", "MediaCreateLogPath", g_dd_cfg.media_create_filename , sizeof(g_dd_cfg.media_create_filename),"./log/media_create.log");
	
	/*expire media log*/	
	MESA_load_profile_string_def(filename, "LOG", "MediaExpireLogPath", g_dd_cfg.media_expire_filename , sizeof(g_dd_cfg.media_expire_filename),"./log/media_expire.log");
	
	/*thread_num*/
	MESA_load_profile_uint_def(filename, "SYSTEM", "ThreadNum", &g_dd_cfg.thread_num, 1); 
	if(MAX_THREAD_NUM<g_dd_cfg.thread_num)
	{
		printf("[%s] thread_num is more than MAXT_THREAD_NUM:%d.\n", DD_MODULE_NAME, MAX_THREAD_NUM);
		MESA_handle_runtime_log(g_dd_run.logger,RLOG_LV_FATAL,DD_MODULE_NAME,
								(char*)"[%s:%d] thread_num is more than MAXT_THREAD_NUM:%s." , __FILE__,__LINE__,MAX_THREAD_NUM);

	}	
	MESA_load_profile_uint_def(filename, "SYSTEM", "RecvQueueMaxnum", &g_dd_cfg.recv_queue_maxnum, 0);	
	g_dd_run.recv_lq = (MESA_lqueue_head*)calloc(1, g_dd_cfg.thread_num*sizeof(MESA_lqueue_head));
	for(uint32_t i=0;i<g_dd_cfg.thread_num;i++)
	{		
		g_dd_run.recv_lq[i] =  MESA_lqueue_create(1, 0);
	}

	/*media hash param*/	
	MESA_load_profile_uint_def(filename, "SYSTEM", "MediaHashThreadSafe", &hash_thread_safe, 512);
	MESA_load_profile_uint_def(filename, "SYSTEM", "MediaHashSize", &hash_size, 1024*1024);
	MESA_load_profile_uint_def(filename, "SYSTEM", "MediaHashElemNum", &hash_max_elem_num, 1024*1024*16);
	MESA_load_profile_uint_def(filename, "SYSTEM", "MediaHashExpireTime", &hash_expire_time, 120);
	memset(&hash_args,0,sizeof(MESA_htable_create_args_t));
	hash_args.thread_safe = hash_thread_safe; //group lock
	hash_args.recursive = 1;
	hash_args.hash_slot_size = hash_size;
	hash_args.max_elem_num = hash_max_elem_num;
	hash_args.eliminate_type = HASH_ELIMINATE_ALGO_LRU;
	hash_args.expire_time = hash_expire_time;
	hash_args.key_comp = NULL;
	hash_args.key2index = NULL;
	hash_args.data_free = free_media; 
	hash_args.data_expire_with_condition = expire_media_hash_node;	 
	g_dd_run.media_hash = MESA_htable_create(&hash_args, sizeof(hash_args));	
	if(NULL==g_dd_run.media_hash)
	{		
		printf("create media_hash error.\n");
		MESA_handle_runtime_log(g_dd_run.logger, RLOG_LV_FATAL, DD_MODULE_NAME, 
								"{%s:%d} create media_hash error", __FILE__,__LINE__);
		return -1;
	}
	
	/*maat³õÊ¼»¯*/			
	MESA_load_profile_string_def(filename,"MAAT","MaatTableInfo",table_info_filename,sizeof(table_info_filename),"./conf/table_info.conf");
	MESA_load_profile_string_def(filename,"MAAT","FullCfgDir",full_index,sizeof(full_index),"./conf/full/index/");
	MESA_load_profile_string_def(filename,"MAAT","IncCfgDir",inc_index,sizeof(inc_index),"./conf/inc/index/");
	g_dd_run.feather = Maat_summon_feather(g_dd_cfg.thread_num,table_info_filename,full_index,inc_index,g_dd_run.logger);
	g_dd_run.digest_tableid = Maat_table_register(g_dd_run.feather,MM_FILE_DIGEST_TABLENAME);

	/*recv data*/
	MESA_load_profile_uint_def(filename, "NETWORK", "DataRecvPort", &g_dd_cfg.recv_port,33082);	
	g_dd_run.recv_data_sd = init_recv_udp_socket(htons(g_dd_cfg.recv_port));
	if(-1==g_dd_run.recv_data_sd)
	{
		printf("[%s] init_recv_udp_socket error.\n", DD_MODULE_NAME);
		MESA_handle_runtime_log(g_dd_run.logger,RLOG_LV_FATAL,DD_MODULE_NAME,
								(char*)"[%s:%d] init_recv_udp_socket." , __FILE__,__LINE__);
		return -1;
	}

	/*send survey*/
	MESA_load_profile_uint_def(filename, "NETWORK", "SurveySendPort", &g_dd_cfg.survey_send_port,22080);
	for(uint32_t i=0; i<g_dd_cfg.thread_num; i++)
	{
		g_dd_run.send_survey_sd[i] = init_send_udp_socket();
		if(-1==g_dd_run.send_survey_sd[i])
		{
			printf("[%s] init_send_udp_socket error.\n", DD_MODULE_NAME);
			MESA_handle_runtime_log(g_dd_run.logger,RLOG_LV_FATAL,DD_MODULE_NAME,
									(char*)"[%s:%d] init_send_udp_socket." , __FILE__,__LINE__);
			return -1;
		}
	}

	
	/*stat log*/
	rssb_stats_init(filename);
	
	return 0;
}

int   main(int argc, char **argv)
{	
	memset(&g_dd_run, 0, sizeof(dd_parameter_t));	
	memset(&g_dd_cfg, 0, sizeof(dd_configure_t));
	memset(&g_dd_stat, 0, sizeof(dd_status_t));
	struct tango_cache_parameter *parameter;

	/*read main.conf and init*/
	if(-1==read_conf_and_init("./conf/main.conf"))
	{		
		return -1;
	}

	parameter = cache_evbase_parameter_new("./conf/main.conf", "TANGO_CACHE", g_dd_run.logger);
	if(parameter == NULL)
	{
		return -1;
	}
	
	for(uint32_t i=0;i<g_dd_cfg.thread_num;i++)
	{
		g_dd_run.instance_asyn[i] = cache_evbase_instance_new(parameter, g_dd_run.logger);
	}	

	/*stat thread*/
	if(0<g_dd_stat.stat_interval)
	{
		if(-1 == create_pthread(thread_stat_output,NULL,g_dd_run.logger))
    	{
			MESA_handle_runtime_log(g_dd_run.logger, RLOG_LV_FATAL, DD_MODULE_NAME, 
									(char*)"[%s:%d] Thread thread_stat_output Create Failed." ,__FILE__,__LINE__);
			return -1;
    	}
	}		

	/*sysinfo thread*/	
	if(0<g_dd_stat.sysinfo_interval)
	{
		if(-1 == create_pthread(thread_sysinfo_output,NULL,g_dd_run.logger))
    	{
			MESA_handle_runtime_log(g_dd_run.logger, RLOG_LV_FATAL, DD_MODULE_NAME, 
									(char*)"[%s:%d] Thread thread_sysinfo_output Create Failed." ,__FILE__,__LINE__);
			return -1;
    	}
	}	

	/*recv data*/	
	if(-1 == create_pthread(udp_recv_data,NULL, g_dd_run.logger))
	{
		MESA_handle_runtime_log(g_dd_run.logger, RLOG_LV_FATAL, DD_MODULE_NAME, 
								(char*)"[%s:%d] Thread udp_recv_data Create Failed." , __FILE__,__LINE__);
		return -1;
	}	

	for(uint32_t i=0;i<g_dd_cfg.thread_num;i++)
	{    	
		if(-1 == create_pthread(recv_data_from_queue,(void*)i, g_dd_run.logger))
		{
			MESA_handle_runtime_log(g_dd_run.logger, RLOG_LV_FATAL, DD_MODULE_NAME, 
									(char*)"[%s:%d] recv_data_from_queue Create Failed." , __FILE__,__LINE__);
			return -1;
		}
	}	
	
	while(1)
	{
		pause();	
	}
	
	return 0;
}

