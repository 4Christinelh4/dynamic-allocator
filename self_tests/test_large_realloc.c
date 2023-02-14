#include "../virtual_alloc.h"
#include "../virtual_sbrk.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "../cMocka/cmocka.h"

void *virtual_heap = NULL;

void *virtual_sbrk(int32_t increment) {
    // Your implementation here (for your testing only)
    return sbrk(increment);
}

static int setup(){
    virtual_heap = virtual_sbrk(20);
    init_allocator(virtual_heap, 9, 4);
    
    return 0;
}

static int teardown(){
    return 0;
}

// 1024:1 = 512
static void test_malloc_free_realloc_success(){

    int fd = open("self_tests/large_realloc_1.txt", O_CREAT|O_RDWR|O_TRUNC, 0644);
    int save_fd = dup(STDOUT_FILENO);
    int redirection = dup2(fd, STDOUT_FILENO);

    void *res_1 = virtual_malloc(virtual_heap, 55);
    void *res_2 = virtual_malloc(virtual_heap, 60);
    void *res_3 = virtual_malloc(virtual_heap, 59);
    void *res_4 = virtual_malloc(virtual_heap, 60);
    void *res_5 = virtual_malloc(virtual_heap, 200);

    virtual_info(virtual_heap);

    virtual_free(virtual_heap, res_1);
    virtual_free(virtual_heap, res_3);
    virtual_free(virtual_heap, res_4);

    void *realloc_res = virtual_realloc(virtual_heap, res_2, 256);

    assert_int_equal(realloc_res-virtual_heap, 12); // realloc at the first block

    virtual_info(virtual_heap);

    close(redirection);
    close(fd);
    dup2(save_fd, STDOUT_FILENO);

    FILE *actual = fopen("self_tests/large_realloc_1.txt", "r");
    FILE *expect = fopen("self_tests/large_realloc_1.out", "r");

    char buf_actual[256];
    char buf_expect[256];

    while (fgets(buf_actual, 256, actual)){
        fgets(buf_expect, 256, expect);

        assert_string_equal(buf_expect, buf_actual);
    }


    fclose(expect);
    fclose(actual);
}

int main(){

    const struct CMUnitTest tests[] = {
            cmocka_unit_test_setup_teardown(test_malloc_free_realloc_success, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}