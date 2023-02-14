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
    init_allocator(virtual_heap, 10, 10);
    
    return 0;
}

static int teardown(){
    return 0;
}

// 1024:1 = 512
static void test_malloc_and_info(){
    
    int fd_1 = open("self_tests/same_size_info.txt", O_CREAT|O_TRUNC|O_RDWR, 0644);
    int save_out = dup(STDOUT_FILENO);
    int redirection = dup2(fd_1, STDOUT_FILENO);

    virtual_info(virtual_heap);

    void *res1 = virtual_malloc(virtual_heap, 22); // success
    void *res2 = virtual_malloc(virtual_heap, 1); // fail!

    assert_non_null(res1);
    assert_null(res2);

    virtual_info(virtual_heap);

    virtual_free(virtual_heap, res1);
    virtual_info(virtual_heap);

    close(redirection);
    close(fd_1);

    dup2(save_out, STDOUT_FILENO);

    // FILE *expect = fopen("self_tests/same_size_info.out", "r");
    // FILE *actual = fopen("self_tests/same_size_info.txt", "r");

    // char buf_expect[256];
    // char buf_actual[256];

    // while (fgets(buf_actual, 256, actual)){
    //     fgets(buf_expect, 256, expect);
    //     assert_string_equal(buf_actual, buf_expect);
    // }

    // fclose(expect);
    // fclose(actual);
}

int main(){

    const struct CMUnitTest tests[] = {
            cmocka_unit_test_setup_teardown(test_malloc_and_info, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}