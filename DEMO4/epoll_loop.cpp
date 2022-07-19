#include <cstdio>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <strings.h>
#include <cstdlib>
#include <ctime>
#include <ctype.h>

#define MAX_EVENTS 1024
#define BUFLEN 4096
#define SERV_PORT 8888

void recvdata(int fd, int events, void *arg);
void senddata(int fd, int events, void *arg);

// 红黑树的节点
struct myevent_h {
	// 要监听的文件描述符
	int fd;
	// 对应的监听事件
	int events;
	// 泛型参数
	void *arg;
	// 回调函数
	void (*call_back)(int fd, int events, void *arg);
	// 判断是否在红黑树上，1——>在红黑树上（监听中）
	int status;
	char buf[BUFLEN];
	int len;
	// 记录每次加入红黑树g_efd
	long last_active;
};

int g_efd;
struct myevent_h g_events[MAX_EVENTS + 1];

void eventset(struct myevent_h *ev, int fd, void (*call_back)(int, int, void*), void *arg) {
	ev->fd = fd;
	ev->call_back = call_back;
	ev->events = 0;
	ev->arg = arg;
	ev->status = 0;
//	memset(ev->buf, 0, sizeof(ev->buf));
//	ev->len = 0;
	ev->last_active = time(NULL);
}

void eventadd(int efd, int events, struct myevent_h *ev) {
	struct epoll_event epv = {0, {0}};
	int op;
	epv.data.ptr = ev;
	epv.events = ev->events = events;

	if (ev->status == 1) {
		op = EPOLL_CTL_MOD;
	} else {
		op = EPOLL_CTL_ADD;
		ev->status = 1;
	}

	if (epoll_ctl(efd, op, ev->fd, &epv) < 0)
		printf("event add failed [fd = %d], events[%d]\n", ev->fd, events);

	return ;
}

void eventdel(int efd, struct myevent_h *ev) {
	struct epoll_event epv = {0, {0}};
	if (ev->status != 1)
		return ;

	epv.data.ptr = ev;
	ev->status = 0;
	if (epoll_ctl(efd, EPOLL_CTL_DEL, ev->fd, &epv) < 0) {
		printf("del [fd = %d] error\n", ev->fd);
		exit(1);
	}

	return ;
}

void acceptconn(int lfd, int events, void *arg) {
	struct sockaddr_in cin;
	socklen_t len = sizeof(cin);
	int cfd, i;

	if ((cfd = accept(lfd, (struct sockaddr *)&cin, &len)) == -1) {
		if (errno != EAGAIN && errno != EINTR) {
			// ...
		}
		printf("%s: accept, %s\n", __func__, strerror(errno));
		return ;
	}

	do {
		for (i = 0; i < MAX_EVENTS; i++)
			if (g_events[i].status == 0)
				break;
		if (i == MAX_EVENTS) {
			printf("%s: max connect limit[%d]\n", __func__, MAX_EVENTS);
			break;
		}

		if (fcntl(cfd, F_SETFL, O_NONBLOCK) < 0) {
			printf("%s: fcntl nonblocking failed, %s\n", __func__, strerror(errno));
			break;
		}

		eventset(&g_events[i], cfd, recvdata, &g_events[i]);
		eventadd(g_efd, EPOLLIN, &g_events[i]);
	} while(0);

	printf("new connect [%s:%d][time:%ld], pos[%d]\n", inet_ntoa(cin.sin_addr), ntohs(cin.sin_port), g_events[i].last_active, i);

	return ;
}

