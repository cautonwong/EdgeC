#include "battery_balancer.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

struct battery_balancer {
    battery_bal_config_t config;
    battery_balancer_ports_t ports;
    battery_bal_cell_input_t cells[BATTERY_BAL_MAX_CELLS];
    battery_bal_plan_t plan;
    
    /* 历史累计运行统计 */
    double cum_transferred_wh;
    double cum_dissipated_wh;
    uint64_t total_cycles_planned;
    double last_timestamp_s;
};

/* -------------------------------------------------------------------------- */
/* Compile-Time Size & Alignment Assertions (AEP-752)                          */
/* -------------------------------------------------------------------------- */
_Static_assert(sizeof(struct battery_balancer) <= sizeof(battery_balancer_storage_t),
               "battery_balancer_storage_t buffer too small for struct battery_balancer");
_Static_assert(_Alignof(struct battery_balancer) <= _Alignof(battery_balancer_storage_t),
               "battery_balancer_storage_t alignment mismatch for struct battery_balancer");

battery_balancer_t *battery_balancer_create_in(battery_balancer_storage_t *store,
                                               const battery_bal_config_t *cfg,
                                               uint16_t cell_count,
                                               const battery_balancer_ports_t *ports) {
    if (!store || cell_count < 2 || cell_count > BATTERY_BAL_MAX_CELLS) {
        return NULL;
    }

    battery_balancer_t *bal = (battery_balancer_t *)store->raw;
    if (battery_balancer_init(bal, cfg, cell_count) != 0) {
        return NULL;
    }

    if (ports) {
        bal->ports = *ports;
    }

    return bal;
}

const battery_bal_config_t *battery_balancer_get_config(const battery_balancer_t *bal) {
    return bal ? &bal->config : NULL;
}

uint16_t battery_balancer_get_cell_count(const battery_balancer_t *bal) {
    return bal ? bal->config.cell_count : 0;
}

double battery_balancer_get_transferred_wh(const battery_balancer_t *bal) {
    return bal ? bal->cum_transferred_wh : 0.0;
}

int battery_balancer_step_ports(battery_balancer_t *bal,
                                const uint32_t *cell_u_tags,
                                const uint32_t *cell_soc_tags,
                                const uint32_t *cell_temp_tags,
                                const uint32_t *cmd_out_tags,
                                double dt_s) {
    if (!bal || dt_s <= 0.0) return -1;

    uint64_t now_us = 0;
    if (bal->ports.clock.now_us) {
        now_us = bal->ports.clock.now_us(bal->ports.clock.ctx);
    }
    double cur_ts_s = (double)now_us / 1000000.0;

    for (uint16_t i = 0; i < bal->config.cell_count; i++) {
        double v = 3.30, soc = 0.50, temp = 25.0;
        if (bal->ports.reader.read_tag && cell_u_tags && cell_u_tags[i]) {
            bal->ports.reader.read_tag(bal->ports.reader.ctx, cell_u_tags[i], &v, NULL);
        }
        if (bal->ports.reader.read_tag && cell_soc_tags && cell_soc_tags[i]) {
            bal->ports.reader.read_tag(bal->ports.reader.ctx, cell_soc_tags[i], &soc, NULL);
        }
        if (bal->ports.reader.read_tag && cell_temp_tags && cell_temp_tags[i]) {
            bal->ports.reader.read_tag(bal->ports.reader.ctx, cell_temp_tags[i], &temp, NULL);
        }

        battery_bal_cell_input_t input = {
            .cell_id = i,
            .voltage_v = v,
            .soc = soc,
            .temperature_c = temp,
            .soh = 1.0,
            .r0_mohm = 0.50
        };
        battery_balancer_update_cell(bal, i, &input);
    }

    int rc = battery_balancer_plan_step(bal, cur_ts_s);
    if (rc != 0) return rc;

    const battery_bal_plan_t *plan = battery_balancer_get_plan(bal);
    if (plan && bal->ports.writer.write_tag && cmd_out_tags) {
        for (uint16_t i = 0; i < bal->config.cell_count; i++) {
            if (cmd_out_tags[i]) {
                bal->ports.writer.write_tag(bal->ports.writer.ctx, cmd_out_tags[i],
                                           (double)plan->commands[i].action, now_us);
            }
        }
    }

    return 0;
}

