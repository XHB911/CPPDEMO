#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <strings.h>
#include <sys/wait.h>

#define SERV_PORT 8888

void wait_child(int signo) {
	while (waitpid(0, NULL, WNOHANG) > 0);
	return ;
}

int main(void) {
	int lfd, cfd;
	pid_t pid;
	struct sockaddr_in serv_addr, clie_addr;
	socklen_t clie_addr_len;
	char buf[BUFSIZ], clie_IP[BUFSIZ];

	lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1) {
		perror("socket error");
		exit(1);
	}

	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int ret = bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (ret == -1) {
		perror("bind error");
		exit(1);
	}


	listen(lfd, 128);
	if (ret == -1) {
		perror("listen error");
		exit(1);
	}


	while(1) {
		clie_addr_len = sizeof(clie_addr);
		cfd = accept(lfd, (struct sockaddr *)&clie_addr, &clie_addr_len);
		printf("client IP <---> %s:%d\n", inet_ntop(AF_INET, &clie_addr.sin_addr.s_addr, clie_IP, sizeof(clie_IP)), ntohs(clie_addr.sin_port));
		if (cfd == -1) {
			perror("accept error");
			exit(1);
		}
		pid = fork();
		if (pid < 0) {
			perror("fork error");
			exit(1);
		} else if (pid == 0) {
			close(lfd);
			break;
		} else {
			close(cfd);
			signal(SIGCHLD, wait_child);
		}
	}
	if(pid == 0) {
		while (1) {
			int n = read(cfd, buf, sizeof(buf));
			if (n == 0) {
				close(cfd);
				return 0;
			} else if (n == -1) {
				perror("read error");
				exit(1);
			} else {
				for (int i = 0; i < n; i++) {
					buf[i] = toupper(buf[i]);
				}
				write(cfd, buf, n);
				write(STDOUT_FILENO, buf, n);
			}
		}

	}
	return 0;
}