void recvdata(int fd, int events, void *arg) {
	struct myevent_h *ev = (struct myevent_h *)arg;
	int len = recv(fd, ev->buf, sizeof(ev->buf), 0);
	
	eventdel(g_efd, ev);

	if (len > 0) {
		ev->len = len;
		ev->buf[len] = '\0';
		printf("received Client[%d] data ---> %s", fd, ev->buf);

		// 对数据进行简单的处理，转为大写
		for (int i = 0; i < len; i++)
			ev->buf[i] = toupper(ev->buf[i]);

		eventset(ev, fd, senddata, ev);
		eventadd(g_efd, EPOLLOUT, ev);
	} else if (len == 0) {
		if (close(ev->fd) < 0) {
			printf("close [fd = %d] error\n", ev->fd);
			exit(1);
		}
		printf("Client[%d] pos[%ld], closed\n", fd, ev - g_events);
	} else {
		if (close(ev->fd)) {
			printf("close [fd = %d] error\n", ev->fd);
			exit(1);
		}		
		printf("recv[fd = %d] error[%d]:%s\n", fd, errno, strerror(errno));
	}

	return ;
}

void senddata(int fd, int events, void *arg) {
	struct myevent_h *ev = (struct myevent_h *)arg;
	int len = send(fd, ev->buf, ev->len, 0);

	if (len > 0) {
		printf("send Client[%d], [len = %d] data ---> %s", fd, len, ev->buf);
		eventdel(g_efd, ev);
		eventset(ev, fd, recvdata, ev);
		eventadd(g_efd, EPOLLIN, ev);
	} else {
		if (close(ev->fd) < 0) {
			printf("close [fd = %d] error\n", ev->fd);
			exit(1);
		}
		eventdel(g_efd, ev);
		printf("send[fd = %d] error %s\n", fd, strerror(errno));
	}

	return ;
}

void init_listen_socket(int fd, short port) {
	// 创建socket文件描述符，并设置为非阻塞模式
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(lfd, F_SETFL, O_NONBLOCK);

	eventset(&g_events[MAX_EVENTS], lfd, acceptconn, &g_events[MAX_EVENTS]);
	eventadd(g_efd, EPOLLIN, &g_events[MAX_EVENTS]);

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	bind(lfd, (struct sockaddr *)&sin, sizeof(sin));

	listen(lfd, 128);

	return ;
}

int main(int argc, char **argv) {
	unsigned short port = SERV_PORT;

	if (argc == 2)
		port = atoi(argv[1]);

	g_efd = epoll_create(MAX_EVENTS + 1);
	if (g_efd <= 0) {
		printf("create efd in %s err %s\n", __func__, strerror(errno));
		exit(1);
	}

	init_listen_socket(g_efd, port);

	// 保存已经满足就绪事件的文件描述符数组
	struct epoll_event events[MAX_EVENTS + 1];
	printf("server running:port[%d]\n", port);

	int checkpos = 0;
	while (1) {
		long now = time(NULL);
		for (int i = 0; i < 100; i++, checkpos++) {
			if (checkpos == MAX_EVENTS)
				checkpos = 0;
			if (g_events[checkpos].status != 1)
				continue;

			if (now - g_events[checkpos].last_active >= 60) {
				if (close(g_events[checkpos].fd) < 0) {
					printf("close %d error", g_events[checkpos].fd);
					exit(1);
				}
				printf("[fd = %d] timeout\n", g_events[checkpos].fd);
				eventdel(g_efd, &g_events[checkpos]);
			}
		}

		// 监听红黑树g_efd， 将满足的时间的文件描述符加到events数组中，1秒没有时间满足，则返回0
		int nfd = epoll_wait(g_efd, events, MAX_EVENTS + 1, 1000);
		if (nfd < 0) {
			perror("epoll_wait error");
			break;
		}

		for (int i = 0; i < nfd; i++) {
			struct myevent_h *ev = (struct myevent_h *)events[i].data.ptr;

			if ((events[i].events & EPOLLIN) && (ev->events & EPOLLIN)) {
				ev->call_back(ev->fd, events[i].events, ev->arg);
			}
			if ((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT)) {
				ev->call_back(ev->fd, events[i].events, ev->arg);
			
			}
		}
	}

	return 0;
}
