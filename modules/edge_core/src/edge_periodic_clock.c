#include "edge_periodic_clock.h"
#include <string.h>
#include <uv.h>

uint64_t edge_periodic_clock_now_us(const edge_periodic_clock_t *clk) {
    if (clk && clk->cfg.custom_clock) {
        return clk->cfg.custom_clock(clk->cfg.clock_ctx);
    }
    return uv_hrtime() / 1000ULL;
}

int edge_periodic_clock_init(edge_periodic_clock_t *clk, const edge_periodic_clock_config_t *cfg) {
    if (!clk || !cfg || cfg->interval_us == 0) return -1;
    memset(clk, 0, sizeof(*clk));
    clk->cfg = *cfg;

    uint64_t now_us = edge_periodic_clock_now_us(clk);
    if (cfg->wall_clock_align) {
        uint64_t interval = cfg->interval_us;
        uint64_t next_boundary = ((now_us / interval) + 1) * interval;
        clk->next_tick_us = next_boundary;
    } else {
        clk->next_tick_us = now_us + cfg->interval_us;
    }
    clk->ticks_count = 0;
    clk->missed_ticks = 0;
    return 0;
}

void edge_periodic_clock_reset(edge_periodic_clock_t *clk) {
    if (!clk || clk->cfg.interval_us == 0) return;
    uint64_t now_us = edge_periodic_clock_now_us(clk);
    if (clk->cfg.wall_clock_align) {
        uint64_t interval = clk->cfg.interval_us;
        clk->next_tick_us = ((now_us / interval) + 1) * interval;
    } else {
        clk->next_tick_us = now_us + clk->cfg.interval_us;
    }
}

int edge_periodic_clock_check(edge_periodic_clock_t *clk, uint64_t *out_ticks_to_fire) {
    if (!clk || clk->cfg.interval_us == 0) {
        if (out_ticks_to_fire) *out_ticks_to_fire = 0;
        return 0;
    }

    uint64_t now_us = edge_periodic_clock_now_us(clk);
    if (now_us < clk->next_tick_us) {
        if (out_ticks_to_fire) *out_ticks_to_fire = 0;
        return 0;
    }

    uint64_t elapsed = (now_us - clk->next_tick_us) / clk->cfg.interval_us + 1;
    if (elapsed == 1) {
        clk->next_tick_us += clk->cfg.interval_us;
        clk->ticks_count++;
        if (out_ticks_to_fire) *out_ticks_to_fire = 1;
        return 1;
    }

    // Overrun occurred
    uint64_t missed = elapsed - 1;
    clk->missed_ticks += missed;

    if (clk->cfg.policy == EDGE_OVERRUN_SKIP) {
        clk->next_tick_us += elapsed * clk->cfg.interval_us;
        clk->ticks_count++;
        if (out_ticks_to_fire) *out_ticks_to_fire = 1;
        return 1;
    } else if (clk->cfg.policy == EDGE_OVERRUN_CATCHUP) {
        clk->next_tick_us += elapsed * clk->cfg.interval_us;
        clk->ticks_count += elapsed;
        if (out_ticks_to_fire) *out_ticks_to_fire = elapsed;
        return (int)elapsed;
    } else { // EDGE_OVERRUN_COALESCE
        clk->next_tick_us += elapsed * clk->cfg.interval_us;
        clk->ticks_count++;
        if (out_ticks_to_fire) *out_ticks_to_fire = 1;
        return 1;
    }
}
