#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#define MAXLINE 8192
#define SERV_PORT 8888

struct socket_info {
	struct sockaddr_in clieaddr;
	int connect_fd;
};

void* do_work(void *arg) {
	int n, i;
	struct socket_info *ts = (struct socket_info*)arg;
	char buf[BUFSIZ];
	char str[INET_ADDRSTRLEN];

	while(1) {
		n = read(ts->connect_fd, buf, MAXLINE);
		if (n == 0) {
			printf("the client %d closed...\n", ts->connect_fd);
			break;
		} else if (n < 0) {
			perror("read error: ");
			exit(1);
		}
		printf("receiver from %s:%d ---> ", 
				inet_ntop(AF_INET, &(*ts).clieaddr.sin_addr, str, sizeof(str)), ntohs((*ts).clieaddr.sin_port));
		fflush(stdout);
		write(STDOUT_FILENO, buf, n);
		for (i = 0; i < n; i++) {
			buf[i] = toupper(buf[i]);
		}
		write(ts->connect_fd, buf, n);
	}
	int ret = close(ts->connect_fd);
	if(ret < 0) {
		perror("close error:");
		exit(1);
	}
	return (void*)0;
}

int main(void) {
	struct sockaddr_in servaddr, clieaddr;
	socklen_t clieaddr_len;
	int listen_fd, connect_fd;
	pthread_t tid;
	char str[BUFSIZ];
	socket_info ts[256];
	int i = 0, ret = 0;
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		perror("socket error...");
		exit(1);
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	ret = bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if (ret < 0) {
		perror("bind error");
		exit(1);
	}
	ret = listen(listen_fd, 128);	
	if (ret < 0) {
		perror("listen error");
		exit(1);
	}
	printf("Accepting client connect ...\n");

	while (1) {
		clieaddr_len = sizeof(clieaddr);
		connect_fd = accept(listen_fd, (struct sockaddr *)&clieaddr, &clieaddr_len);
		if(connect_fd < 0) {
			perror("connect error...");
			exit(1);
		}
		
		ts[i].clieaddr = clieaddr;
		ts[i].connect_fd = connect_fd;
		printf("connected with  %s:%d\n", 
				inet_ntop(AF_INET, &(ts[i].clieaddr.sin_addr), str, sizeof(str)), ntohs(ts[i].clieaddr.sin_port));
		
		int ret = pthread_create(&tid, NULL, do_work, (void*)&ts[i]);
		if (ret < 0) {
			perror("create pthread error...");
			exit(1);
		}
		pthread_detach(tid);
		i++;
	}

	return 0;
}
