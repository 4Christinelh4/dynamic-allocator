#!/bin/bash
# LIBS=-L/usr/lib/x86_64-linux-gnu -lm
for test in self_tests/*.c; do
  name=$(basename $test .c)
  gcc -L"cMocka" self_tests/$name.c virtual_alloc.c -g -lm -o actual -lcmocka-static
  ./actual
done
rm actual