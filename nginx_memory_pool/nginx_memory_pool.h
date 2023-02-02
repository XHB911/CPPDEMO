#ifndef __ABOO_NGINX_MEMORY_POOL_H__
#define __ABOO_NGINX_MEMORY_POOL_H__

#include <cstddef>
#include <cstdlib>
#include <cstring>

namespace aboo {

using u_char = unsigned char;
using u_int = unsigned int;
using u_long = unsigned long;

// 默认一页的大小 4K
const int ngx_pagesize = 4096;
// ngx 小块内存池可分配的最大空间，超过此空间，直接使用 malloc 管理
const int NGX_MAX_ALLOC_FROM_POOL = ngx_pagesize - 1;
// 表示默认的 ngx 内存池开辟的大小
const int NGX_DEFAULT_POOL_SIZE = 16 * 1024;

// 内存池大小按照 16 字节进行对齐
#ifndef NGX_POOL_ALIGNMENT
#define NGX_POOL_ALIGNMENT 16
#endif

// 小块内存分配考虑字节对齐时的单位
#ifndef NGX_ALIGNMENT
#define NGX_ALIGNMENT sizeof(unsigned long)
#endif

// buf 缓冲区清0
#ifndef ngx_memzero
#define ngx_memzero(buf, n) (void)std::memset(buf, 0, n)
#endif

// 把 d 调整到邻近 a 的倍数
#ifndef ngx_align
#define ngx_align(d, a) \
	(((d) + (a - 1)) & ~(a - 1))
#endif

// 把指针 p 调整到邻近 a 的倍数
#ifndef ngx_align_ptr
#define ngx_align_ptr(p, a) \
	(u_char*)(((u_long) (p) + ((u_long)a - 1)) & ~((u_long)a - 1))
#endif

// ngx 内存池最小的 size 调整为 NGX_POOL_ALIGNMENT 的倍数
#ifndef NGX_MIN_POOL_SIZE
#define NGX_MIN_POOL_SIZE \
	ngx_align((sizeof(ngx_pool_t) + 2 * sizeof(ngx_pool_large_t)), NGX_POOL_ALIGNMENT)
#endif

typedef void(*ngx_pool_cleanup_ft)(void *data);
struct ngx_pool_s;

struct ngx_pool_cleanup_s {
	ngx_pool_cleanup_ft handler;
	ngx_pool_cleanup_s* next;
	void* data;
};

struct ngx_pool_large_s {
	ngx_pool_large_s* next;
	void* alloc;
};

struct ngx_pool_data_s {
	u_char* last;
	u_char* end;
	ngx_pool_s* next;
	u_int failed;
};

struct ngx_pool_s {
	ngx_pool_data_s d;
	size_t max;
	ngx_pool_s* current;
	ngx_pool_large_s* large;
	ngx_pool_cleanup_s* cleanup;
};

class ngx_mem_pool {
public:
	// 创建指定 size 大小的内存池，但是小块内存池不超过 1 个页面大小
	void* ngx_create_pool(size_t size);
	// 考虑内存字节对齐，从内存池申请 size 大小的内存
	void* ngx_palloc(size_t size);
	// 不考虑内存字节对齐，从内存池申请 size 大小的内存
	void* ngx_pnalloc(size_t size);
	// 调用 ngx_palloc 实现内存分配，但是会初始化为 0
	void* ngx_pcalloc(size_t size);
	// 释放大块内存
	void ngx_free(void* p);
	// 内存重置函数
	void ngx_reset_pool();
	// 内存池的销毁函数
	void ngx_destroy_pool();
	// 添加回调清理操作函数
	ngx_pool_cleanup_s* ngx_pool_cleanup_add(size_t size);
private:
	ngx_pool_s* pool;
	// 小块内存分配
	void* ngx_palloc_small(size_t size, u_int align);
	// 大块内存分配
	void* ngx_palloc_large(size_t size);
	// 分配新的小块内存池
	void* ngx_palloc_block(size_t size);
};

}

#endif
