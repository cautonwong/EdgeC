#include <string.h>

struct safe {
    char name[32];
};

void safe_copy(struct safe *s, const char *src) {
    strncpy(s->name, src, sizeof(s->name) - 1);
    s->name[sizeof(s->name) - 1] = '\0';
    strncpy(s->name, "short", 31);
}
