#ifndef _TIME_WHEEL_TIMER_H_
#define _TIME_WHEEL_TIMER_H_

#include <time.h>
#include <netinet/in.h>
#include <stdio.h>

#define BUFFER_SIZE 64

class tw_timer;

struct client_data {
	sockaddr_in address;
	int sockfd;
	char buf[BUFFER_SIZE];
	tw_timer* timer;
};

class tw_timer {
public:
	tw_timer(int port, int ts) : next(nullptr), prev(nullptr), rotation(rot), time_slot(ts) {}

public:
	int rotation;	// 记录定时器在时间轮轮转多少圈后生效
	int time_slot;	// 记录定时器属于时间轮上的哪个槽
	void (*cb_func)(client_data*);	// 定时器回调函数
	client_data* uer_data;
	tw_timer* prev;
	tw_timer* next;
};

class time_wheel {
public:
	time_wheel() : cur_slot(0) {
		for (int i = 0; i < N; ++i)
			slots[i] = nullptr;
	}
	~time_wheel() {
		for (int i = 0; i < N; ++i) {
			tw_timer* tmp = slots[i];
			while (tmp) {
				slots[i] = tmp->next;
				delete tmp;
				tmp = slots[i];
			}
		}
	}

	tw_timer* add_timer(int timeout) {
		if (timeout < 0) return nullptr;
		int ticks = 0;
		if (timeout < SI) ticks = 1;
		else ticks = timeout / SI;
		// 计算待插入的定时器在时间轮转动多少圈后被触发
		int rotation = ticks / N;
		// 计算待插入的定时器应该被插入到哪个槽中
		int ts = (cur_slot + (ticks % N)) % N;
		// 创建一个新定时器，它在时间轮转动 rotation 圈后被触发，且位于第 ts 个槽上
		tw_timer* timer = new tw_timer(rotation, ts);
		if (!slots[ts]) {
			printf("add timer, rotation is %d, ts is %d, cur_slot is %d\n", rotation, ts, cur_slot);
			slots[ts] = timer;
		} else {
			timer->next = slots[ts];
			slots[ts]->prev = timer;
			slots[ts] = timer;
		}
		return timer;
	}

	void del_timer(tw_timer* timer) {
		if (!timer) return;
		int ts = timer->time_slot;
		if (timer == slots[ts]) {
			slots[ts] = slots[ts]->next;
			if (slots[ts]) slots[ts]->prev = nullptr;
			delete timer;
		} else {
			timer->prev->next = timer->next;
			if (timer->next) {
				timer->next->prev = timer->prev;
			}
			delete timer;
		}
	}

	// SI 时间到后，调用该函数，时间轮向前滚动一个槽的间隔
	void tick() {
		tw_timer* tmp = slots[cur_slot];
		printf("current slot is %d\n", cur_slot);
		while (tmp) {
			printf("tick the timer once\n");
			if (tmp->rotation > 0) {
				-- tmp->rotation;
				tmp = tmp->next;
			} else {
				tmp->cb_func(tmp->user_data);
				if (tmp == slots[cur_slot]) {
					printf("delete header in cur_slot\n");
					slots[cur_slot] = tmp_next;
					delete tmp;
					if (slots[cur_slot]) {
						slots[cur_slot]->prev = nullptr;
					}
					tmp = slots[cur_slot];
				} else {
					tmp->prev->next = tmp->next;
					if (tmp->next) {
						tmp->next->prev = tmp->prev;
					}
					tw_timer* tmp2 = tmp->next;
					delete tmp;
					tmp = tmp2;
				}
			}
		}
		cur_slot = ++cur_slot % N;
	}
private:
	// 时间轮上的槽数
	static const int N = 60;
	// 每 1s 时间轮转动一次，即槽间隔为 1s
	static const int SI = 1;
	// 时间轮的槽，其中每个元素指向一个定时器链表，链表无序
	tw_timer* slots[N];
	// 时间轮的当前槽
	int cur_slot;
};

#endif
