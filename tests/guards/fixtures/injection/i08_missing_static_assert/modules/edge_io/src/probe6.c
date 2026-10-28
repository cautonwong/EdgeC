#include "probe6.h"

/* #411 probe #8: storage placeholder declared but never size-checked.
 * Without a compile-time size assertion, a later field addition silently
 * overflows the caller's buffer. */
struct probe6_s {
    int id;
    double payload;
};
