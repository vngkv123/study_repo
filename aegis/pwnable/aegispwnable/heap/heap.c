#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

void *chunk_array[0x10];

void initialize(){
	char buf;
	setvbuf(stdin, 0, 2, 0);
	setvbuf(stdout, 0, 2, 0);
	setvbuf(stderr, 0, 2, 0);
	int fd = open("./banner.txt", O_RDONLY);
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
	int size, i;
	write(1, "> ", 2);
	scanf("%d", &size);
	for(i = 0; chunk_array[i]; i++){}
	switch(size){
		case 1:
			chunk_array[i] = malloc(0x68);
			read(0, chunk_array[i], 0x68);
			break;
		case 2:
			chunk_array[i] = malloc(0x88);
			read(0, chunk_array[i], 0x88);
			break;
		case 3:
			chunk_array[i] = malloc(0x228);
			read(0, chunk_array[i], 0x228);
			break;
		default:
			puts("[-] Invalid size");
			break;
	}
}
void gWrite(){
	int index;
	write(1, "> ", 2);
	scanf("%d", &index);
	if( chunk_array[index] && index >= 0 && index <= 15 ){
		write(1, "> ", 2);
		write(1, chunk_array[index], strlen(chunk_array[index]));
	}
	else{
		puts("[-] Invalid index");
	}
}
void gEdit(){
	int index;
	write(1, "> ", 2);
	scanf("%d", &index);
	if( chunk_array[index] && index >= 0 && index <= 15 ){
		write(1, "> ", 2);
		read(0, chunk_array[index], strlen(chunk_array[index]));
	}
	else{
		puts("[-] Invalid index");
	}
}
void gClear(){
	int i;
	write(1, "> ", 2);
	scanf("%d", &i);
	if( chunk_array[i] && i >= 0 && i <= 15 ){
		free(chunk_array[i]);
		chunk_array[i] = 0;
	}
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
				gClear();
				break;
			default:
				puts("[-] Invalid choice");
				break;
		}
	}
	return 0;
}
