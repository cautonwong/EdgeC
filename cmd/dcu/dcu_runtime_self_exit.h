#ifndef DCU_RUNTIME_SELF_EXIT_H
#define DCU_RUNTIME_SELF_EXIT_H

#include <stdint.h>
#include <uv.h>

/**
 * @brief 解析 DCU_SELF_EXIT_SECONDS：0 = 不启动自退定时器
 *
 * 未设、空串、0、负数、非法数字都返回 0。
 */
uint64_t dcu_runtime_self_exit_ms(const char *value);

/**
 * @brief 按 DCU_SELF_EXIT_SECONDS 装配一次性自退定时器；<=0 或非法时不启动
 */
void dcu_self_exit_start(uv_loop_t *loop, uv_timer_cb cb);

#endif /* DCU_RUNTIME_SELF_EXIT_H */
