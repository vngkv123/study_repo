#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]){
	int fd;
	char buff[1024];
	int ret;
	fd = open("/dev/null", O_RDWR);
	ret = read(fd, buff, sizeof(buff));
	printf("ret : %d\n", ret);
	strncpy(buff, "Hello", 5);
	ret = write(fd, buff, 5);
	printf("ret : %d\n", ret);
	ret = read(fd, buff, sizeof(buff));
	printf("ret : %d\n", ret);
	close(fd);
	return 0;
}
