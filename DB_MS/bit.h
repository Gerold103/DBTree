#ifndef _BIT_H
#define _BIT_H
#include <stdbool.h>
#include <stdio.h>

bool bit_test(const void *data, size_t pos);

bool bit_set(void *data, size_t pos);

#endif
