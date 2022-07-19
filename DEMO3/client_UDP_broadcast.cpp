#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#define SERV_PORT 8888 
#define SERV_IP "0.0.0.0"
#define CLIENT_PORT 9000

int main(void) {
	struct sockaddr_in localaddr;
	char buf[BUFSIZ];
	int sockfd;
	ssize_t len;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket error");
		exit(1);
	}
	
	memset(&localaddr, 0, sizeof(localaddr));
	localaddr.sin_family = AF_INET;
	localaddr.sin_port = htons(CLIENT_PORT);
	inet_pton(AF_INET, SERV_IP, &localaddr.sin_addr.s_addr); 

	int ret = bind(sockfd, (struct sockaddr*)&localaddr, sizeof(localaddr));
	if (ret == -1) {
		perror("bind error");
		exit(1);
	}
	while(1) {
		len = recvfrom(sockfd, buf, sizeof(buf), 0, NULL, 0);
		if (len == -1)
			perror("recvfrom error");

		write(STDOUT_FILENO, buf, len);
		}
	if (close(sockfd) < 0) {
		perror("close error");
		exit(1);
	}
	return 0;
}
