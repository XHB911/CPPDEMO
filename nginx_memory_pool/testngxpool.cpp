#include "nginx_memory_pool.h"
#include <cstdio>

struct stData {
	char* ptr;
	FILE* pfile;
};

void fun1(void* p) {
	char* p1 = (char*)p;
	printf("free ptr mem!\n");
	free(p1);
}

void fun2(void *pf) {
	FILE* p = (FILE*)pf;
	printf("close file!\n");
	fclose(p);
}

int main() {
	aboo::ngx_mem_pool mempool;
	if (nullptr == mempool.ngx_create_pool(512)) {
		printf("ngx_create_pool fail...\n");
		return -1;
	}

	void *p1 = mempool.ngx_palloc(128);
	if (p1 == nullptr) {
		printf("ngx_palloc 128 bytes fail...\n");
		return -1;
	}

	struct stData *p2 = (struct stData*)mempool.ngx_palloc(512);
	if (p2 == nullptr) {
		printf("ngx_palloc 512 bytes fail...\n");
		return -1;
	}
	p2->ptr = (char*)malloc(12);
	strcpy(p2->ptr, "hello world");
	p2->pfile = fopen("data.txt", "r");

	aboo::ngx_pool_cleanup_s *c1 = mempool.ngx_pool_cleanup_add(sizeof(char*));
	c1->handler = fun1;
	c1->data = p2->ptr;

	aboo::ngx_pool_cleanup_s *c2 = mempool.ngx_pool_cleanup_add(sizeof(FILE*));
	c2->handler = fun2;
	c2->data = p2->pfile;

	mempool.ngx_destroy_pool();
	return 0;
}
