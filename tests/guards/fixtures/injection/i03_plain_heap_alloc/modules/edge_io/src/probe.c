#include <stdlib.h>

/* #411 probe #3: plain heap allocation in an ordinary helper.
 * RT-001 only inspected *_step/_update/... functions, so this passed. */
void *probe_alloc(unsigned long n) {
    return malloc(n);
}
