#include "net_buf.h"

void sample(net_buf_pool_t *pool, int len) {
    net_buf_t *buf = net_buf_alloc(pool, len);
    (void)buf;
}
