#include "sort.h"
#include <stdlib.h>
#include <string.h>

static void merge(void *arr, size_t n1, size_t n2, size_t item, bms_Less less)
{
    void *mid = (char *)arr + item * n1;
    void *end = (char *)mid + item * n2;
    void *a = arr;
    void *b = mid;
    void *tmp = malloc(item * (n1 + n2));
    void *c = tmp;
    while (a < mid && b < end)
    {
        if (less(b, a))
        {
            memcpy(c, b, item);
            b = (char *)b + item;
        }
        else
        {
            memcpy(c, a, item);
            a = (char *)a + item;
        }
        c = (char *)c + item;
    }
    if (a < mid)
    {
        memcpy(c, a, mid - a);
    }
    if (b < end)
    {
        memcpy(c, b, end - b);
    }
    memcpy(arr, tmp, item * (n1 + n2));
    free(tmp);
}

void sort(void *arr, size_t n, size_t item, bms_Less less)
{
    if (n > 1)
    {
        size_t n1 = n / 2;
        size_t n2 = n - n1;
        void *a1 = arr;
        void *a2 = (char *)arr + item * n1;
        sort(a1, n1, item, less);
        sort(a2, n2, item, less);
        merge(a1, n1, n2, item, less);
    }
}