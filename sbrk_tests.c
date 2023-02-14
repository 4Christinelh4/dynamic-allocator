#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

/*
 * When you use sbrk(0) you get the current "break" address.
 * When you use sbrk(size) you get the previous "break" address,
 * i.e. the one before the change.
 */

void *heap = NULL;

int main(){
    heap = sbrk(0);
    printf("heap: %p, current brek is %p \n", heap, sbrk(0));

    heap = sbrk(256);
    printf("heap: %p, current brek is %p \n", heap, sbrk(0));

    for (int i=0; i<256; i++){
        *((uint8_t *)heap + i) = 6;
    }

    heap = sbrk(64);
    printf("heap: %p, current brek is %p \n", heap, sbrk(0));

    heap = sbrk(-256);
    printf("heap: %p, current brek is %p \n", heap, sbrk(0));

    heap = sbrk(256);
    printf("heap: %p, current brek is %p \n", heap, sbrk(0));

    return 0;
}