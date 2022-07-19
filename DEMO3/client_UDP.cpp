#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#define SERV_PORT 8888 
#define SERV_IP "127.0.0.1"

int main(void) {
	struct sockaddr_in serv_addr;
	char buf[BUFSIZ];
	int sockfd, n;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket error");
		exit(1);
	}
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, SERV_IP, &serv_addr.sin_addr.s_addr); 

	while(fgets(buf, BUFSIZ, stdin) != NULL) {
		n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
		if (n == -1) {
			perror("sendto error");
			exit(1);
		}
		n = recvfrom(sockfd, buf, sizeof(buf), 0, NULL, 0);
		if (n == -1)
			perror("recvfrom error");

		write(STDOUT_FILENO, buf, n);
	}
	if (close(sockfd) < 0) {
		perror("close error");
		exit(1);
	}
	return 0;
}
