#ifndef __BMSPARSER_SORT_H__
#define __BMSPARSER_SORT_H__

#include <stddef.h>

typedef unsigned char (*bms_Less)(void *, void *);

void sort(void *arr, size_t n, size_t item, bms_Less less);

#endif