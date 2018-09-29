#ifndef _MY_SOCKET_H_
#define _MY_SOCKET_H_

#include <stdint.h>
#include <sys/un.h>

#define SOCKET_BUF_SIZE	1500


#ifdef __cplusplus
extern "C" {
#endif

int init_unix_socket(const char* file_name);
int unix_socket_send(int sockfd, struct sockaddr_un *dest_un, const char*data, int datalen);
int init_recv_udp_socket(uint16_t port);
int recv_udp_socket_recv(int sockfd, uint32_t *src_ip, uint8_t *buf, uint32_t buf_size);
int init_recv_unix_socket(const char* file_name);
int recv_unix_socket_recv(int sockfd,char*buf,int buf_len);
int init_send_udp_socket();
int send_udp_socket_send(int sockfd, uint32_t addr, uint16_t port, char *data, int datalen);


#ifdef __cplusplus
}
#endif

#endif 
