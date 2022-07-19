#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <strings.h>
#include <cstring>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <stddef.h>

#define SERV_ADDR "serv.socket"

int main(void) {
	int lfd, cfd, len, size, i;
	struct sockaddr_un servaddr, clieaddr;
	char buf[4096];

	if ((lfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		exit(1);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, SERV_ADDR);

	// offsetof 计算sun_path在sockaddr_un中的偏移位置
	len = offsetof(struct sockaddr_un, sun_path) + strlen(servaddr.sun_path);

	// 要确保在bind之前serv.sock文件不能存在
	unlink(SERV_ADDR);
	if (bind(lfd, (struct sockaddr*)&servaddr, len) < 0) {
		perror("bind error");
		exit(1);
	}

	if (listen(lfd, 100) < 0) {
		perror("listen error");
		exit(1);
	}
	printf("Accept....\n");
	while (1) {
		len = sizeof(clieaddr);
		cfd = accept(lfd, (struct sockaddr *)&clieaddr, (socklen_t *)&len);
		if (cfd < 0) {
			perror("accept error");
			exit(1);
		}

		len -= offsetof(struct sockaddr_un, sun_path);
		clieaddr.sun_path[len] = '\0';

		printf("client bind filename %s\n", clieaddr.sun_path);
		while ((size = read(cfd, buf, sizeof(buf))) > 0) {
			for (i = 0; i < size; i++)
				buf[i] = toupper(buf[i]);
			write(cfd, buf, size);
		}
		if (close(cfd) < 0) {
			perror("close cfd error");
			exit(1);
		}
	}
	if (close(lfd) < 0) {
		perror("close lfd error");
		exit(1);
	}
	return 0;
}

