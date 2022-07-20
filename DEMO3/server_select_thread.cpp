#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>

#define SERV_PORT 8888

int main(int agrc, char **argv) {
	int i, j, n, maxi;
	int nready, client[FD_SETSIZE];
	int maxfd, listenfd, connfd, sockfd;
	char buf[BUFSIZ], str[INET_ADDRSTRLEN];

	struct sockaddr_in clie_addr, serv_addr;
	socklen_t clie_addr_len;
	fd_set rset, allset;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(SERV_PORT);

	bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	listen(listenfd, 128);

	maxfd = listenfd;
	maxi = -1;
	for (i = 0; i < FD_SETSIZE; i++)
		client[i] = -1;

	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);

	printf("wait connect...\n");
	while (1) {
		rset = allset;
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
		if (nready < 0) {
			perror("select error");
			exit(1);
		}
		if (FD_ISSET(listenfd, &rset)) {
			clie_addr_len = sizeof(clie_addr);
			connfd = accept(listenfd, (struct sockaddr *)&clie_addr, &clie_addr_len);
			printf("connect from [ %s:%d ]\n", inet_ntop(AF_INET, &clie_addr.sin_addr, str, sizeof(str)), ntohs(clie_addr.sin_port));

			for (i = 0; i < FD_SETSIZE; i++)
				if (client[i] < 0) {
					client[i] = connfd;
					break;
				}

			if (i == FD_SETSIZE) {
				fputs("too many clients\n", stderr);
				exit(1);
			}

			FD_SET(connfd, &allset);
			if (connfd > maxfd)
				maxfd = connfd;

			if (i > maxi)
				maxi = i;

			if (--nready == 0)
				continue;
		}

		for (i = 0; i <= maxi; i++) {
			if ((sockfd = client[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset)) {
				if ((n = read(sockfd, buf, sizeof(buf))) == 0) {
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
				} else if (n > 0) {
					write(STDOUT_FILENO, buf, n);
					for (j = 0; j < n; j++)
						buf[j] = toupper(buf[j]);
					write(sockfd, buf, n);
				}
				if (--nready == 0)
					break;
			}
		}
	}
	close(listenfd);
	return 0;
}
