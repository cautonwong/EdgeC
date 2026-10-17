// Compliant aggregate usage: accesses aggregate root exclusively via public API
#include <stdint.h>
typedef struct datahub datahub_t;
extern int datahub_write_ring(datahub_t *hub, const void *msg);

void test_publish(datahub_t *hub) {
    uint8_t payload = 42;
    datahub_write_ring(hub, &payload);
}
