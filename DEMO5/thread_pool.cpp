#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <cstring>
#include <signal.h>
#include <errno.h>

#define DEFAULT_TIME 10
#define MIN_WAIT_TASK_NUM 10
#define DEFAULT_THREAD_VARY 10

typedef struct {
	void *(*function) (void *);
	void *arg;
} thread_pool_task_t;

struct thread_pool_t {
	// 用于锁住整个结构体
	pthread_mutex_t lock;
	// 记录忙状态线程个数的锁
	pthread_mutex_t thread_counter;
	// 当任务队列满时，添加任务的线程阻塞，等待此条件变量
	pthread_cond_t queue_not_full;
	// 当任务队列不为空时，通知等待任务的线程
	pthread_cond_t queue_not_empty;
	
	// 存放线程池中每个线程的tid
	pthread_t *threads;
	// 管理线程tid
	pthread_t adjust_tid;
	// 任务队列
	thread_pool_task_t *task_queue;

	// 线程池的最小线程数
	int min_thread_num;
	// 线程池的最大线程数
	int max_thread_num;
	// 当前存活线程的个数
	int live_thread_num;
	// 处于忙状态线程的个数
	int busy_thread_num;
	// 要销毁的线程个数
	int wait_exit_thread_num;

	// task_queue队头下标
	int queue_front;
	// task_queue队尾下标
	int queue_tail;
	// task_queue队中实际任务数
	int queue_len;
	// task_queue队中实际任务数量上限
	int queue_size;

	// 标识这线程池的使用状态
	int shutdown;
};

void *work_thread(void *thread_pool);
void *adjust_thread(void *thread_pool);

int is_thread_alive(pthread_t tid);
int thread_pool_free(thread_pool_t *pool);

thread_pool_t *thread_pool_create(int min_thr_num, int max_thr_num, int queue_max_size) {
	int i;
	thread_pool_t *pool = NULL;
	do {
		if ((pool = (thread_pool_t *)malloc(sizeof(thread_pool_t))) == NULL) {
			printf("malloc thread_pool fail\n");
			break;
		}

		pool->min_thread_num = min_thr_num;
		pool->max_thread_num = max_thr_num;
		pool->busy_thread_num = 0;
		pool->live_thread_num = min_thr_num;
		pool->queue_len = 0;
		pool->queue_size = queue_max_size;
		pool->queue_front = 0;
		pool->queue_tail = 0;
		pool->shutdown = false;

		pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * max_thr_num);
		if (pool->threads == NULL) {
			printf("malloc pool_threads fail\n");
			break;
		}
		memset(pool->threads, 0, sizeof(pthread_t) * max_thr_num);

		pool->task_queue = (thread_pool_task_t *)malloc(sizeof(thread_pool_task_t) * queue_max_size);
		if (pool->task_queue == NULL) {
			printf("malloc task_queue fail\n");
			break;
		}

		if (pthread_mutex_init(&(pool->lock), NULL) != 0
				|| pthread_mutex_init(&(pool->thread_counter), NULL) != 0
				|| pthread_cond_init(&(pool->queue_not_empty), NULL) != 0
				|| pthread_cond_init(&(pool->queue_not_full), NULL) != 0) {
			printf("init the lock or cond fail\n");
			break;
		}

		for (i = 0; i < min_thr_num; i++) {
			pthread_create(&(pool->threads[i]), NULL, work_thread, (void *)pool);
			printf("start thread 0x%x...\n", (unsigned int)pool->threads[i]);
		}
		pthread_create(&(pool->adjust_tid), NULL, adjust_thread, (void *)pool);

		return pool;
	} while (0);

	thread_pool_free(pool);
	return NULL;
}

int thread_pool_add(thread_pool_t *pool, void*(*function)(void *arg), void *arg) {
	pthread_mutex_lock(&(pool->lock));

	// 如果工作队列已经满，需要阻塞等待cond，使得工作队列可以加入一个任务
	while ((pool->queue_size == pool->queue_len) && (!pool->shutdown)) {
		pthread_cond_wait(&(pool->queue_not_full), &(pool->lock));
	}
	if (pool->shutdown) {
		pthread_mutex_unlock(&(pool->lock));
	}

	// 清空工作线程所调用的回调函数的参数arg
	if (pool->task_queue[pool->queue_tail].arg != NULL) {
		free(pool->task_queue[pool->queue_tail].arg);
		pool->task_queue[pool->queue_tail].arg = NULL;
	}

	// 添加任务到任务队列中
	pool->task_queue[pool->queue_tail].function = function;
	pool->task_queue[pool->queue_tail].arg = arg;
	pool->queue_tail = (pool->queue_tail + 1) % pool->queue_size;
	pool->queue_len++;

	// 唤醒线程池中的等待处理任务的线程
	pthread_cond_signal(&(pool->queue_not_empty));
	pthread_mutex_unlock(&(pool->lock));

	return 0;
}

