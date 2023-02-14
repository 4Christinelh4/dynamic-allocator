CC=gcc
CFLAGS=-fsanitize=address -Wall -Werror -std=gnu11 -g
# CFLAGS=-fsanitize=address -std=gnu11 -g
# LIBS=-L/usr/lib/x86_64-linux-gnu -lm

my_tests: tests.c virtual_alloc.c
	$(CC) $(CFLAGS) $^ -o $@ -lm

run_tests:
	bash test.sh

clean:
	rm large_malloc
	rm 1.txt
	rm my_tests

