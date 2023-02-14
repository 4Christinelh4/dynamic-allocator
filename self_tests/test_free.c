#include "../virtual_alloc.h"
#include "../virtual_sbrk.h"
#include <unistd.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include "../cMocka/cmocka.h"

static void *virtual_heap = NULL;

void *virtual_sbrk(int32_t increment) {
    // Your implementation here (for your testing only)
    return sbrk(increment);
}

static int setup(){
    virtual_heap = virtual_sbrk(20);
    init_allocator(virtual_heap, 8, 5);
    
    return 0;
}

static int teardown(){
    return 0;
}

static void test_free_successfully(){
    void *res_1 = virtual_malloc(virtual_heap, 60);
    void *res_2 = virtual_malloc(virtual_heap, 60);
    void *res_3 = virtual_malloc(virtual_heap, 58);

    assert_non_null(res_1);
    assert_non_null(res_2);
    assert_non_null(res_3);

    int ret = virtual_free(virtual_heap, res_1);
    assert_int_equal(0, ret);
}

static void test_free_fail(){
    void *non_existing = virtual_heap + 55;

    int ret_fail = virtual_free(virtual_heap, non_existing);
    
    assert_int_equal(-1, ret_fail); 
}

static void test_free_twice(){

    void *res_1 = virtual_malloc(virtual_heap, 60);
    assert_non_null(res_1);
    
    int ret_1 = virtual_free(virtual_heap, res_1);
    assert_int_equal(0, ret_1);
    
    int ret_2 = virtual_free(virtual_heap, res_1);
    assert_int_equal(-1, ret_2); 
}

static void test_give_null_free(){

    void *null_ptr = NULL;
    int ret = virtual_free(virtual_heap, null_ptr);
    assert_int_equal(-1, ret);
}


int main(){

    const struct CMUnitTest tests[] = {
            cmocka_unit_test_setup_teardown(test_free_successfully, setup, teardown),
            cmocka_unit_test_setup_teardown(test_free_fail, setup, teardown),
            cmocka_unit_test_setup_teardown(test_free_twice, setup, teardown),
            cmocka_unit_test_setup_teardown(test_give_null_free, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}