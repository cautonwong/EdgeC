// VIOLATION: Directly peeks into aggregate root's private ring field
#include <stdint.h>

struct ring {
    int fd;
};
struct datahub {
    struct ring ring;
};
typedef struct datahub datahub_t;

void test_violation(datahub_t *hub) {
    hub->ring.fd = -1;  // Forbidden direct internal field dereference
}
