// output.c
//
//
//
// MODIFIED by
// CREATED BY Guo Le, 08/24/2010
// use sendnto instead of sendto, zhengchao@iie.ac.cn 2012-05-30
// add Unix-domain sokcet,zhengchao@iie.ac.cn 2012-06-04
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>
#include <stddef.h>//offsetof

#include "my_socket.h"

int init_send_udp_socket()
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (-1 == sockfd) 
	{
        return -1;
    }
    return sockfd;
}


void output_udp_clear(int sockfd)
{
    close(sockfd);
}

// send udp packet
int send_udp_socket_send(int sockfd, uint32_t addr, uint16_t port, char *data, int datalen)
{
    struct sockaddr_in dst_addr; /* connector's address information */
    dst_addr.sin_family = AF_INET; /* host byte order */
    dst_addr.sin_port = port; /* short, network byte order */
    dst_addr.sin_addr.s_addr = addr;
    bzero(&(dst_addr.sin_zero), 8); /* zero the rest of the struct */
	int to_send_len=datalen;
	int already_sended_len=0;
	while(to_send_len>0)
	{
		already_sended_len=sendto(sockfd,data,
								to_send_len-already_sended_len,
								0,
								(struct sockaddr *)&(dst_addr),
								sizeof(dst_addr));
		if(already_sended_len==-1)
		{
			if((EAGAIN == errno)||( EINTR == errno )|| (EWOULDBLOCK==errno))
			{
				continue;
			}
			else
			{
				return -1;
			}
		}
		to_send_len-=already_sended_len;						
	}
    return already_sended_len;
}


int output_udp_init_connect(uint32_t addr, uint16_t port, char *dev)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (-1 == sockfd) {
        return -1;
    }
	struct sockaddr_in dst_addr; /* connector's address information */
	dst_addr.sin_family = AF_INET; /* host byte order */
	dst_addr.sin_port = port; /* short, network byte order */
	dst_addr.sin_addr.s_addr = addr;
    bzero(&(dst_addr.sin_zero), 8); /* zero the rest of the struct */

	if (-1 == connect(sockfd, (struct sockaddr*)&dst_addr, sizeof(dst_addr)))
	{
		return -1;
	}

    if (dev) {
        struct ifreq interface;
        strncpy(interface.ifr_ifrn.ifrn_name, dev, IFNAMSIZ);
        if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&interface, sizeof(interface))  < 0) {
            return -1;
        }
    }

    return sockfd;
}

void output_udp_clear_connect(int sockfd)
{
    close(sockfd);
}

// send udp packet
int output_udp_send_connect(int sockfd, uint8_t *data, int datalen)
{
	int to_send_len=datalen;
	int already_sended_len=0;
	while(to_send_len>0)
	{
		already_sended_len= send(sockfd, data, datalen, 0);
		if(already_sended_len==-1)
		{
			if((EAGAIN == errno)||( EINTR == errno )|| (EWOULDBLOCK==errno))
			{
				continue;
			}
			else
			{
				printf("udpout_send_connect: loss data %s\n",strerror(errno));
				return -1;
			}
		}
		to_send_len-=already_sended_len;						
	}
	return already_sended_len;
}

void input_udp_clear(int sockfd)
{
    close(sockfd);
}

