#include "nginx_memory_pool.h"

namespace aboo {

void* ngx_mem_pool::ngx_create_pool(size_t size) {
	ngx_pool_s* p;
	p = (ngx_pool_s*)std::malloc(size);
	if (nullptr == p) return nullptr;
	p->d.last = (u_char*)p + sizeof(ngx_pool_s);
	p->d.end = (u_char*)p + size;
	p->d.next = nullptr;
	p->d.failed = 0;

	size = size - sizeof(ngx_pool_s);
	p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

	p->current = p;
	p->large = nullptr;
	p->cleanup = nullptr;

	pool = p;

	return p;
}

void* ngx_mem_pool::ngx_palloc(size_t size) {
	if (size <= pool->max) {
		return ngx_palloc_small(size, 1);
	}
	return ngx_palloc_large(size);
}

void* ngx_mem_pool::ngx_pnalloc(size_t size) {
	if (size <= pool->max) {
		return ngx_palloc_small(size, 0);
	}
	return ngx_palloc_large(size);
}
void* ngx_mem_pool::ngx_pcalloc(size_t size) {
	void* p;
	p = ngx_palloc(size);
	if (p) {
		ngx_memzero(p, size);
	}
	return p;
}
void ngx_mem_pool::ngx_free(void* p) {
	ngx_pool_large_s* l;
	for (l = pool->large; l; l = l->next) {
		if (p == l->alloc) {
			std::free(l->alloc);
			l->alloc = nullptr;
			return;
		}
	}
}

void ngx_mem_pool::ngx_reset_pool() {
	ngx_pool_s* p;
	ngx_pool_large_s* l;
	for (l = pool->large; l; l = l->next) {
		if (l->alloc) {
			free(l->alloc);
		}
	}
	p = pool;
	p->d.last = (u_char*)p + sizeof(ngx_pool_s);
	p->d.failed = 0;

	for (p = p->d.next; p; p = p->d.next) {
		p->d.last = (u_char*)p + sizeof(ngx_pool_data_s);
		p->d.failed = 0;
	}

	pool->current = pool;
	pool->large = nullptr;
}

void ngx_mem_pool::ngx_destroy_pool() {
	ngx_pool_s *p, *n;
	ngx_pool_large_s *l;
	ngx_pool_cleanup_s *c;

	for (c = pool->cleanup; c; c = c->next) {
		if (c->handler) {
			c->handler(c->data);
		}
	}

	for (l = pool->large; l; l = l->next) {
		if (l->alloc) {
			free(l->alloc);
		}
	}

	for (p = pool, n = pool->d.next; ; p = n, n = n->d.next) {
		free(p);
		if (n == nullptr)break;
	}
}

ngx_pool_cleanup_s* ngx_mem_pool::ngx_pool_cleanup_add(size_t size) {
	ngx_pool_cleanup_s *c;
	c = (ngx_pool_cleanup_s*)malloc(sizeof(ngx_pool_cleanup_s));
	if (c == nullptr) {
		return nullptr;
	}

	if (size) {
		c->data = ngx_palloc(size);
		if (c->data == nullptr) {
			return nullptr;
		}
	} else {
		c->data = nullptr;
	}
	c->handler = nullptr;
	c->next = pool->cleanup;
	pool->cleanup = c;

	return c;
}

void* ngx_mem_pool::ngx_palloc_small(size_t size, u_int align) {
	u_char* m;
	ngx_pool_s* p;
	p = pool->current;
	do {
		m = p->d.last;
		if (align) {
			m = ngx_align_ptr(m, NGX_ALIGNMENT);
		}
		if ((size_t)(p->d.end - m) >= size) {
			p->d.last = m + size;
			return m;
		}
		p = p->d.next;
	} while(p);
	return ngx_palloc_block(size);
}

void* ngx_mem_pool::ngx_palloc_large(size_t size) {
    void *p;
    u_int n;
    ngx_pool_large_s* large;

    p = malloc(size);
    if (p == nullptr) return nullptr;
    n = 0;
    for (large = pool->large; large; large = large->next) {
		if (large->alloc == nullptr) {
			large->alloc = p;
			return p;
		}

		if (n++ > 3) {
			break;
		}
	}

    large = (ngx_pool_large_s*)ngx_palloc_small(sizeof(ngx_pool_large_s), 1);
    if (large == nullptr) {
		ngx_free(p);
		return nullptr;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}

void* ngx_mem_pool::ngx_palloc_block(size_t size) {
	u_char* m;
	size_t psize;
	ngx_pool_s* p, *newp;
	psize = (size_t)(pool->d.last - (u_char*)pool);
	m = (u_char*)std::malloc(psize);
	if (m == nullptr) return nullptr;
	newp = (ngx_pool_s*)m;
	newp->d.end = m + psize;
	newp->d.next = nullptr;
	newp->d.failed = 0;

	m += sizeof(ngx_pool_data_s);
	m = ngx_align_ptr(m, NGX_ALIGNMENT);
	newp->d.last = m + size;

	for (p = pool->current; p->d.next; p = p->d.next) {
		if (p->d.failed++ > 4) {
			pool->current = p->d.next;
		}
	}
	p->d.next = newp;
	return m;
}

}