int battery_balancer_init(battery_balancer_t *bal, const battery_bal_config_t *cfg, uint16_t cell_count)
{
    if (!bal || cell_count < 2 || cell_count > BATTERY_BAL_MAX_CELLS) {
        return -1;
    }

    memset(bal, 0, sizeof(*bal));

    if (cfg) {
        bal->config = *cfg;
    } else {
        bal->config.mode = BATTERY_BAL_MODE_ACTIVE_PAIRWISE;
        bal->config.cell_count = cell_count;
        bal->config.nominal_capacity_ah = 280.0;
        bal->config.start_voltage_diff_mv = BATTERY_BAL_DEFAULT_START_MV;
        bal->config.stop_voltage_diff_mv = BATTERY_BAL_DEFAULT_STOP_MV;
        bal->config.start_soc_diff_pct = 1.50;
        bal->config.stop_soc_diff_pct = 0.50;
        bal->config.max_active_current_a = BATTERY_BAL_DEFAULT_MAX_CURR_A;
        bal->config.bleed_resistor_ohm = BATTERY_BAL_BLEED_RESISTOR_OHM;
        bal->config.max_concurrent_channels = 8;
        bal->config.adjacent_interlock = true;
        bal->config.temp_min_c = 5.0;
        bal->config.temp_max_c = 45.0;
        bal->config.cell_min_volt_v = 2.80;
        bal->config.cell_max_volt_v = 3.65;
    }

    bal->config.cell_count = cell_count;

    /* 校验与修正参数 */
    if (bal->config.nominal_capacity_ah <= 0.0 || !isfinite(bal->config.nominal_capacity_ah)) {
        bal->config.nominal_capacity_ah = 280.0;
    }
    if (bal->config.start_voltage_diff_mv <= bal->config.stop_voltage_diff_mv || !isfinite(bal->config.start_voltage_diff_mv)) {
        bal->config.start_voltage_diff_mv = bal->config.stop_voltage_diff_mv + 5.0;
    }
    if (bal->config.start_soc_diff_pct <= bal->config.stop_soc_diff_pct || bal->config.start_soc_diff_pct <= 0.01 || !isfinite(bal->config.start_soc_diff_pct)) {
        bal->config.start_soc_diff_pct = bal->config.stop_soc_diff_pct + 0.50;
        if (bal->config.start_soc_diff_pct <= 0.0) {
            bal->config.start_soc_diff_pct = 1.50;
        }
    }
    if (bal->config.max_active_current_a <= 0.0 || !isfinite(bal->config.max_active_current_a)) {
        bal->config.max_active_current_a = BATTERY_BAL_DEFAULT_MAX_CURR_A;
    }
    if (bal->config.bleed_resistor_ohm <= 0.0 || !isfinite(bal->config.bleed_resistor_ohm)) {
        bal->config.bleed_resistor_ohm = BATTERY_BAL_BLEED_RESISTOR_OHM;
    }
    if (bal->config.max_concurrent_channels == 0) {
        bal->config.max_concurrent_channels = 4;
    }

    for (uint16_t i = 0; i < cell_count; i++) {
        bal->cells[i].cell_id = i;
        bal->cells[i].voltage_v = 3.30;
        bal->cells[i].soc = 0.50;
        bal->cells[i].temperature_c = 25.0;
        bal->cells[i].soh = 1.0;
        bal->cells[i].r0_mohm = 0.50;
    }

    battery_balancer_reset(bal);
    return 0;
}

int battery_balancer_update_cell(battery_balancer_t *bal, uint16_t cell_id, const battery_bal_cell_input_t *input)
{
    if (!bal || !input || cell_id >= bal->config.cell_count) {
        return -1;
    }

    bal->cells[cell_id] = *input;
    bal->cells[cell_id].cell_id = cell_id;
    return 0;
}

/* 辅助结构体用于排序 */
typedef struct {
    uint16_t cell_id;
    double soc;
    double voltage_v;
    double score;
} cell_sort_item_t;

static int compare_soc_desc(const void *a, const void *b)
{
    const cell_sort_item_t *ca = (const cell_sort_item_t *)a;
    const cell_sort_item_t *cb = (const cell_sort_item_t *)b;
    bool a_fin = isfinite(ca->soc);
    bool b_fin = isfinite(cb->soc);
    if (!a_fin && !b_fin) return (ca->cell_id > cb->cell_id) - (ca->cell_id < cb->cell_id);
    if (!a_fin) return 1; /* non-finite sinks to bottom */
    if (!b_fin) return -1;
    if (cb->soc > ca->soc) return 1;
    if (cb->soc < ca->soc) return -1;
    return (ca->cell_id > cb->cell_id) - (ca->cell_id < cb->cell_id);
}