int init_unix_socket(const char* file_name)
{
	int sockfd = -1;
	int len = 0;
	struct sockaddr_un un;
	struct timeval send_timeout;
	int send_buf_size = 1024*1024*4;
	
	if(strlen(file_name)+8>sizeof(un.sun_path))
	{
		printf("Unix-domain socket file name too long.\n");
		return -1;
	}
	sockfd=socket(AF_UNIX,SOCK_DGRAM,0);
	if(sockfd<0)
	{
		printf("Get Unix-domain socket error:%s\n",strerror(errno));
		return -1;
	}
	
	send_timeout.tv_sec = 0;
	send_timeout.tv_usec = 100000;
	memset(&un,0,sizeof(un));
	un.sun_family = AF_UNIX;
	if(-1==setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,&send_timeout,sizeof(send_timeout)))
	{
		printf("Set Unix-domain socket error:%s\n",strerror(errno));
		return -1;
	}
	if(-1==setsockopt(sockfd,SOL_SOCKET,SO_SNDBUF,&send_buf_size,sizeof(send_buf_size)))
	{
		printf("Set Unix-domain socket error:%s\n",strerror(errno));
		return -1;
	}
	snprintf(un.sun_path,sizeof(un.sun_path),"%s",file_name);
	len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
	unlink(un.sun_path);
	if(0 > bind(sockfd,(struct sockaddr*)&un,len))
	{
		printf("Bind Unix-domain socket error:%s\n",strerror(errno));
		return -1;
	}
	return sockfd;
}

int  unix_socket_send(int sockfd, struct sockaddr_un *dest_un, const char*data, int datalen)
{
	int to_send_len = datalen;
	int already_sended_len = 0;
	int sock_addr_len=offsetof(struct sockaddr_un,sun_path)+strlen(dest_un->sun_path);
	while(to_send_len>0)
	{
		already_sended_len= sendto(sockfd, data, datalen,0,(struct sockaddr*)dest_un, sock_addr_len);
		if(already_sended_len==-1)
		{
			return -1;
		}
		to_send_len -= already_sended_len;						
	}
	return already_sended_len;
}

int init_recv_udp_socket(uint16_t port)
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sockfd) 
	{
        perror("listener: socket");
        return -1;
    }
    struct sockaddr_in my_addr; 			/* my address information */
    my_addr.sin_family = AF_INET; 			/* host byte order */
    my_addr.sin_port = port; 				/* short, network byte order */
    my_addr.sin_addr.s_addr = INADDR_ANY; 	/* auto-fill with my IP */
    bzero(&(my_addr.sin_zero), 8); 			/* zero the rest of the struct */

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) 
	{
        perror("listener: bind");
        close(sockfd);
        return -1;
    }   
    return sockfd;
}

int recv_udp_socket_recv(int sockfd, uint32_t *src_ip, uint8_t *buf, uint32_t buf_size)
{
    if (NULL == buf)     return -1;

    int numbytes;
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof(their_addr);
	
    if ((numbytes = recvfrom(sockfd, buf, buf_size , 0,(struct sockaddr *)&their_addr, &addr_len)) == -1) 
	{
        perror("recvfrom");
        return -1;
    }
    *src_ip = ((struct sockaddr_in *)&their_addr)->sin_addr.s_addr;
    return numbytes;
}

int init_recv_unix_socket(const char* file_name)
{
	int sockfd=-1;
	int len;
	struct sockaddr_un un;
	if(strlen(file_name)+8>sizeof(un.sun_path))
	{
		printf("Unix-domain socket file name too long.\n");
		return -1;
	}
	sockfd=socket(AF_UNIX,SOCK_DGRAM,0);
	if(sockfd<0)
	{
		printf("Get Unix-domain socket error:%s\n",strerror(errno));
		return -1;
	}
	memset(&un,0,sizeof(un));
	un.sun_family=AF_UNIX;

	snprintf(un.sun_path,sizeof(un.sun_path),"%s",file_name);
	len=offsetof(struct sockaddr_un,sun_path)+strlen(un.sun_path);
	unlink(un.sun_path);
	if(0>bind(sockfd,(struct sockaddr*)&un,len))
	{
		printf("Bind Unix-domain socket error:%s\n",strerror(errno));
		return -1;
	}
	return sockfd;
}

int recv_unix_socket_recv(int sockfd,char*buf,int buf_len)
{
	int recv_len = 0;
	struct sockaddr_storage sa;
	socklen_t sa_len = sizeof(struct sockaddr);
	
	recv_len = recvfrom(sockfd,buf,buf_len,0,(struct sockaddr*)&sa,&sa_len);
	return recv_len;
}

