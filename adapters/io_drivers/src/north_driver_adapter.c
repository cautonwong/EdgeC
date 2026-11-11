#include "north_driver_adapter.h"

#include <stdalign.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "net_buf.h"

extern int sparkplug_b_driver_register(driver_manager_t *mgr);
extern int mqtt_json_driver_register(driver_manager_t *mgr);

typedef struct {
    driver_manager_storage_t mgr_storage;
    driver_manager_t *mgr;
    edge_driver_ctx_t *driver;
    char protocol[32];
} north_driver_adapter_ctx_t;

/* The Paho PUBACK path and the northbound loop can encode concurrently. */
static _Thread_local uint8_t t_frame[NORTH_DRIVER_FRAME_BYTES];

_Static_assert(sizeof(north_driver_adapter_ctx_t) <= NORTH_DRIVER_ADAPTER_STORAGE_BYTES,
               "north driver adapter storage is too small");

static int _attach(void *ctx, const char *protocol, north_driver_t **out_driver) {
    north_driver_adapter_ctx_t *a = (north_driver_adapter_ctx_t *)ctx;
    if (!a || !protocol || !out_driver) return -1;
    if (a->driver) return -1;

    int (*reg)(driver_manager_t *) = NULL;
    if (strcmp(protocol, "SparkplugB") == 0) {
        reg = sparkplug_b_driver_register;
    } else if (strcmp(protocol, "MqttJson") == 0) {
        reg = mqtt_json_driver_register;
    } else {
        return -1;
    }

    a->mgr = driver_manager_create_in(&a->mgr_storage);
    if (!a->mgr) return -1;
    if (reg(a->mgr) != 0) {
        driver_manager_destroy(a->mgr);
        a->mgr = NULL;
        return -1;
    }
    a->driver = driver_instance_create(a->mgr, protocol, 1);
    if (!a->driver) {
        driver_manager_destroy(a->mgr);
        a->mgr = NULL;
        return -1;
    }
    a->driver->pool = net_buf_pool_new(32, 1024, 0);
    if (!a->driver->pool) {
        driver_instance_destroy(a->driver, protocol);
        driver_manager_destroy(a->mgr);
        a->driver = NULL;
        a->mgr = NULL;
        return -1;
    }
    snprintf(a->protocol, sizeof(a->protocol), "%s", protocol);
    *out_driver = (north_driver_t *)a->driver;
    return 0;
}

static void _detach(void *ctx, north_driver_t *driver) {
    north_driver_adapter_ctx_t *a = (north_driver_adapter_ctx_t *)ctx;
    if (!a || !driver) return;
    edge_driver_ctx_t *d = (edge_driver_ctx_t *)driver;
    if (d->pool) net_buf_pool_unref(d->pool);
    driver_instance_destroy(d, a->protocol);
    driver_manager_destroy(a->mgr);
    a->driver = NULL;
    a->mgr = NULL;
}

static int _copy_frame(north_driver_adapter_ctx_t *a, net_buf_t *nb,
                       const uint8_t **out_data, size_t *out_len) {
    if (!a || !nb || !out_data || !out_len) return -1;
    if (nb->b.len > sizeof(t_frame)) return -1;
    memcpy(t_frame, nb->b.data, nb->b.len);
    *out_data = t_frame;
    *out_len = nb->b.len;
    return 0;
}

static int _encode_value(void *ctx, north_driver_t *driver, uint32_t tag_id, double value,
                         const char *name, const uint8_t **out_data, size_t *out_len) {
    north_driver_adapter_ctx_t *a = (north_driver_adapter_ctx_t *)ctx;
    edge_driver_ctx_t *d = (edge_driver_ctx_t *)driver;
    if (!a || !d || !d->driver || !d->driver->req_prepare || !d->driver->req_encode) return -1;

    net_buf_t *nb = net_buf_alloc(d->pool, 0);
    if (!nb) return -1;
    alignas(max_align_t) uint8_t scratch[REQ_DRV_PRIV_MAX];
    memset(scratch, 0, sizeof(scratch));
    size_t name_len = name ? strlen(name) + 1 : 0;
    int rc = d->driver->req_prepare(scratch, 1, tag_id, NULL, 0, name, name_len, true, value);
    if (rc == 0) rc = d->driver->req_encode(scratch, d->priv, nb);
    if (rc == 0) rc = _copy_frame(a, nb, out_data, out_len);
    net_buf_unref(nb);
    return rc;
}

static int _encode_tick(void *ctx, north_driver_t *driver,
                        const edge_north_source_ops_t *north, void *north_ctx,
                        const uint8_t **out_data, size_t *out_len) {
    north_driver_adapter_ctx_t *a = (north_driver_adapter_ctx_t *)ctx;
    edge_driver_ctx_t *d = (edge_driver_ctx_t *)driver;
    if (!a || !d || !d->driver || !d->driver->req_tick) return -1;

    net_buf_t *nb = NULL;
    int rc = d->driver->req_tick(d->priv, north, north_ctx, NULL, NULL, d->pool, &nb);
    if (rc == 0 && nb) rc = _copy_frame(a, nb, out_data, out_len);
    else if (rc == 0) rc = -1;
    if (nb) net_buf_unref(nb);
    return rc;
}

static void _reset_session(void *ctx, north_driver_t *driver) {
    (void)ctx;
    edge_driver_ctx_t *d = (edge_driver_ctx_t *)driver;
    if (d && d->driver && d->driver->req_session_reset) d->driver->req_session_reset(d->priv);
}

north_driver_port_t north_driver_adapter_init(north_driver_adapter_storage_t *storage) {
    north_driver_adapter_ctx_t *a = (north_driver_adapter_ctx_t *)storage->raw;
    memset(a, 0, sizeof(*a));
    north_driver_port_t port = {
        .ctx = a,
        .attach = _attach,
        .detach = _detach,
        .encode_value = _encode_value,
        .encode_tick = _encode_tick,
        .reset_session = _reset_session,
    };
    return port;
}
