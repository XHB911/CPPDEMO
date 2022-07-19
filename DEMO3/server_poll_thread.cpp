#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <ctype.h>
#include <poll.h>
#include <errno.h>

#define SERV_PORT 8888
#define OPEN_MAX 1024

int main(int argc, char **argv) {
	int i, n, maxi;
	int nready, listenfd, connfd, sockfd;
	char buf[BUFSIZ], str[INET_ADDRSTRLEN];

	struct sockaddr_in cliaddr, servaddr;
	socklen_t clilen;
	struct pollfd client[OPEN_MAX];

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

	client[0].fd = listenfd;
	client[0].events = POLLIN;
	
	maxi = 0;

	for (i = 1; i < OPEN_MAX; i++)
		client[i].fd = -1;

	printf("wait connecting...\n");
	while (1) {
		nready = poll(client, maxi + 1, -1);
		if (nready < 0) {
			perror("select error");
			exit(1);
		}
		
		if (client[0].revents & POLLIN) {
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
			printf("connected from %s at PORT %d\n", inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)), ntohs(cliaddr.sin_port));

			for (i = 1; i < OPEN_MAX; i++)
				if (client[i].fd < 0) {
					client[i].fd = connfd;
					break;
				}

			if (i == OPEN_MAX) {
				fputs("too many clients\n", stderr);
				exit(1);
			}

			client[i].events = POLLIN;
			if (i > maxi)
				maxi = i;

			if (--nready == 0)
				continue;
		}

		for (int i = 1; i <= maxi; i++) {
			if ((sockfd = client[i].fd) < 0)
				continue;
			if (client[i].revents & POLLIN) {
				if (( n = read(sockfd, buf, sizeof(buf))) < 0) {
					if (errno == ECONNRESET) {
						printf("client[%d] aborted connection\n", i);
						if (close(sockfd) < 0) {
							perror("close error");
							exit(1);
						}
						client[i].fd = -1;
					} else {
						perror("read error");
						exit(1);
					}
				} else if (n == 0) {
					printf("client[%d] closed connection\n", i);
					if (close(sockfd) < 0) {
						perror("close error");
						exit(1);
					}
					client[i].fd = -1;
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
				if (--nready == 0) break;
			}
		}
	}
	ret = close(listenfd);
	if (ret < 0) {
		perror("close listenfd error");
		exit(1);
	}
	return 0;
}
