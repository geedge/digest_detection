#ifndef __CACHE_ASYN_CLIENT_H__
#define __CACHE_ASYN_CLIENT_H__

#include <event2/event.h>  
#include <event.h>

#include "tango_cache_client.h"

struct cache_evbase_instance
{
	struct tango_cache_instance *instance;
	evutil_socket_t notify_readfd;
	evutil_socket_t notify_sendfd;
	struct event_base* evbase;
};

struct cache_evbase_ctx
{
	struct tango_cache_ctx *ctx;
	struct cache_evbase_instance *instance_asyn;
};

/*所有API线程不安全，API的使用说明参考tango_cache_client.h*/

enum CACHE_ERR_CODE cache_evbase_get_last_error(const struct cache_evbase_ctx *ctx_asyn);
enum CACHE_ERR_CODE cache_evbase_ctx_error(const struct cache_evbase_instance *instance);
void cache_evbase_get_statistics(const struct cache_evbase_instance *instance, struct cache_statistics *out);

/*创建实例，每线程一个，或使用时加锁*/
struct cache_evbase_instance *cache_evbase_instance_new(const char* profile_path, const char* section, void *runtimelog);


//GET接口，成功返回0，失败返回-1，future回调函数会在另外的线程中执行，下同
int cache_evbase_fetch_object(struct cache_evbase_instance *instance, struct future* future, struct tango_cache_meta *meta);
struct tango_cache_result *cache_evbase_read_result(void *promise_result);

//DELETE接口
int cache_evbase_delete_object(struct cache_evbase_instance *instance, struct future* future, const char *objkey);

//一次性上传接口
int cache_evbase_upload_once_data(struct cache_evbase_instance *instance, struct future* future, 
                                               enum PUT_MEMORY_COPY_WAY way, const char *data, size_t size, 
                                               struct tango_cache_meta *meta, 
                                               char *path, size_t pathsize);
int cache_evbase_upload_once_evbuf(struct cache_evbase_instance *instance, struct future* future, 
                                                struct evbuffer *evbuf, 
                                                struct tango_cache_meta *meta, 
                                                char *path, size_t pathsize);

//流式上传接口
struct cache_evbase_ctx *cache_evbase_update_start(struct cache_evbase_instance *instance, struct future* future, struct tango_cache_meta *meta);
int cache_evbase_update_frag_data(struct cache_evbase_ctx *ctx_asyn, enum PUT_MEMORY_COPY_WAY way, const char *data, size_t size);
int cache_evbase_update_frag_evbuf(struct cache_evbase_ctx *ctx_asyn, struct evbuffer *evbuf);
void cache_evbase_update_end(struct cache_evbase_ctx *ctx_asyn);

void cache_evbase_get_object_path(const struct cache_evbase_ctx *ctx, char *path, size_t pathsize);

#endif

