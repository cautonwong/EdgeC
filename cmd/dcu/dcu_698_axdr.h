#ifndef DCU_698_AXDR_H
#define DCU_698_AXDR_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int dcu_north_resolve_axdr(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len);
int dcu_north_accept_axdr(void *ctx, uint32_t oad, const uint8_t *axdr, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* DCU_698_AXDR_H */
