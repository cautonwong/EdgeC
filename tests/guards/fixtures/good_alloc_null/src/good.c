#include <stdlib.h>

struct thing {
    int value;
};

struct thing *thing_create(void) {
    struct thing *t = calloc(1, sizeof(*t));
    if (!t) return NULL;
    int *scratch = malloc(sizeof(int));
    if (!scratch) {
        free(t);
        return NULL;
    }
    free(scratch);
    t->value = 1;
    return t;
}

struct thing *thing_create_multiline(int a,
                                     int b) {
    struct thing *t = calloc(1, sizeof(*t));
    if (!t) return NULL;
    t->value = a + b;
    return t;
}