// 线程池中的工作线程
void *work_thread(void *thread_pool) {
	thread_pool_t *pool = (thread_pool_t *)thread_pool;
	thread_pool_task_t task;

	while (1) {
		// 
		pthread_mutex_lock(&(pool->lock));

		// 如果任务队列中没有任务，则调用锁wait阻塞在queue_not_empty上
		while ((pool->queue_len == 0) && (!pool->shutdown)) {
			printf("thread 0x%x is waiting\n", (unsigned int)pthread_self());
			pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));

			// 清楚指定数目的空闲线程，如果要结束的线程个数大于0，则结束线程
			if (pool->wait_exit_thread_num > 0) {
				pool->wait_exit_thread_num--;

				// 如果线程池中的线程个数大于最小值时，则可以结束当前线程
				if (pool->live_thread_num > pool->min_thread_num) {
					printf("thread 0x%x is exiting\n", (unsigned int)pthread_self());
					pool->live_thread_num--;
					pthread_mutex_unlock(&(pool->lock));
					pthread_exit(NULL);
				}
			}
		}

		if (pool->shutdown) {
			pthread_mutex_unlock(&(pool->lock));
			printf("thread 0x%x is exiting\n", (unsigned int)pthread_self());
			pthread_exit(NULL);
		}

		// 从任务队列中取出任务
		task.function = pool->task_queue[pool->queue_front].function;
		task.arg = pool->task_queue[pool->queue_front].arg;
		pool->queue_front = (pool->queue_front + 1) % pool->queue_size;
		pool->queue_len--;

		// 通知可以有新的任务添加进来
		pthread_cond_broadcast(&(pool->queue_not_full));

		// 任务取出后，立即将线程池锁释放掉
		pthread_mutex_unlock(&(pool->lock));

		printf("thread 0x%x start working\n", (unsigned int)pthread_self());
		pthread_mutex_lock(&(pool->thread_counter));
		pool->busy_thread_num++;
		pthread_mutex_unlock(&(pool->thread_counter));
		
		//执行回调函数
		(*(task.function))(task.arg);

		// 处理任务结束
		printf("thread 0x%x end working\n", (unsigned int)pthread_self());
		pthread_mutex_lock(&(pool->thread_counter));
		pool->busy_thread_num--;
		pthread_mutex_unlock(&(pool->thread_counter));

	}

	pthread_exit(NULL);
}

// 管理者线程
void *adjust_thread(void *thread_pool) {
	int i;
	thread_pool_t *pool = (thread_pool_t *)thread_pool;
	while (!pool->shutdown) {
		sleep(DEFAULT_TIME);

		pthread_mutex_lock(&(pool->lock));
		int queue_len = pool->queue_len;
		int live_thr_num = pool->live_thread_num;
		pthread_mutex_unlock(&(pool->lock));

		pthread_mutex_lock(&(pool->thread_counter));
		int busy_thr_num = pool->busy_thread_num;
		pthread_mutex_unlock(&(pool->thread_counter));

		// 当任务数大于最小线程池个数时，且存活的线程数小于最大线程个数时，创建新线程
		if (queue_len >= MIN_WAIT_TASK_NUM && live_thr_num < pool->max_thread_num) {
			pthread_mutex_lock(&(pool->lock));
			int add = 0;

			// 一次增加 DEFAULT_THREAD_VARY 个线程
			for (i = 0; i < pool->max_thread_num && add < DEFAULT_THREAD_VARY
					&& pool->live_thread_num < pool->max_thread_num; i++) {
				// 如果当前的进程ID thread[i]是0，说明当前位置没有进程ID
				// 如果当前的进程已经死亡，则重复利用该位置
				if (pool->threads[i] == 0 || !is_thread_alive(pool->threads[i])) {
					pthread_create(&(pool->threads[i]), NULL, work_thread, (void *)pool);
					add++;
					pool->live_thread_num++;
				}
			}

			pthread_mutex_unlock(&(pool->lock));
		}

		// 销毁多余的空闲进程，当忙碌的线程小于存活的线程数的一半时，且存活的线程数大于最小线程数时
		if ((busy_thr_num * 2) < live_thr_num && live_thr_num > pool->min_thread_num) {
			pthread_mutex_lock(&(pool->lock));
			pool->wait_exit_thread_num = DEFAULT_THREAD_VARY;
			pthread_mutex_unlock(&(pool->lock));

			for (i = 0; i < DEFAULT_THREAD_VARY; i++) {
				pthread_cond_signal(&(pool->queue_not_empty));
			}
		}
	}

	return NULL;
}

int thread_pool_destroy(thread_pool_t *pool) {
	if (pool == NULL)
		return -1;

	pool->shutdown = true;

	pthread_join(pool->adjust_tid, NULL);

	for (int i = 0; i < pool->live_thread_num; i++)
		pthread_cond_broadcast(&(pool->queue_not_empty));
	for (int i = 0; i < pool->live_thread_num; i++)
		pthread_join(pool->threads[i], NULL);

	thread_pool_free(pool);
	return 0;
}

int thread_pool_free(thread_pool_t *pool) {
	if (pool == NULL)
		return -1;

	if (pool->task_queue) 
		free(pool->task_queue);

	if (pool->threads) {
		free(pool->threads);
		pthread_mutex_lock(&(pool->lock));
		pthread_mutex_destroy(&(pool->lock));
		pthread_mutex_lock(&(pool->thread_counter));
		pthread_mutex_destroy(&(pool->thread_counter));
		pthread_cond_destroy(&(pool->queue_not_empty));
		pthread_cond_destroy(&(pool->queue_not_full));
	}
	free(pool);
	pool = NULL;

	return 0;
}

int is_thread_alive(pthread_t tid){
	// 发送0号信号，测试线程是否存活
	int kill_rc = pthread_kill(tid, 0);
	if (kill_rc == ESRCH)
		return false;
	return true;
}

#if true

void *process(void *arg) {
	printf("thread 0x%x working on task %ld\n", (unsigned int)pthread_self(), *((long *)arg));
	sleep(1);
	printf("task %ld is end\n", *((long *)arg));

	return NULL;
}

int main(void) {

	// 创建线程池，里面最少5个线程，最多100个线程
	thread_pool_t *thp = thread_pool_create(5, 100, 100);
	printf("thread pool created\n");
	
	long num[20];
	for (long i = 0; i < 20; i++) {
		num[i] = i;
		printf("add task %ld\n", i);
		thread_pool_add(thp, process, (void*)&num[i]);
	}
	sleep(10);
	thread_pool_destroy(thp);
	printf("thread pool destroy...\n");

	return 0;
}

#endif
