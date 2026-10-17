#include "hal_crypto.h"

static const crypto_ops_t *s_active_crypto_ops = NULL;

void hal_crypto_register(const crypto_ops_t *ops) {
    s_active_crypto_ops = ops;
}

const crypto_ops_t *hal_crypto_get_ops(void) {
    return s_active_crypto_ops;
}
