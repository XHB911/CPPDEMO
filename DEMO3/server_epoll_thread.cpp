#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/epoll.h>
#include <errno.h>

#define SERV_PORT 8888
#define OPEN_MAX 1024

int main(int argc, char **argv) {
	int i, n, num = 0, listenfd, connfd, sockfd;
	ssize_t nready, efd, res;
	char buf[BUFSIZ], str[INET_ADDRSTRLEN];

	struct sockaddr_in cliaddr, servaddr;
	socklen_t clilen;
	struct epoll_event tep, ep[OPEN_MAX];

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		perror("socket error");
		exit(1);
	}

	int opt = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	int ret = bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if (ret < 0) {
		perror("bind error");
		exit(1);
	}

	ret = listen(listenfd, 128);
	if (ret < 0) {
		perror("listen error");
		exit(1);
	}

	efd = epoll_create(OPEN_MAX);
	if (efd == -1) {
		perror("epoll_create");
		exit(1);
	}

	tep.events = EPOLLIN;
	tep.data.fd = listenfd;
	res = epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &tep);
	if (res == -1) {
		perror("epoll_ctl error");
		exit(1);
	}

	printf("wait connecting...\n");
	while (1) {
		nready = epoll_wait(efd, ep, OPEN_MAX, -1);
		if (nready < 0) {
			perror("epoll error");
			exit(1);
		}
		
		for (i = 0; i < nready; i++) {
		       if (!(ep[i].events & EPOLLIN)) continue;

		       if (ep[i].data.fd == listenfd) {
			       clilen = sizeof(cliaddr);
			       connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
			       printf("accept from %s at Port %d\n", inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)), ntohs(cliaddr.sin_port));
			       printf("cfd %d---client %d\n", connfd, ++num);

			       tep.events = EPOLLIN;
			       tep.data.fd = connfd;
			       res = epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &tep);
			       if (res == -1) {
				       perror("epoll_ctl error");
				       exit(1);
			       }
		       } else {
			       sockfd = ep[i].data.fd;
			       if (( n = read(sockfd, buf, sizeof(buf))) < 0) {
				       perror("read n<0 error:");
				       res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
				       close(sockfd);
				} else if (n == 0) {
					if (epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL) == -1) {
						perror("epoll_ctl error");
						exit(1);
					}
					printf("client[%d] closed connection\n", sockfd);
					if (close(sockfd) < 0) {
						perror("close error");
						exit(1);
					}
				} else {
					write(STDOUT_FILENO, buf, n);
					for (int j = 0; j < n; j++)
						buf[j] = toupper(buf[j]);
					ret = write(sockfd, buf, n);
					if (ret < 0) {
						perror("write error");
						exit(1);
					}
				}
			}
		}
	}
	if (close(listenfd) < 0) {
		perror("close listenfd error");
		exit(1);
	}
	if (close(efd) < 0) {
	       perror("close efd error");
	       exit(1);
	} 
	return 0;
}
