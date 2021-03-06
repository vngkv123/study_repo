#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/ptrace.h>
#include <time.h>
#include <fcntl.h>

int check = 0;
int user_check = 0;
int final[28];
char not_flag[] = {'A', 'e', 'g', 'i', 's', '{', 'c', 'r', 'a', 'c', 'k', '_', 'm', 'e', '_', 'c', 'h', 'a', 'l', 'l', 'e', 'n', 'g', 'e', '_', 'c', 'r', 'a', 'c', 'k', '_', 'm', 'e', '~', '}'};
char key[35] = {0, 0, 0, 0, 0, 0, -20, -71, -22, -1, -4, -1, -30, -82, -44, -1, -1, -63, -96, -14, -23, -24, -57, -22, -109, -17, -12, -92, -75, -22, -52, -13, -10, -32, 0};
int valid = 0;
int seq = 6;

void validation(int _check, int c){
	if( _check == check ){
		key[seq] = ~key[seq];
		seq++;
		if( seq == 34 )
			valid = 1;
	}
	if( c == 27 ){
		printf("[-] Wrong\n");
		exit(0);
	}
}

void *pthread_police(void *args){
	if( ptrace(PTRACE_TRACEME, 0, 0, 0) == -1 ){
		printf("Debugging Detect\n");
		exit(-1);
	}
}

void *pthread_lover(void *args){
	for(int i = 0; i < 28; i++ ){
		printf("Input [trial : %d] : ", i);
		scanf("%d", &user_check);
		validation(user_check, i);
	}
}

void *pthread_checker(void *args){
	while(1){
		if( valid ){
			printf("Congrat!\n");
			for(int i = 0; i < 35; i++){
				printf("%c", not_flag[i] ^ key[i]);
			}
			puts("");
			exit(0);
		}
		check = 0;
		sleep(1);
	}
}

void *pthread_loop(void *args){
	pthread_t police[0x10];
	for(int i = 0; i < 0x10; i++){
		pthread_create( &police[i], 0, (void *)&pthread_police, 0 );
		pthread_join( police[i], 0 );
	}
}

void *pthread_shuffler(void *args){
	while(1){
		for( int i = 0; i < 28; i++ )
			final[i] = rand() % 0x4000;
		sleep(0.3);
	}
}

int main(int argc, char *argv[]){
	int fd = open("/dev/urandom", O_RDONLY);
	unsigned int secret;
	read(fd, &check, 4);
	srand(check);
	for( int i = 0; i < 28; i++ )
		final[i] = rand() % 0x4000;
	setvbuf(stdout, 0, 2, 0);
	setvbuf(stdin, 0, 2, 0);
	setvbuf(stderr, 0, 2, 0);
	printf("Aegis Crackme Challenge\n");
	pthread_t lover, checker, shuffler;
	pthread_create( &lover, 0, (void *)&pthread_lover, 0 );
	pthread_create( &checker, 0, (void *)&pthread_checker, 0 );
	pthread_create( &shuffler, 0, (void *)&pthread_shuffler, 0 );
	pthread_join( lover, 0 );
	pthread_join( checker, 0 );
	pthread_join( shuffler, 0 );
	return 0;
}

void __attribute__((constructor)) __printf(void){
	pthread_t loop;
	pthread_create( &loop, 0, (void *)&pthread_loop, 0 );
}
