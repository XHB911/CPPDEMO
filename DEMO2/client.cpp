#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#define SERV_PORT 8888 
#define SERV_IP "127.0.0.1"

int main(void) {
	int cfd;
	struct sockaddr_in serv_addr;
	char buf[BUFSIZ];

	cfd = socket(AF_INET, SOCK_STREAM, 0);
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, SERV_IP, &serv_addr.sin_addr.s_addr); 
	connect(cfd, (struct sockaddr *)(&serv_addr), sizeof(serv_addr));

	while(1) {
		fgets(buf, sizeof(buf), stdin);
		write(cfd, buf, strlen(buf));

		int n = read(cfd, buf, sizeof(buf));
		write(STDOUT_FILENO, buf, n);
	}
	close(cfd);
	return 0;
}
