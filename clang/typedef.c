#include <stdio.h>
#include <stdlib.h>

typedef void * Var;

int main(int argc, char *argv[]){
    Var test;
    test = malloc(0x50);
    printf("%p\n", test);
    free(test);
    return 0;
}
