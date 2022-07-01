#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void error(const char* s, int code) {
	perror(s);
	exit(1);
}

int main(int argc, char** argv) {
	int n;
	printf("请输入进程个数");
	scanf("%d", &n);
	struct stat fstatbuf;
	if(stat(argv[1], &fstatbuf) == -1) error("get stat failure", 1);

	int f_size = fstatbuf.st_size;
	int fd_r = open(argv[1], O_RDONLY);
	if(fd_r == -1) error("open read file failure", 1);
	int fd_w = open(argv[2], O_RDWR);
	if(fd_w == -1) {
		perror("open file failure\n create file \n");
		fd_w = open(argv[2], O_RDWR | O_CREAT, 0644);
		if(fd_w == -1) error("create file faile", 1);
	}
	pid_t pid = -1;
	int pid_file_size = f_size / n;
	if(ftruncate(fd_w, f_size) == -1) error("truncate failure", 1);
	for (int i = 0; i < n; i++) {
		pid = fork();
		if(pid == 0) {
			if(lseek(fd_r, i * pid_file_size, SEEK_SET) == -1) error("seek file_rp failure", 1);
			if(lseek(fd_w, i * pid_file_size, SEEK_SET) == -1) error("seek file_wp failure", 1);
			char buf[pid_file_size << 1];
			if(read(fd_r, buf, pid_file_size) == -1) error("read failure", 1);
			if(write(fd_w, buf, pid_file_size) == -1) error("write failure", 1);
			if(i == n-1) {	
				if(read(fd_r, buf, f_size % n) == -1) error("read failure", 1);
				if(write(fd_w, buf, f_size % n) == -1) error("write failure", 1);
			}
			break;
		}
	}
	if(pid) {
		sleep(1);
		write(1, "copy success!\n", 14);
	}
	return 0;
}

