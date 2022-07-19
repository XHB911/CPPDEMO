#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <strings.h>

#define SERV_PORT 8888 
#define SERV_IP	"127.0.0.1"

int main(void) {
	struct sockaddr_in serv_addr, clie_addr;
	socklen_t clie_addr_len;
	int sockfd;
	char buf[BUFSIZ], str[INET_ADDRSTRLEN];
	int n;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		perror("socket error");
		exit(1);
	}

	bzero(&serv_addr, sizeof(serv_addr));	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int ret = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (ret == -1) {
		perror("bind error");
		exit(1);
	}

	printf("wait connect...\n");

	while(1) {
		clie_addr_len = sizeof(clie_addr);
		n = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&clie_addr, &clie_addr_len);
		if (n == -1) {
			perror("recvfrom error");
			exit(1);
		}
		printf("received from %s:%d ----> ", inet_ntop(AF_INET, &clie_addr.sin_addr.s_addr, str, sizeof(str)), ntohs(clie_addr.sin_port));
		fflush(stdout);
		write(STDOUT_FILENO, buf, n);
		for (int i = 0; i < n; i++) {
			buf[i] = toupper(buf[i]);
		}
		n = sendto(sockfd, buf, n, 0, (struct sockaddr *)&clie_addr, sizeof(clie_addr));
		if (n == -1) {
			perror("sendto error");
			exit(1);
		}
	}
	if (close(sockfd) == -1) {
		perror("close sockfd error");
		exit(1);
	}
	return 0;
}
