#ifndef DCU_CLOCK_PORT_H
#define DCU_CLOCK_PORT_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int (*now)(void *ctx, struct tm *out);
    int (*set)(void *ctx, const struct tm *in);
    void *ctx;
} dcu_clock_port_t;

/** 系统时钟实现：now 读系统时间，set 用 clock_settime（需要权限）。 */
const dcu_clock_port_t *dcu_clock_port_system(void);

#ifdef __cplusplus
}
#endif

#endif /* DCU_CLOCK_PORT_H */
