#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

char ukey[32];
char skey[32];
int _chk;

void initialize(){
	char buf;
	setvbuf(stdin, 0, 2, 0);
	setvbuf(stdout, 0, 2, 0);
	setvbuf(stderr, 0, 2, 0);
	int fd = open("/dev/urandom", O_RDONLY);
	if( read(fd, skey, 32) != 32 ){ exit(-1); }
	close(fd);
	fd = open("./banner.txt", O_RDONLY);
	if( fd < 0 ){ exit(-1); }
	while( read(fd, &buf, 1) )
		write(1, &buf, 1);
	close(fd);
}

void menu(){
	puts("1. Read");
	puts("2. Write");
	puts("3. Edit");
	write(1, "> ", 2);
}

void gRead(){
	if( _chk ){ return; }
	write(1, "> ", 2);
	read(0, ukey, 32);
	_chk = 1;
}
void gWrite(){
	write(1, "> ", 2);
	write(1, ukey, 32);
}
void gEdit(){
	write(1, "> ", 2);
	read(0, ukey, strlen(ukey));
}
void gPwn(){
	if( memcmp(ukey, skey, 32) != 0 ){ exit(-1); }
	char buf[16] = {0, };
	write(1, "> ", 2);
	read(0, buf, 0x38);
}

int main(int argc, char *argv[]){
	int choice;
	initialize();
	while(1){
		menu();
		scanf("%d", &choice);
		switch(choice){
			case 1:
				gRead();
				break;
			case 2:
				gWrite();
				break;
			case 3:
				gEdit();
				break;
			case 0x1337:
				gPwn();
				break;
			default:
				puts("[-] Invalid choice");
				break;
		}
	}
	return 0;
}
