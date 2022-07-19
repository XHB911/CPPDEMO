#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <cstring>
#include <ctype.h>
#include <arpa/inet.h>
#include <strings.h>

#define SERV_PORT 8888 
#define CLIENT_PORT 9000

#define BROADCAST_IP "192.168.65.255"

int main(void) {
	struct sockaddr_in serv_addr, clie_addr;
	int sockfd;
	char buf[BUFSIZ];

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

	int flag = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag));
	
	bzero(&clie_addr, sizeof(clie_addr));
	clie_addr.sin_family = AF_INET;
	inet_pton(AF_INET, BROADCAST_IP, &clie_addr.sin_addr.s_addr);
	clie_addr.sin_port = htons(CLIENT_PORT);

	printf("wait connect...\n");
	int i = 0;
	while(1) {
		sprintf(buf, "Drink %d glasses of water\n", i++);
		sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&clie_addr, sizeof(clie_addr));
		sleep(1);
	}
	if (close(sockfd) == -1) {
		perror("close sockfd error");
		exit(1);
	}
	return 0;
}
