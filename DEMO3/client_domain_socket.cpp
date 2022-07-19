#include <cstdio>
#include <unistd.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <cstring>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <stddef.h>

#define SERV_ADDR "serv.socket"
#define CLIE_ADDR "clie.socket"

int main(void) {
	int cfd, len;
	struct sockaddr_un servaddr, clieaddr;
	char buf[4096];

	cfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (cfd < 0) {
		perror("socket cfd error");
		exit(1);
	}

	bzero(&clieaddr, sizeof(clieaddr));
	clieaddr.sun_family = AF_UNIX;
	strcpy(clieaddr.sun_path, CLIE_ADDR);
	
	len = offsetof(struct sockaddr_un, sun_path) + strlen(clieaddr.sun_path);

	unlink(CLIE_ADDR);
	if (bind(cfd, (struct sockaddr *)&clieaddr, len) < 0) {
		perror("bind error");
		exit(1);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, SERV_ADDR);
	
	len = offsetof(struct sockaddr_un, sun_path) + strlen(servaddr.sun_path);

	if (connect(cfd, (struct sockaddr *)&servaddr, len) < 0) {
		perror("connect error");
		exit(1);
	}

	while (fgets(buf, sizeof(buf), stdin) != NULL) {
		write(cfd, buf, strlen(buf));
		len = read(cfd, buf, sizeof(buf));
		write(STDOUT_FILENO, buf, len);
	}

	if (close(cfd) < 0) {
		perror("cfd error");
		exit(1);
	}

	return 0;
}



