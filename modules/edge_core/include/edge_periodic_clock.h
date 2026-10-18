#ifndef EDGE_PERIODIC_CLOCK_H
#define EDGE_PERIODIC_CLOCK_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EDGE_OVERRUN_SKIP = 0,     /** 漏拍丢弃：直接跳到未来下一个最近的对齐周期，仅触发一次当前拍 */
    EDGE_OVERRUN_CATCHUP = 1,  /** 漏拍追赶：补发错过的所有周期，保证周期总数确定性 */
    EDGE_OVERRUN_COALESCE = 2  /** 漏拍合并：错过的多个周期合并为一次触发，但基线保持格点对齐 */
} edge_overrun_policy_t;

typedef uint64_t (*edge_clock_fn)(void *ctx);

typedef struct {
    uint64_t interval_us;           /** 周期时间 (微秒, 1s = 1000000us) */
    edge_overrun_policy_t policy;   /** 漏拍超限处理策略 */
    bool wall_clock_align;          /** 是否按自然格点/墙钟对齐 (消除相位漂移) */
    edge_clock_fn custom_clock;     /** 自定义单调时钟源 (为 NULL 则使用 uv_hrtime) */
    void *clock_ctx;                /** 自定义时钟源上下文 */
} edge_periodic_clock_config_t;

typedef struct {
    edge_periodic_clock_config_t cfg;
    uint64_t next_tick_us;          /** 下一次预定触发的单调时戳 (微秒) */
    uint64_t ticks_count;           /** 累计成功触发的 tick 周期总数 */
    uint64_t missed_ticks;          /** 累计检测到的漏拍总数 */
} edge_periodic_clock_t;

int edge_periodic_clock_init(edge_periodic_clock_t *clk, const edge_periodic_clock_config_t *cfg);
int edge_periodic_clock_check(edge_periodic_clock_t *clk, uint64_t *out_ticks_to_fire);
uint64_t edge_periodic_clock_now_us(const edge_periodic_clock_t *clk);
void edge_periodic_clock_reset(edge_periodic_clock_t *clk);

#ifdef __cplusplus
}
#endif

#endif // EDGE_PERIODIC_CLOCK_H
