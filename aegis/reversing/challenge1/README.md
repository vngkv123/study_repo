# Aegis reversing challenge

**Challenge 1**

- Find simple xor key value
- Main part of code

```
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

unsigned char cipher[27];

char *encrypt(char *plain, int len){
	unsigned char xor_key;
	for( int i = 0; i < len; i++ ){
		xor_key = rand() & 0xff;
		cipher[i] = plain[i] ^ xor_key;
	}
	return (char *)cipher;
}

int main(int argc, char *argv[]){
	printf("Reversing Challenge 1\n");
	unsigned char plain_text[] = "---------------------------";
	unsigned char *cipher = encrypt(plain_text, 27);
  	return 0;
}
```

- Download only `reversing_challenge1` file on the top.
- Upload your write-up on Aegis Webpage.
