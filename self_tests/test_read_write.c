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
    virtual_heap = virtual_sbrk(10);
    init_allocator(virtual_heap, 8, 5);
    return 0;
}

static int teardown(){
    return 0;
}

static void test_malloc_w_r(){
    void *res_1 = virtual_malloc(virtual_heap, 100);

    uint8_t *array = res_1;

    for (int i=0; i<100; i++){
        array[i] = i%55;
    }

    for (int i=0; i<100; i++){
        assert_int_equal(array[i], i%55);
    }

}

static void test_realloc_w_r(){

    void *res1 = virtual_malloc(virtual_heap, 51);
    void *res2 = virtual_malloc(virtual_heap, 60);

    uint8_t *array = res1;

    for (int i=0; i<51; i++){
    
        array[i] = i%40;
    }

    // printf("\n");

    void *res3 = virtual_realloc(virtual_heap, res1, 120);

    uint8_t *realloc_array = res3;

    for (int i=0; i<51; i++){
        // printf("%d ", realloc_array[i]);
        assert_int_equal(realloc_array[i], i%40);
    }
}

static void test_fail_realloc_w_r(){
    void *res1 = virtual_malloc(virtual_heap, 51);
    void *res2 = virtual_malloc(virtual_heap, 60);

    uint8_t *array = res1;

    for (int i=0; i<51; i++){
        array[i] = 26+i;
    }

    void *res3 = virtual_realloc(virtual_heap, res1, 200);

    assert_null(res3);

    for (int i=0; i<51; i++){
        assert_int_equal(array[i], 26+i);
    }
}

int main(){

    const struct CMUnitTest tests[] = {
            cmocka_unit_test_setup_teardown(test_malloc_w_r, setup, teardown),
            cmocka_unit_test_setup_teardown(test_realloc_w_r, setup, teardown),
            cmocka_unit_test_setup_teardown(test_fail_realloc_w_r, setup, teardown)

    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}