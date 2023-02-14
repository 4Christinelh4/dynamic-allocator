#include "../virtual_alloc.h"
#include "../virtual_sbrk.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "../cMocka/cmocka.h"

static void *virtual_heap = NULL;

void *virtual_sbrk(int32_t increment) {
    // Your implementation here (for your testing only)
    return sbrk(increment);
}

static int setup(){
    virtual_heap = virtual_sbrk(5);
    init_allocator(virtual_heap, 15, 11);
    return 0;
}

static int teardown(){
    return 0;
}

static void test_init(){
    uint32_t *heap = virtual_heap;
    assert_non_null(virtual_heap);

    assert_int_equal(heap[0], 15);
    assert_int_equal(heap[1], 11);
    assert_int_equal(heap[2], 2);
}

static void test_malloc_successful(){
    void *res_1 = virtual_malloc(virtual_heap, 4000);
    void *res_2 = virtual_malloc(virtual_heap, 2000);
    void *res_3 = virtual_malloc(virtual_heap, 18999);

    assert_non_null(res_1);
    assert_non_null(res_2);
    assert_null(res_3);
    
    assert_int_equal(res_1-virtual_heap, 12);
    assert_int_equal(res_2-res_1, 4096);
}

static void test_info_1(){

    void *res_1 = virtual_malloc(virtual_heap, 4000);
    void *res_2 = virtual_malloc(virtual_heap, 2000);
    void *res_3 = virtual_malloc(virtual_heap, 16000);

    // virtual_info(virtual_heap);

    int fd_actual = open("self_tests/malloc_info_1.txt", O_CREAT|O_TRUNC|O_RDWR, 0644);

    if (fd_actual < 0){
        printf("open error\n");
        exit(1);
    }

    int save_fd = dup(STDOUT_FILENO);

    int redirection = dup2(fd_actual, STDOUT_FILENO);

    if (redirection < 0){
        printf("dup2 error\n");
        exit(1);
    }

    virtual_info(virtual_heap);
    
    close(redirection);
    close(fd_actual);

    dup2(save_fd, STDOUT_FILENO);

    char buf_expect[256];
    char buf_actual[256];

    FILE *expect = fopen("self_tests/malloc_info_1.out", "r");
    if (expect == NULL){
        printf("open expect file error\n");
        exit(1);
    }
    
    FILE *actual = fopen("self_tests/malloc_info_1.txt", "r");
    if (actual == NULL){
        printf("open actual file error\n");
        exit(1);
    }

    while (fgets(buf_actual, 256, actual) != NULL){
        fgets(buf_expect, 256, expect);
        assert_string_equal(buf_expect, buf_actual);
    }

    fclose(actual);
    fclose(expect);
}

int main(){

    const struct CMUnitTest tests[] = {
            cmocka_unit_test_setup_teardown(test_init, setup, teardown),
            cmocka_unit_test_setup_teardown(test_malloc_successful, setup, teardown),
            cmocka_unit_test_setup_teardown(test_info_1, setup, teardown)
            
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}