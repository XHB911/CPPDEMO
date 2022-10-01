#ifndef _TIME_HEAP_TIMER_H_
#define _TIME_HEAP_TIMER_H_

#include <iostream>
#include <netinet/in.h>
#include <time.h>

using std::exception;

#define BUFFER_SIZE 64

class heap_timer;

struct client_data {
	sockaddr_in address;
	int sockfd;
	char buf[BUFFER_SIZE];
	heap_timer* timer;
};

class heap_timer {
public:
	heap_timer(int delay) {
		expire = time(nullptr) + delay;
	}
public:
	time_t expire;	// 定时器生效的绝对时间
	void (*cb_func)(client_data*);
	client_data* user_data;
};

class time_heap_timer {
public:
	time_heap_timer(int cap) throw (std::exception) : capacity(cap), cur_size(0) {
		array = new heap_timer* [capacity];
		if (!array) {
			throw std::exception();
		}
		for (int i = 0; i < capacity; ++i) {
			array[i] = nullptr;
		}
	}
	time_heap_timer(heap_timer** init_array, int size, int cap) throw (std::exception) : cur_size(size), capacity(cap) {
		if (capacity < size) throw std::exception();
		array = new heap_timer* [capacity];
		if (!array) throw std::exception();
		for (int i = 0; i < capacity; ++i) {
			array[i] = nullptr;
		}
		if (size != 0) {
			for (int i = 0; i < size; ++i) {
				array[i] = init_array[i];
			}
			for (int i = (cur_size - 1) / 2; i >= 0; --i) {
				percolate_down(i);
			}
		}
	}

	~time_heap_timer() {
		for (int i = 0; i < cur_size; ++i) {
			delete array[i];
		}
		delete[] array;
	}
public:
	// 添加目标定时器
	void add_timer(heap_timer* timer) throw(std::exception) {
		if (!timer) return;
		if (cur_size >= capacity) resize();
		// 新插入一个元素，当前堆大小加 1，hole 是新元素的位置
		int hole = cur_size++;
		int parent = 0;
		// 调整堆
		for (; hole > 0; hole = parent) {
			parent = (hole - 1) / 2;
			if (array[parent]->expire <= timer->expire) break;
			array[hole] = array[parent];
		}
		array[hole] = timer;
	}

	void del_timer(heap_timer* timer) {
		if (!timer) return;
		// 延迟销毁，节省了真正删除该定时器造成的开销，但这样做容易使数组膨胀
		timer->cb_func = nullptr;
	}

	heap_timer* top() const {
		if (empty()) return nullptr;
		return array[0];
	}

	void pop_timer() {
		if (empty()) return;
		if (array[0]) {
			delete array[0];
			array[0] = array[-- cur_size];
			// 调整堆
			percolate_down(0);
		}
	}

	void tick() {
		heap_timer* tmp = array[0];
		time_t cur = time(nullptr);
		while (!empty()) {
			if (!tmp) break;
			if (tmp->expire > cur) break;
			if (array[0]->cb_func) {
				array[0]->cb_func(array[0]->user_data);
			}
			pop_timer();
			tmp = array[0];
		}
	}

	bool empty() const {
		return cur_size == 0;
	}
private:
	// 调整最小堆函数
	void percolate_down(int hole) {
		heap_time* tmp = array[hole];
		int child = 0;
		for (; (hole * 2 + 1) <= (cur_size - 1); hole = child) {
			child = hole * 2 + 1;
			if ((child < cur_size - 1) &&  (array[child + 1]->expire < array[child]->expire)) {
				++ child;
			}
			if (array[child]->expire < tmp->expire) {
				array[hole] = array[child];
			} else break;
		}
		array[hole] = tmp;
	}

	void resize() throw(std::exception) {
		heap_timer** temp = new heap_timer*[2 * capacity];
		if (!temp) throw std::exception();
		for (int i = 0; i < 2 * capacity; ++i) temp[i] = nullptr;
		capacity *= 2;
		for (int i = 0; i < cur_size; ++i) {
			temp[i] = array[i];
		}
		delete[] array;
		array = temp;
	}
private:
	heap_timer** array;
	int capacity;
	int cur_size;
};

#endif
