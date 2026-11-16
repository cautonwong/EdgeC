#include <stdlib.h>

struct thing {
    int value;
};

struct thing *thing_create(void) {
    struct thing *t = calloc(1, sizeof(*t));
    t->value = 1;
    return t;
}

struct thing *thing_create_two(void) {
    struct thing *t = calloc(1, sizeof(*t));
    if (!t) return NULL;
    int *scratch = malloc(sizeof(int));
    scratch[0] = 2;
    free(scratch);
    return t;
}

struct thing *thing_create_multiline(int a,
                                     int b) {
    struct thing *t = calloc(1, sizeof(*t));
    t->value = a + b;
    return t;
}
