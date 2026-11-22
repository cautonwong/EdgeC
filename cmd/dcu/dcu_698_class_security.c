#include "dcu_698_object_internal.h"

#include <string.h>

#include "dcu_security.h"
#include "edge_slave_plane.h"

int dcu_698_security_f100_execute(const dcu_698_object_t *obj, uint8_t method_id,
                                  const void *param, void *result) {
    (void)param;
    (void)result;
    dcu_esam_t *esam = (dcu_esam_t *)obj->instance;
    if (!esam) return EDGE_SLAVE_SRC_NOT_FOUND;

    if (method_id == 1) {
        memset(esam->random_challenge, 0, sizeof(esam->random_challenge));
        memset(esam->session_key, 0, sizeof(esam->session_key));
        esam->is_authenticated = false;
        return 0;
    }
    return EDGE_SLAVE_SRC_CLASS_MISMATCH;
}
