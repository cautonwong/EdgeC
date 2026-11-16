#include "net_buf.h"

void sample(net_buf_pool_t *pool) {
    net_buf_t *buf = net_buf_alloc(pool, 0);
    (void)buf;
}
