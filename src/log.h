#ifndef _LOG_H
#define _LOG_H

#include "AV_sendback.h"
#include "digest_detection.h"

#if(__GNUC__ * 100 + __GNUC_MINOR__ * 10 + __GNUC_PATCHLEVEL__ >= 410)
#define atomic_inc(x) __sync_add_and_fetch((x),1)
#define atomic_dec(x) __sync_sub_and_fetch((x),1)
#define atomic_add(x,y) __sync_add_and_fetch((x),(y))
#define atomic_sub(x,y) __sync_sub_and_fetch((x),(y))
typedef long atomic_t;
#define ATOMIC_INIT(i)  { (i) }
#define atomic_read(x) __sync_add_and_fetch((x),0)
#define atomic_set(x,y) __sync_lock_test_and_set((x),y)
#else 
typedef long atomic_t;
#define atomic_inc(x)	((*(x))++)
#define atomic_dec(x)	((*(x))--)
#define atomic_add(x,y) ((*(x))+=(y))
#define atomic_sub(x,y) ((*(x))-=(y))
#define ATOMIC_INIT(i)  { (i) }
#define atomic_read(x) 	(*(x))
#define atomic_set(x,y)	((*(x))=(y))
#endif 

/*log type*/
typedef enum
{
	MEDIA_NEW=0,
	MEDIA_RENEW,	
	MEDIA_EXPIRE,	
	MEDIA_OFFSET_ZERO,			
}MEDIA_LOG_TYPE;

typedef enum
{
	SEND_SURVEY,		
	PUT_MINIO_SUCC,
	PUT_MINIO_FAIL,
}RESP_LOG_TYPE;

typedef enum
{
	/*recv data*/
	RECV=0,	
	RECV_DROP,	
	INVALID_RECV,	
	META_RECV, 	
	DATA_RECV,
	OTHER_RECV,		
	MEDIA_NOMETA,	
	LOG_TYPE_MAXNUM,
}LOG_TYPE;

typedef enum
{
	TOTAL_PKTS=0,
	TOTAL_BYTES,
	PPS,
	BPS,
	FAIL_PKTS,
	FAIL_BYTES,
	FAIL_PPS,
	FAIL_BPS,	

	LOG_STAT_MAXNUM,
}LOG_STAT;

typedef enum
{
	RECV_QUEUE=0,
	MEDIA_HASH,

	SYSLOG_TYPE_MAXNUM,	
}SYSLOG_TYPE;

typedef enum
{
	NUM_CUR=0,
	NUM_IN,
	NUM_OUT,

	SYSLOG_STAT_MAXNUM,
}SYSLOG_STAT;

typedef enum
{
    QUEUE_CURRENT=0,
    QUEUE_IN,
    QUEUE_OUT,

    QUEUELOG_STAT_MAXNUM,
}QUEUELOG_STAT;

typedef enum
{
    HASH_CURRENT=0,
    HASH_NUM_EXPIRE,
    HASH_TIME_EXPIRE,

    HASHLOG_STAT_MAXNUM,
}HASHLOG_STAT;

#ifdef __cplusplus
extern "C" {
#endif
void* thread_stat_output(void *param);
void* thread_sysinfo_output(void *param);

void create_media_write_to_log(media_t* mdi, int type, void* param);
void expire_media_write_to_log(media_t* mdi, int type, void* param);
void resp_write_to_log(int type, msg_result_t* msi_survey, void* param1, void* param2, uint64_t param3);

#ifdef __cplusplus
}
#endif

#endif
