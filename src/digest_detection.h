#ifndef _DIGEST_DETECTION_H
#define _DIGEST_DETECTION_H

#include "AV_sendback.h"
#include "Maat_rule.h"

#define	MAX_SCAN_RESULT	8

struct future_pdata
{
	struct future* 			future;
	char 					filename[256];
};

typedef struct media_s
{		
	stream_para_t			digest_stream_para;
	scan_status_t 			scan_mid;
	
	uint64_t				mid;							
	int64_t 			  	create_time;
	uint64_t				media_len;	
	
	int 					thread_seq; 					
	uint32_t				src_ip;	
	
	uint8_t 				proto;							
	uint8_t 				media_type; 					
	uint8_t 				data_flag;	
	uint8_t 				hit_digest;	
	
	uint64_t				byte_in;	
	uint64_t				maxoffset;	
	
    uint32_t				pkt_in;			
}media_t;

typedef struct meta_s
{
	uint64_t				pid;			//ID，由数据获取节点生成
	uint64_t				proglen;		//流文件的总字节长度，为0表示长度未知	
	int 					thread_seq;	
	uint32_t				src_ip;			// 获取节点的IP
	uint8_t					protocol;		// 承载协议类型
	uint8_t					mediatype;		// 可能的媒体类型
	uint8_t					data_flag;	    // 数据标志
	uint8_t					opt_num;    	// 选项个数 	
}meta_t;

typedef struct frag_in_s
{	
	uint64_t				mid;
	uint64_t				offset; 	
	char*					data;			
	uint32_t				datalen;	
	uint32_t				src_ip;	
	int 					thread_seq;				
}frag_in_t;


#ifdef __cplusplus
extern "C" {
#endif

void free_media(void* data);
void add_media_info(msg_meta_t* minfo, char* opt, uint32_t src_ip, int thread_seq);
void add_frag(uint64_t pid, uint64_t offset, char* data, uint32_t datalen, uint8_t protocol, uint32_t src_ip, int thread_seq);

#ifdef __cplusplus
}
#endif

#endif
