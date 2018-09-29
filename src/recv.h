#ifndef _RECV_H
#define _RECV_H

#define BUF_SIZE	1500

typedef struct recv_data_s
{		
	int			size;
	uint32_t	src_ip;
	char*		data;	
}recv_data_t;

#ifdef __cplusplus
extern "C" {
#endif

void* udp_recv_data(void *param);
void* recv_data_from_queue(void *param);

#ifdef __cplusplus
}
#endif

#endif
