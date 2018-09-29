#ifndef __TANGO_CACHE_CLIENT_H__
#define __TANGO_CACHE_CLIENT_H__

#include <event2/event.h>  
#include <event.h>

#include "tfe_future.h"
#include "proxy_cache.h"

#define USER_TAG_MAX_LEN 1518

enum CACHE_ERR_CODE
{
	CACHE_OK=0,
	CACHE_CACHE_MISS, //缓存未命中
	CACHE_TIMEOUT,	//缓存超时
	CACHE_OUTOF_MEMORY,//当前内存占用超过限制，查看MAX_USED_MEMORY_SIZE_MB是否过小或者当前上传速率跟不上调用者的速率
	CACHE_ERR_CURL,
	CACHE_ERR_UNKNOWN
};

enum PUT_MEMORY_COPY_WAY
{
	PUT_MEM_COPY=0, //拷贝这块内存
	PUT_MEM_FREE,   //不拷贝内存，发送完毕由本缓存模块释放该内存
};
enum EVBUFFER_COPY_WAY
{
	EVBUFFER_MOVE=0,//evbuffer_add_buffer
	EVBUFFER_COPY,  //evbuffer_add_buffer_reference
};

struct cache_statistics
{
	long long get_recv_num;	//发起GET的次数
	long long get_succ_num;	//GET成功的次数
	long long get_miss_num;	//GET未命中的次数
	long long get_error_num;//GET失败的次数
	long long put_recv_num;	//发起UPLOAD的次数
	long long put_succ_num;	//UPLOAD成功的次数
	long long put_error_num;//UPLOAD失败的次数
	long long del_recv_num;	//发起DELETE的次数
	long long del_succ_num;	//DELETE成功的次数
	long long del_error_num;//DELETE成功的次数
	long long memory_used;	//当前UPLOAD BODY所占内存大小
	long long session_num;	//当前正在进行GET/PUT的HTTP会话数
};

enum CACHE_RESULT_TYPE
{
	RESULT_TYPE_HEADER=0, //只调用一次
	RESULT_TYPE_USERTAG,  //只调用一次
	RESULT_TYPE_BODY,     //可能调用多次
};

//promise_success的结果result
struct tango_cache_result
{
	const void *data_frag; //如果type为RESULT_TYPE_HEADER，每个头部后会包含一个换行(HTTP1.1格式)
	size_t size;
	enum CACHE_RESULT_TYPE type;
};

enum CACHE_HTTP_HDR_TYPE
{
	HDR_CONTENT_TYPE=0,
	HDR_CONTENT_ENCODING,
	HDR_CONTENT_DISPOSITION,
	HDR_CONTENT_MD5,
	
	HDR_CONTENT_NUM,
};

struct tango_cache_meta
{
	const char* url; //缓存:URL；非结构化日志:文件名；最大长度256字节
	const char* std_hdr[HDR_CONTENT_NUM]; //完整头部，如"Content-Type: text/html"，不要包含换行；NULL表示没有该头部；
	const char* usertag; //可以是任意内容，GET时会原样返回
	size_t usertag_len;  //最大长度USER_TAG_MAX_LEN，0表示没有该头部
	union{
		struct response_freshness put;
		struct request_freshness get;
	};
};

struct tango_cache_instance;
struct tango_cache_ctx;

enum CACHE_ERR_CODE tango_cache_get_last_error(const struct tango_cache_ctx *ctx);
enum CACHE_ERR_CODE tango_cache_ctx_error(const struct tango_cache_instance *instance);
void tango_cache_get_statistics(const struct tango_cache_instance *instance, struct cache_statistics *out);

/*以下所有API线程不安全*/
//每个监听线程创建一个instance
struct tango_cache_instance *tango_cache_instance_new(struct event_base* evbase,const char* profile_path, const char* section, void *runtimelog);


/* GET接口的API*/
//成功时回调promise_success, result为NULL时表示结束；
//失败时回调promise_failed(仅一次)，使用get_last_error获取错误码；
//future不可以为NULL
int tango_cache_fetch_object(struct tango_cache_instance *instance, struct future* future, struct tango_cache_meta *meta);
//从promise_success的result参数提取结果
struct tango_cache_result *tango_cache_read_result(void *promise_result);


/* DELETE接口的API*/
int tango_cache_delete_object(struct tango_cache_instance *instance, struct future* future, const char *objkey);


/* UPLOAD接口的API
 * 注意: UPLOAD接口的API，若future不为NULL，则在上传结束时会调用通知回调函数，否则不调用；
 */

/*完整一次上传API*/
//若path不为空，则输出对象的存储路径
//返回值: 0-成功；<0失败；下同
int tango_cache_upload_once_data(struct tango_cache_instance *instance, struct future* future, 
                                            enum PUT_MEMORY_COPY_WAY way, const char *data, size_t size, 
                                            struct tango_cache_meta *meta, 
                                            char *path, size_t pathsize);
int tango_cache_upload_once_evbuf(struct tango_cache_instance *instance, struct future* future, 
                                            enum EVBUFFER_COPY_WAY way, struct evbuffer *evbuf, 
                                            struct tango_cache_meta *meta, 
                                            char *path, size_t pathsize);
/*流式上传API*/
//返回值: 若为NULL则表示创建失败，调用tango_cache_ctx_error查看错误码是否是CACHE_OUTOF_MEMORY(正常情况下是)；
struct tango_cache_ctx *tango_cache_update_start(struct tango_cache_instance *instance, struct future* future, struct tango_cache_meta *meta);
//返回值: 0-成功；<0失败，调用tango_cache_get_last_error查看错误码；
int tango_cache_update_frag_data(struct tango_cache_ctx *ctx, const char *data, size_t size);
int tango_cache_update_frag_evbuf(struct tango_cache_ctx *ctx, enum EVBUFFER_COPY_WAY way, struct evbuffer *evbuf);
void tango_cache_update_end(struct tango_cache_ctx *ctx);

//获取对象key值；当CACHE_OBJECT_KEY_HASH_SWITCH=1开启对URL/文件名哈希时有用
void tango_cache_get_object_path(const struct tango_cache_ctx *ctx, char *path, size_t pathsize);

#endif

