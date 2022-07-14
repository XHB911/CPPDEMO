#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <ctype.h>
#include <arpa/inet.h>

#define SERV_PORT 6666
#define SERV_IP	"127.0.0.1"

int main(void) {
	int lfd, cfd;
	struct sockaddr_in serv_addr, clie_addr;
	socklen_t clie_addr_len;
	char buf[BUFSIZ], clie_IP[BUFSIZ];
	int n;

	lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1) {
		perror("socket error");
		exit(1);
	}
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int ret = bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (ret == -1) {
		perror("bind error");
		exit(1);
	}

	ret = listen(lfd, 128);
	if (ret == -1) {
		perror("listen error");
		exit(1);
	}
	
	clie_addr_len = sizeof(clie_addr);
	
	printf("wait connect...\n");

	cfd = accept(lfd, (struct sockaddr *)(&clie_addr), &clie_addr_len);
	if (cfd == -1) {
		perror("accept error");
		exit(1);
	}
	
	printf("client IP:%s, client port:%d\n", inet_ntop(AF_INET, &clie_addr.sin_addr.s_addr, clie_IP, sizeof(clie_IP)), ntohs(clie_addr.sin_port));

	while(1) {
		n = read(cfd, buf, sizeof(buf));
		for (int i = 0; i < n; i++) {
			printf("%c", buf[i]);
			buf[i] = toupper(buf[i]);
		}
		write(cfd, buf, n);
	}
	close(cfd);
	close(lfd);
	return 0;
}
