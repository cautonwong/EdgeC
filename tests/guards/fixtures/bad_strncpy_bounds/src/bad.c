#include <string.h>

struct bad {
    char name[32];
    char topic[256];
};

void bad_copy(struct bad *b, const char *src, const char *topic) {
    strncpy(b->name, src, 31);
    strncpy(b->topic, topic, sizeof(b->topic));
    strncpy(b->name, "this literal is far longer than thirty one bytes", 31);
}
