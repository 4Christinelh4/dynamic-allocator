#include "../virtual_alloc.h"
#include "../virtual_sbrk.h"
#include <unistd.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "../cMocka/cmocka.h"

static void *virtual_heap = NULL;

void *virtual_sbrk(int32_t increment) {
    // Your implementation here (for your testing only)
    return sbrk(increment);
}

static int setup(){
    virtual_heap = virtual_sbrk(5);
    init_allocator(virtual_heap, 8, 5);
    return 0;
}

static int teardown(){
    return 0;
}

static void test_realloc_success_fail(){
    void *res_1 = virtual_malloc(virtual_heap, 60);
    void *res_2 = virtual_malloc(virtual_heap, 60);
    void *res_3 = virtual_malloc(virtual_heap, 58);
    void *res_4 = virtual_malloc(virtual_heap, 40);

    int ret_success = virtual_free(virtual_heap, res_3);
    assert_int_equal(0, ret_success); 

    void *fail_ret = virtual_realloc(virtual_heap, res_2, 100);
    assert_null(fail_ret);
    
    virtual_free(virtual_heap, res_4);
    
    void *ret = virtual_realloc(virtual_heap, res_2, 111);

    assert_non_null(ret);
    assert_int_equal(ret - res_1, 128);
}

static void test_info_1(){

    void *res_1 = virtual_malloc(virtual_heap, 60);
    void *res_2 = virtual_malloc(virtual_heap, 55);
    void *res_3 = virtual_malloc(virtual_heap, 49);
    void *res_4 = virtual_malloc(virtual_heap, 39);

    int fd_actual = open("self_tests/realloc_info_1.txt", O_CREAT|O_TRUNC|O_RDWR, 0644);

    if (fd_actual < 0){
        printf("open error\n");
        exit(1);
    }

    int save_fd = dup(STDOUT_FILENO);

    int redirection = dup2(fd_actual, STDOUT_FILENO);
    
    virtual_info(virtual_heap);

    virtual_free(virtual_heap, res_3);
    void *res_realloc = virtual_realloc(virtual_heap, res_2, 100); // should fail
    
    virtual_info(virtual_heap);

    virtual_free(virtual_heap, res_4);

    void *new_res = virtual_realloc(virtual_heap, res_2, 124);
    virtual_info(virtual_heap);

    close(redirection);
    close(fd_actual);

    dup2(save_fd, STDOUT_FILENO);

    char buf_expect[256];
    char buf_actual[256];

    FILE *expect = fopen("self_tests/realloc_info_1.out", "r");
    if (expect == NULL){
        printf("open expect file error\n");
        exit(1);
    }
    
    FILE *actual = fopen("self_tests/realloc_info_1.txt", "r");
    if (actual == NULL){
        printf("open actual file error\n");
        exit(1);
    }


    while (fgets(buf_expect, 256, expect) != NULL){
        fgets(buf_actual, 256, actual);

        assert_string_equal(buf_actual, buf_expect);
    }


    fclose(expect);
    fclose(actual);
}

static void test_realloc_null_ptr(){
    // should behave like malloc
    void *res = virtual_realloc(virtual_heap, NULL, 100);
    assert_non_null(res);
}

static void test_realloc_size_zero(){

    void *res_1 = virtual_malloc(virtual_heap, 150);
    void *res_2 = virtual_malloc(virtual_heap, 60);

    // res_1 != null and res_2 = null
    void *realloc_zero = virtual_realloc(virtual_heap, res_1, 0);
    assert_int_equal((long *)realloc_zero, 0);
}


int main(){

    const struct CMUnitTest tests[] = {
            cmocka_unit_test_setup_teardown(test_realloc_success_fail, setup, teardown),
            cmocka_unit_test_setup_teardown(test_realloc_null_ptr, setup, teardown),
            cmocka_unit_test_setup_teardown(test_info_1, setup, teardown),
            cmocka_unit_test_setup_teardown(test_realloc_size_zero, setup, teardown)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}