static int compare_soc_asc(const void *a, const void *b)
{
    const cell_sort_item_t *ca = (const cell_sort_item_t *)a;
    const cell_sort_item_t *cb = (const cell_sort_item_t *)b;
    bool a_fin = isfinite(ca->soc);
    bool b_fin = isfinite(cb->soc);
    if (!a_fin && !b_fin) return (ca->cell_id > cb->cell_id) - (ca->cell_id < cb->cell_id);
    if (!a_fin) return 1; /* non-finite sinks to bottom */
    if (!b_fin) return -1;
    if (ca->soc > cb->soc) return 1;
    if (ca->soc < cb->soc) return -1;
    return (ca->cell_id > cb->cell_id) - (ca->cell_id < cb->cell_id);
}

int battery_balancer_plan_step(battery_balancer_t *bal, double cur_timestamp_s)
{
    if (!bal || bal->config.mode == BATTERY_BAL_MODE_DISABLED) {
        if (bal) {
            memset(&bal->plan, 0, sizeof(bal->plan));
        }
        return 0;
    }

    uint16_t n = bal->config.cell_count;
    if (n < 2) {
        return -1;
    }

    double dt = 1.0;
    if (bal->last_timestamp_s > 0.0) {
        dt = cur_timestamp_s - bal->last_timestamp_s;
        if (dt < 0.0 || !isfinite(dt)) dt = 0.0;
        if (dt > 60.0) dt = 1.0;
    }
    bal->last_timestamp_s = cur_timestamp_s;
    bal->total_cycles_planned++;

    memset(&bal->plan, 0, sizeof(bal->plan));

    /* 初始化所有通道为 IDLE */
    bool safe_to_balance[BATTERY_BAL_MAX_CELLS];
    for (uint16_t i = 0; i < n; i++) {
        bal->plan.commands[i].cell_id = i;
        bal->plan.commands[i].action = BATTERY_BAL_ACTION_IDLE;
        bal->plan.commands[i].interlocked = false;
        safe_to_balance[i] = true;
    }

    /* 1. 统计整组极值、均值与离散度，拦截 NaN / Inf 脏数据 */
    double v_min = 1e9, v_max = -1e9, v_sum = 0.0;
    double soc_min = 1e9, soc_max = -1e9, soc_sum = 0.0;
    uint16_t valid_cells = 0;

    for (uint16_t i = 0; i < n; i++) {
        double v = bal->cells[i].voltage_v;
        double s = bal->cells[i].soc;
        double t = bal->cells[i].temperature_c;
        double soh = bal->cells[i].soh;

        if (!isfinite(v) || !isfinite(s) || !isfinite(t) || !isfinite(soh) ||
            v < bal->config.cell_min_volt_v || v > bal->config.cell_max_volt_v ||
            t < bal->config.temp_min_c || t > bal->config.temp_max_c) {
            safe_to_balance[i] = false;
            bal->plan.commands[i].interlocked = true;
            continue;
        }

        if (v < v_min) v_min = v;
        if (v > v_max) v_max = v;
        v_sum += v;

        if (s < soc_min) soc_min = s;
        if (s > soc_max) soc_max = s;
        soc_sum += s;
        valid_cells++;
    }

    double v_mean = (valid_cells > 0) ? (v_sum / (double)valid_cells) : 3.30;
    double soc_mean = (valid_cells > 0) ? (soc_sum / (double)valid_cells) : 0.50;
    double v_diff_mv = (valid_cells >= 2) ? ((v_max - v_min) * 1000.0) : 0.0;
    double soc_diff_pct = (valid_cells >= 2) ? ((soc_max - soc_min) * 100.0) : 0.0;

    if (v_min > 1e8) v_min = 0.0;
    if (v_max < -1e8) v_max = 0.0;

    bal->plan.max_voltage_v = v_max;
    bal->plan.min_voltage_v = v_min;
    bal->plan.voltage_diff_mv = v_diff_mv;
    bal->plan.avg_soc = soc_mean;
    bal->plan.max_soc_diff_pct = soc_diff_pct;

    /* 2. 判断是否满足启动门限 (满足任意电压差或 SOC 极差) */
    bool trigger = (v_diff_mv >= bal->config.start_voltage_diff_mv) ||
                   (soc_diff_pct >= bal->config.start_soc_diff_pct);

    if ((!trigger && v_diff_mv <= bal->config.stop_voltage_diff_mv &&
         soc_diff_pct <= bal->config.stop_soc_diff_pct) || valid_cells < 2) {
        bal->plan.is_balancing_active = false;
        bal->plan.energy_efficiency_score = 1.0;
        bal->plan.valid = true;
        return 0;
    }

    bal->plan.is_balancing_active = true;

    double total_power_transferred = 0.0;
    double total_power_dissipated = 0.0;
    double total_charge_diff_ah = 0.0;
    uint8_t active_ch_count = 0;
    uint8_t active_pairs = 0;

    /* 4. 模式 A: 被动阻性放电均衡规划 */
    if (bal->config.mode == BATTERY_BAL_MODE_PASSIVE) {
        for (uint16_t i = 0; i < n; i++) {
            if (!safe_to_balance[i]) continue;
            if (active_ch_count >= bal->config.max_concurrent_channels) break;

            double diff_v = bal->cells[i].voltage_v - (v_mean + bal->config.stop_voltage_diff_mv * 1e-3);
            if (diff_v > 0.0) {
                /* 硬件相邻互锁检查 */
                if (bal->config.adjacent_interlock && i > 0 &&
                    bal->plan.commands[i - 1].action == BATTERY_BAL_ACTION_DISCHARGE_PASSIVE) {
                    bal->plan.commands[i].interlocked = true;
                    continue;
                }

                double i_bleed = bal->cells[i].voltage_v / bal->config.bleed_resistor_ohm;
                double p_diss = i_bleed * bal->cells[i].voltage_v;
                double duty = diff_v / (bal->config.start_voltage_diff_mv * 1e-3);
                if (duty > 1.0) duty = 1.0;
                if (duty < 0.1) duty = 0.1;

                bal->plan.commands[i].action = BATTERY_BAL_ACTION_DISCHARGE_PASSIVE;
                bal->plan.commands[i].duty_cycle = duty;
                bal->plan.commands[i].target_current_a = -i_bleed * duty;
                bal->plan.commands[i].power_dissipated_w = p_diss * duty;

                total_power_dissipated += p_diss * duty;
                total_charge_diff_ah += (bal->cells[i].soc - soc_mean) * bal->config.nominal_capacity_ah;
                active_ch_count++;
            }
        }
    }
    /* 5. 模式 B/C: 双向主动 / 混合最优能效配对均衡规划 */
    else if (bal->config.mode == BATTERY_BAL_MODE_ACTIVE_PAIRWISE ||
             bal->config.mode == BATTERY_BAL_MODE_HYBRID_OPTIMAL) {
        cell_sort_item_t high_cells[BATTERY_BAL_MAX_CELLS];
        cell_sort_item_t low_cells[BATTERY_BAL_MAX_CELLS];
        uint16_t high_count = 0, low_count = 0;

        for (uint16_t i = 0; i < n; i++) {
            if (!safe_to_balance[i]) continue;
            if (bal->cells[i].soc > soc_mean + (bal->config.stop_soc_diff_pct * 0.01 * 0.5)) {
                high_cells[high_count].cell_id = i;
                high_cells[high_count].soc = bal->cells[i].soc;
                high_cells[high_count].voltage_v = bal->cells[i].voltage_v;
                high_count++;
            } else if (bal->cells[i].soc < soc_mean - (bal->config.stop_soc_diff_pct * 0.01 * 0.5)) {
                low_cells[low_count].cell_id = i;
                low_cells[low_count].soc = bal->cells[i].soc;
                low_cells[low_count].voltage_v = bal->cells[i].voltage_v;
                low_count++;
            }
        }

        qsort(high_cells, high_count, sizeof(cell_sort_item_t), compare_soc_desc);
        qsort(low_cells, low_count, sizeof(cell_sort_item_t), compare_soc_asc);

        uint16_t max_pairs = (high_count < low_count) ? high_count : low_count;
        if (max_pairs > (bal->config.max_concurrent_channels / 2)) {
            max_pairs = (bal->config.max_concurrent_channels / 2);
        }

        for (uint16_t p = 0; p < max_pairs; p++) {
            uint16_t src_id = high_cells[p].cell_id;
            uint16_t dst_id = low_cells[p].cell_id;

            /* 检查相邻通道硬件互锁 */
            if (bal->config.adjacent_interlock && abs((int)src_id - (int)dst_id) == 1) {
                continue;
            }

            double delta_soc = high_cells[p].soc - low_cells[p].soc;
            double soc_scale = bal->config.start_soc_diff_pct * 0.01;
            if (soc_scale <= 1e-4 || !isfinite(soc_scale)) soc_scale = 0.015;
            double i_transfer = bal->config.max_active_current_a * (delta_soc / soc_scale);
            if (i_transfer > bal->config.max_active_current_a) {
                i_transfer = bal->config.max_active_current_a;
            }
            if (i_transfer < 0.20) {
                i_transfer = 0.20;
            }

            /* SOH 寿命权重调制 (对老化电芯限制大电流冲击) */
            double soh_weight = (bal->cells[src_id].soh + bal->cells[dst_id].soh) * 0.5;
            if (soh_weight < 0.85) {
                i_transfer *= 0.70;
            }

            /* 能效计算 (考虑 DC-DC 转换器拓扑 90% 基础能效与内阻发热) */
            double r_total_ohm = (bal->cells[src_id].r0_mohm + bal->cells[dst_id].r0_mohm) * 1e-3;
            double eff = 0.92 - (r_total_ohm * 10.0);
            if (eff < 0.75) eff = 0.75;
            if (eff > 0.95) eff = 0.95;

            double p_src = i_transfer * bal->cells[src_id].voltage_v;
            double p_dst = p_src * eff;
            double p_loss = p_src - p_dst + (i_transfer * i_transfer * r_total_ohm);

            /* 设置控制命令 */
            bal->plan.commands[src_id].action = BATTERY_BAL_ACTION_ACTIVE_SOURCE;
            bal->plan.commands[src_id].target_current_a = -i_transfer;
            bal->plan.commands[src_id].duty_cycle = 1.0;
            bal->plan.commands[src_id].power_dissipated_w = p_loss * 0.5;

            bal->plan.commands[dst_id].action = BATTERY_BAL_ACTION_ACTIVE_TARGET;
            bal->plan.commands[dst_id].target_current_a = i_transfer * eff;
            bal->plan.commands[dst_id].duty_cycle = 1.0;
            bal->plan.commands[dst_id].power_dissipated_w = p_loss * 0.5;

            /* 记录配对信息 */
            bal->plan.pairs[active_pairs].source_cell_id = src_id;
            bal->plan.pairs[active_pairs].target_cell_id = dst_id;
            bal->plan.pairs[active_pairs].transfer_current_a = i_transfer;
            bal->plan.pairs[active_pairs].transfer_power_w = p_src;
            bal->plan.pairs[active_pairs].efficiency = eff;

            total_power_transferred += p_dst;
            total_power_dissipated += p_loss;
            total_charge_diff_ah += delta_soc * bal->config.nominal_capacity_ah;

            active_pairs++;
            active_ch_count += 2;
        }
    }

    /* 6. 估算平衡所需时间与能效综合得分 */
    double effective_current = 0.0;
    for (uint16_t i = 0; i < n; i++) {
        if (bal->plan.commands[i].action != BATTERY_BAL_ACTION_IDLE) {
            effective_current += fabs(bal->plan.commands[i].target_current_a);
        }
    }

    double time_to_bal_s = 0.0;
    if (effective_current > 0.05 && total_charge_diff_ah > 0.0) {
        time_to_bal_s = (total_charge_diff_ah / effective_current) * 3600.0;
    }

    /* 累计能量积分 */
    double energy_transferred_wh = total_power_transferred * (dt / 3600.0);
    double energy_dissipated_wh = total_power_dissipated * (dt / 3600.0);
    bal->cum_transferred_wh += energy_transferred_wh;
    bal->cum_dissipated_wh += energy_dissipated_wh;

    double eff_score = 1.0;
    if (total_power_transferred + total_power_dissipated > 1e-3) {
        eff_score = total_power_transferred / (total_power_transferred + total_power_dissipated);
    } else if (bal->config.mode == BATTERY_BAL_MODE_PASSIVE) {
        eff_score = 0.20; /* 被动放电全为发热损耗 */
    }

    bal->plan.active_channels_count = active_ch_count;
    bal->plan.active_pairs_count = active_pairs;
    bal->plan.estimated_time_to_balance_s = time_to_bal_s;
    bal->plan.total_energy_transferred_wh = bal->cum_transferred_wh;
    bal->plan.total_energy_dissipated_wh = bal->cum_dissipated_wh;
    bal->plan.energy_efficiency_score = eff_score;
    bal->plan.valid = true;

    return 0;
}

const battery_bal_plan_t *battery_balancer_get_plan(const battery_balancer_t *bal)
{
    if (!bal || !bal->plan.valid) {
        return NULL;
    }
    return &bal->plan;
}

void battery_balancer_reset(battery_balancer_t *bal)
{
    if (!bal) {
        return;
    }

    memset(&bal->plan, 0, sizeof(bal->plan));
    bal->cum_transferred_wh = 0.0;
    bal->cum_dissipated_wh = 0.0;
    bal->total_cycles_planned = 0;
    bal->last_timestamp_s = 0.0;
}
