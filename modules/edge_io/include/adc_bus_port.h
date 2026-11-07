#ifndef ADC_BUS_PORT_H
#define ADC_BUS_PORT_H

#include <stddef.h>
#include <stdint.h>

#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *ctx;
    int (*publish)(void *ctx, const pipe_msg_t *msg);
    int (*register_ref)(void *ctx, uint32_t tag_id, int fd, size_t len);
} adc_bus_port_t;

static inline int adc_bus_publish(const adc_bus_port_t *bus, const pipe_msg_t *msg) {
    return (bus && bus->publish) ? bus->publish(bus->ctx, msg) : -1;
}

static inline int adc_bus_register_ref(const adc_bus_port_t *bus, uint32_t tag_id, int fd,
                                       size_t len) {
    return (bus && bus->register_ref) ? bus->register_ref(bus->ctx, tag_id, fd, len) : -1;
}

#ifdef __cplusplus
}
#endif

#endif /* ADC_BUS_PORT_H */
