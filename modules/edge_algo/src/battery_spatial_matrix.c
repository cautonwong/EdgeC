/**
 * @file battery_spatial_matrix.c
 * @brief 4096-Node Cell Voltage-Temperature-Current Sliding Window Polarization Feature
 *        Extractor & Thermal Runaway Spatial Diffusion Predictor Implementation (AEP-733)
 * @author EdgeOS Core Architecture Team
 */

#include "battery_spatial_matrix.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define EPSILON_FLOAT 1e-6f

static inline uint64_t get_time_monotonic_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

static inline float ocv_lookup(float soc) {
    if (soc <= 0.0f) return 2.80f;
    if (soc >= 1.0f) return 3.65f;
    // Typical LiFePO4 3.2V nominal OCV characteristic curve
    if (soc < 0.1f) {
        return 2.80f + (soc / 0.1f) * (3.15f - 2.80f);
    } else if (soc < 0.9f) {
        return 3.15f + ((soc - 0.1f) / 0.8f) * (3.35f - 3.15f);
    } else {
        return 3.35f + ((soc - 0.9f) / 0.1f) * (3.65f - 3.35f);
    }
}

static inline float soc_from_ocv(float ocv) {
    if (ocv <= 2.80f) return 0.0f;
    if (ocv >= 3.65f) return 1.0f;
    if (ocv < 3.15f) {
        return (ocv - 2.80f) / (3.15f - 2.80f) * 0.1f;
    } else if (ocv < 3.35f) {
        return 0.1f + (ocv - 3.15f) / (3.35f - 3.15f) * 0.8f;
    } else {
        return 0.9f + (ocv - 3.35f) / (3.65f - 3.35f) * 0.1f;
    }
}

static inline bool is_valid_sensor_val(float val) {
    return !isnan(val) && !isinf(val) && (val > -100.0f) && (val < 1000.0f);
}

battery_spatial_matrix_t *battery_spatial_matrix_create(const battery_spatial_matrix_config_t *config) {
    if (!config) {
        return NULL;
    }
    if (config->num_cells == 0 || config->num_cells > BATTERY_SPATIAL_MAX_CELLS) {
        return NULL;
    }

    battery_spatial_matrix_t *ctx = (battery_spatial_matrix_t *)calloc(1, sizeof(battery_spatial_matrix_t));
    if (!ctx) {
        return NULL;
    }

    int rc = battery_spatial_matrix_init(ctx, config);
    if (rc != 0) {
        free(ctx);
        return NULL;
    }

    return ctx;
}

void battery_spatial_matrix_destroy(battery_spatial_matrix_t *ctx) {
    if (ctx) {
        free(ctx);
    }
}

int battery_spatial_matrix_init(battery_spatial_matrix_t *ctx, const battery_spatial_matrix_config_t *config) {
    if (!ctx || !config) {
        return -1;
    }
    if (config->num_cells == 0 || config->num_cells > BATTERY_SPATIAL_MAX_CELLS) {
        return -2;
    }

    memset(ctx, 0, sizeof(battery_spatial_matrix_t));
    memcpy(&ctx->config, config, sizeof(battery_spatial_matrix_config_t));

    // Fill defaults for zeroed parameters
    if (ctx->config.nominal_capacity_ah <= EPSILON_FLOAT) {
        ctx->config.nominal_capacity_ah = 280.0f;
    }
    if (ctx->config.internal_r0_mohm <= EPSILON_FLOAT) {
        ctx->config.internal_r0_mohm = 0.25f;
    }
    if (ctx->config.cell_dx <= EPSILON_FLOAT) {
        ctx->config.cell_dx = 0.05f; // 50mm cell spacing
    }
    if (ctx->config.cell_dy <= EPSILON_FLOAT) {
        ctx->config.cell_dy = 0.25f; // 250mm module spacing
    }
    if (ctx->config.cell_dz <= EPSILON_FLOAT) {
        ctx->config.cell_dz = 0.80f; // 800mm rack spacing
    }

    if (ctx->config.temp_attention_thresh <= EPSILON_FLOAT) {
        ctx->config.temp_attention_thresh = 45.0f;
    }
    if (ctx->config.temp_self_heating_thresh <= EPSILON_FLOAT) {
        ctx->config.temp_self_heating_thresh = 55.0f;
    }
    if (ctx->config.temp_acceleration_thresh <= EPSILON_FLOAT) {
        ctx->config.temp_acceleration_thresh = 70.0f;
    }
    if (ctx->config.temp_explosive_thresh <= EPSILON_FLOAT) {
        ctx->config.temp_explosive_thresh = 90.0f;
    }

    if (ctx->config.dtdt_attention_thresh <= EPSILON_FLOAT) {
        ctx->config.dtdt_attention_thresh = 0.2f;
    }
    if (ctx->config.dtdt_self_heating_thresh <= EPSILON_FLOAT) {
        ctx->config.dtdt_self_heating_thresh = 0.5f;
    }
    if (ctx->config.dtdt_acceleration_thresh <= EPSILON_FLOAT) {
        ctx->config.dtdt_acceleration_thresh = 1.0f;
    }
    if (ctx->config.dtdt_explosive_thresh <= EPSILON_FLOAT) {
        ctx->config.dtdt_explosive_thresh = 2.0f;
    }

    if (ctx->config.z_score_outlier_thresh <= EPSILON_FLOAT) {
        ctx->config.z_score_outlier_thresh = 3.0f;
    }

    // Determine 3D topology structure
    uint16_t num_cells_per_mod = ctx->config.num_cells_per_module;
    uint16_t num_mods_per_rack = ctx->config.num_modules_per_rack;
    uint16_t num_racks = ctx->config.num_racks;

    if (num_cells_per_mod == 0) {
        num_cells_per_mod = (ctx->config.num_cells <= 16) ? (uint16_t)ctx->config.num_cells : 16;
        ctx->config.num_cells_per_module = num_cells_per_mod;
    }
    if (num_mods_per_rack == 0) {
        uint32_t total_mods = (ctx->config.num_cells + num_cells_per_mod - 1) / num_cells_per_mod;
        num_mods_per_rack = (total_mods <= 16) ? (uint16_t)total_mods : 16;
        ctx->config.num_modules_per_rack = num_mods_per_rack;
    }
    if (num_racks == 0) {
        uint32_t cells_per_rack = (uint32_t)num_cells_per_mod * num_mods_per_rack;
        num_racks = (uint16_t)((ctx->config.num_cells + cells_per_rack - 1) / cells_per_rack);
        if (num_racks == 0) num_racks = 1;
        ctx->config.num_racks = num_racks;
    }

    // Precalculate default 3D coordinates and neighbor connectivity lattice
    for (uint32_t i = 0; i < ctx->config.num_cells; i++) {
        uint16_t c = (uint16_t)(i % num_cells_per_mod);
        uint32_t rem = i / num_cells_per_mod;
        uint16_t m = (uint16_t)(rem % num_mods_per_rack);
        uint16_t r = (uint16_t)(rem / num_mods_per_rack);

        ctx->coords[i].rack_id = r;
        ctx->coords[i].module_id = m;
        ctx->coords[i].cell_id_in_module = c;
        ctx->coords[i].pos_x = (float)c * ctx->config.cell_dx;
        ctx->coords[i].pos_y = (float)m * ctx->config.cell_dy;
        ctx->coords[i].pos_z = (float)r * ctx->config.cell_dz;

        // Initialize neighbor table to -1 (no neighbor / lattice boundary)
        for (int k = 0; k < 6; k++) {
            ctx->neighbor_indices[i][k] = -1;
        }

        // -X Neighbor (c - 1)
        if (c > 0) {
            ctx->neighbor_indices[i][0] = (int32_t)(i - 1);
        }
        // +X Neighbor (c + 1)
        if (c + 1 < num_cells_per_mod && (i + 1) < ctx->config.num_cells) {
            ctx->neighbor_indices[i][1] = (int32_t)(i + 1);
        }
        // -Y Neighbor (m - 1)
        if (m > 0 && i >= num_cells_per_mod) {
            ctx->neighbor_indices[i][2] = (int32_t)(i - num_cells_per_mod);
        }
        // +Y Neighbor (m + 1)
        if (m + 1 < num_mods_per_rack && (i + num_cells_per_mod) < ctx->config.num_cells) {
            ctx->neighbor_indices[i][3] = (int32_t)(i + num_cells_per_mod);
        }
        // -Z Neighbor (r - 1)
        uint32_t rack_stride = (uint32_t)num_cells_per_mod * num_mods_per_rack;
        if (r > 0 && i >= rack_stride) {
            ctx->neighbor_indices[i][4] = (int32_t)(i - rack_stride);
        }
        // +Z Neighbor (r + 1)
        if (r + 1 < num_racks && (i + rack_stride) < ctx->config.num_cells) {
            ctx->neighbor_indices[i][5] = (int32_t)(i + rack_stride);
        }
    }

    return 0;
}

int battery_spatial_matrix_set_coord(battery_spatial_matrix_t *ctx, uint32_t cell_idx, const battery_cell_coord_t *coord) {
    if (!ctx || !coord || cell_idx >= ctx->config.num_cells) {
        return -1;
    }
    memcpy(&ctx->coords[cell_idx], coord, sizeof(battery_cell_coord_t));
    return 0;
}

int battery_spatial_matrix_reset(battery_spatial_matrix_t *ctx) {
    if (!ctx) return -1;
    memset(ctx->windows, 0, sizeof(ctx->windows));
    memset(ctx->features, 0, sizeof(ctx->features));
    memset(ctx->raw_v, 0, sizeof(ctx->raw_v));
    memset(ctx->raw_t, 0, sizeof(ctx->raw_t));
    memset(ctx->raw_i, 0, sizeof(ctx->raw_i));
    return 0;
}

int battery_spatial_matrix_get_cell_features(const battery_spatial_matrix_t *ctx, uint32_t cell_idx, battery_cell_features_t *out_features) {
    if (!ctx || !out_features || cell_idx >= ctx->config.num_cells) {
        return -1;
    }
    memcpy(out_features, &ctx->features[cell_idx], sizeof(battery_cell_features_t));
    return 0;
}

int battery_spatial_matrix_process_frame(battery_spatial_matrix_t *ctx,
                                        const float *voltages,
                                        const float *temperatures,
                                        const float *currents,
                                        uint32_t num_cells,
                                        uint64_t timestamp_ms,
                                        battery_spatial_frame_result_t *out_result) {
    if (!ctx || !voltages || !temperatures || !out_result) {
        return -1;
    }
    if (num_cells != ctx->config.num_cells) {
        return -2;
    }

    uint64_t t_start = get_time_monotonic_us();
    memset(out_result, 0, sizeof(battery_spatial_frame_result_t));
    out_result->total_cells = num_cells;
    out_result->timestamp_ms = timestamp_ms;
    out_result->overall_tr_stage = BATTERY_TR_STAGE_NORMAL;

    float r0 = ctx->config.internal_r0_mohm * 0.001f; // convert mOhm to Ohm

    // First Pass: Copy to flat contiguous arrays, update sliding windows, calculate time differentials
    uint32_t valid_v_count = 0;
    uint32_t valid_t_count = 0;
    double v_sum = 0.0;
    double t_sum = 0.0;
    float v_min = 1e9f, v_max = -1e9f;
    float t_min = 1e9f, t_max = -1e9f;
    uint32_t v_min_idx = 0, v_max_idx = 0;
    uint32_t t_min_idx = 0, t_max_idx = 0;
    float max_dtdt = -1e9f;
    uint32_t max_dtdt_idx = 0;
    float max_d2vdt2 = 0.0f;
    uint32_t max_d2vdt2_idx = 0;

    for (uint32_t i = 0; i < num_cells; i++) {
        float v = voltages[i];
        float t = temperatures[i];
        float curr = currents ? currents[i] : 0.0f;

        bool v_valid = is_valid_sensor_val(v);
        bool t_valid = is_valid_sensor_val(t);
        bool i_valid = is_valid_sensor_val(curr);
        if (!i_valid) curr = 0.0f;

        ctx->raw_v[i] = v_valid ? v : 0.0f;
        ctx->raw_t[i] = t_valid ? t : 0.0f;
        ctx->raw_i[i] = curr;

        // Statistics accumulation
        if (v_valid) {
            v_sum += (double)v;
            valid_v_count++;
            if (v < v_min) { v_min = v; v_min_idx = i; }
            if (v > v_max) { v_max = v; v_max_idx = i; }
        }
        if (t_valid) {
            t_sum += (double)t;
            valid_t_count++;
            if (t < t_min) { t_min = t; t_min_idx = i; }
            if (t > t_max) { t_max = t; t_max_idx = i; }
        }

        // Update sliding window ringbuffer
        battery_cell_window_t *w = &ctx->windows[i];
        uint8_t prev_head = w->head;
        uint8_t head = (uint8_t)((w->head + 1) % BATTERY_SPATIAL_WINDOW_LEN);
        w->head = head;
        w->v_history[head] = v_valid ? v : 0.0f;
        w->t_history[head] = t_valid ? t : 0.0f;
        w->i_history[head] = curr;
        w->ts_history[head] = timestamp_ms;
        if (w->count < BATTERY_SPATIAL_WINDOW_LEN) {
            w->count++;
        }

        // Differential calculation
        float dv_dt = 0.0f;
        float dt_dt = 0.0f;
        float d2v_dt2 = 0.0f;

        if (w->count >= 2 && v_valid && t_valid) {
            uint64_t ts_curr = timestamp_ms;
            uint64_t ts_prev = w->ts_history[prev_head];
            float dt_sec = (float)(ts_curr - ts_prev) * 0.001f;

            if (dt_sec > 0.0001f) {
                float v_prev = w->v_history[prev_head];
                float t_prev = w->t_history[prev_head];
                if (is_valid_sensor_val(v_prev) && is_valid_sensor_val(t_prev)) {
                    dv_dt = (v - v_prev) / dt_sec;
                    dt_dt = (t - t_prev) / dt_sec;

                    // When 5 or more samples available, use 5-point Savitzky-Golay for curvature
                    if (w->count >= 5) {
                        uint8_t idx0 = head;
                        uint8_t idx1 = (uint8_t)((head + BATTERY_SPATIAL_WINDOW_LEN - 1) % BATTERY_SPATIAL_WINDOW_LEN);
                        uint8_t idx2 = (uint8_t)((head + BATTERY_SPATIAL_WINDOW_LEN - 2) % BATTERY_SPATIAL_WINDOW_LEN);
                        uint8_t idx3 = (uint8_t)((head + BATTERY_SPATIAL_WINDOW_LEN - 3) % BATTERY_SPATIAL_WINDOW_LEN);
                        uint8_t idx4 = (uint8_t)((head + BATTERY_SPATIAL_WINDOW_LEN - 4) % BATTERY_SPATIAL_WINDOW_LEN);

                        float v0 = w->v_history[idx0];
                        float v1 = w->v_history[idx1];
                        float v2 = w->v_history[idx2];
                        float v3 = w->v_history[idx3];
                        float v4 = w->v_history[idx4];

                        if (is_valid_sensor_val(v0) && is_valid_sensor_val(v1) &&
                            is_valid_sensor_val(v2) && is_valid_sensor_val(v3) && is_valid_sensor_val(v4)) {
                            // 5-point 2nd derivative operator: (2*v0 - v1 - 2*v2 - v3 + 2*v4) / 7
                            d2v_dt2 = (2.0f * v0 - v1 - 2.0f * v2 - v3 + 2.0f * v4) / (7.0f * (dt_sec * dt_sec + EPSILON_FLOAT));
                        }
                    }
                }
            }
        }

        if (t_valid && dt_dt > max_dtdt) {
            max_dtdt = dt_dt;
            max_dtdt_idx = i;
        }
        if (v_valid && fabsf(d2v_dt2) > max_d2vdt2) {
            max_d2vdt2 = fabsf(d2v_dt2);
            max_d2vdt2_idx = i;
        }

        battery_cell_features_t *f = &ctx->features[i];
        f->voltage = v_valid ? v : 0.0f;
        f->temperature = t_valid ? t : 0.0f;
        f->current = curr;
        f->soc = v_valid ? soc_from_ocv(v - curr * r0) : 0.0f;
        f->dV_dt = dv_dt;
        f->dT_dt = dt_dt;
        f->d2V_dt2 = d2v_dt2;
        f->overpotential = 0.0f;     // refined in Pass 2
        f->polarization_res = 0.0f;  // refined in Pass 2
        f->spatial_temp_grad = 0.0f; // calculated in pass 2
        f->tr_stage = BATTERY_TR_STAGE_NORMAL;
    }

    // Mean & Standard Deviation
    float v_mean = (valid_v_count > 0) ? (float)(v_sum / (double)valid_v_count) : 0.0f;
    float t_mean = (valid_t_count > 0) ? (float)(t_sum / (double)valid_t_count) : 0.0f;
    double v_var_sum = 0.0;
    double t_var_sum = 0.0;

    for (uint32_t i = 0; i < num_cells; i++) {
        float v = voltages[i];
        float t = temperatures[i];
        if (is_valid_sensor_val(v)) {
            double v_diff = (double)v - (double)v_mean;
            v_var_sum += v_diff * v_diff;
        }
        if (is_valid_sensor_val(t)) {
            double t_diff = (double)t - (double)t_mean;
            t_var_sum += t_diff * t_diff;
        }
    }
    float v_std = (valid_v_count > 0) ? sqrtf((float)(v_var_sum / (double)valid_v_count)) : 0.0f;
    float t_std = (valid_t_count > 0) ? sqrtf((float)(t_var_sum / (double)valid_t_count)) : 0.0f;

    if (valid_v_count == 0) { v_min = 0.0f; v_max = 0.0f; }
    if (valid_t_count == 0) { t_min = 0.0f; t_max = 0.0f; }
    if (max_dtdt < -1e8f) max_dtdt = 0.0f;

    out_result->v_min = v_min;
    out_result->v_max = v_max;
    out_result->v_mean = v_mean;
    out_result->v_std = v_std;
    out_result->v_median = (v_min + v_max) * 0.5f; // Fast mid-point baseline
    out_result->v_min_cell_idx = v_min_idx;
    out_result->v_max_cell_idx = v_max_idx;

    out_result->t_min = t_min;
    out_result->t_max = t_max;
    out_result->t_mean = t_mean;
    out_result->t_std = t_std;
    out_result->t_median = (t_min + t_max) * 0.5f;
    out_result->t_min_cell_idx = t_min_idx;
    out_result->t_max_cell_idx = t_max_idx;

    out_result->max_dT_dt = max_dtdt;
    out_result->max_dT_dt_cell_idx = max_dtdt_idx;
    out_result->max_d2V_dt2 = max_d2vdt2;
    out_result->max_d2V_dt2_cell_idx = max_d2vdt2_idx;

    // Second Pass: Spatial Laplacian Gradients, Polarization Overpotential, Diffusion Vectors, Stage Classification & Hotspots
    for (uint32_t i = 0; i < num_cells; i++) {
        battery_cell_features_t *f = &ctx->features[i];
        float v_raw = voltages[i];
        float t_raw = temperatures[i];
        bool v_valid = is_valid_sensor_val(v_raw);
        bool t_valid = is_valid_sensor_val(t_raw);
        float curr_i = ctx->raw_i[i];

        // Polarization overpotential relative to pack consensus mean
        if (v_valid) {
            float eta = fabsf(v_mean - v_raw);
            float rp = (fabsf(curr_i) > 0.1f) ? (eta / fabsf(curr_i)) : (eta / 1.0f);
            f->overpotential = eta;
            f->polarization_res = rp;
        } else {
            f->overpotential = 0.0f;
            f->polarization_res = 0.0f;
        }

        float max_grad = 0.0f;
        float diff_x = 0.0f;
        float diff_y = 0.0f;
        float diff_z = 0.0f;

        // 6-direction spatial gradient
        // 0: -X (dx), 1: +X (dx), 2: -Y (dy), 3: +Y (dy), 4: -Z (dz), 5: +Z (dz)
        static const float dir_dist_factors[6] = { -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f };

        if (t_valid) {
            for (int k = 0; k < 6; k++) {
                int32_t neighbor_idx = ctx->neighbor_indices[i][k];
                if (neighbor_idx >= 0 && (uint32_t)neighbor_idx < num_cells) {
                    float t_neighbor = temperatures[neighbor_idx];
                    if (is_valid_sensor_val(t_neighbor)) {
                        float dist = (k < 2) ? ctx->config.cell_dx : ((k < 4) ? ctx->config.cell_dy : ctx->config.cell_dz);
                        if (dist < EPSILON_FLOAT) dist = 0.05f;

                        float grad = (t_raw - t_neighbor) / dist; // °C / m
                        if (grad > max_grad) {
                            max_grad = grad;
                        }

                        // Diffusion flux vector: heat flows away from hotspot to colder neighbor
                        float flux = (t_raw - t_neighbor) / (dist * dist);
                        if (k < 2) diff_x += flux * dir_dist_factors[k];
                        else if (k < 4) diff_y += flux * dir_dist_factors[k];
                        else diff_z += flux * dir_dist_factors[k];
                    }
                }
            }
        }

        f->spatial_temp_grad = max_grad;

        // Thermal runaway stage determination
        battery_tr_stage_t stage = BATTERY_TR_STAGE_NORMAL;
        float dtdt = f->dT_dt;

        if (t_valid) {
            if (t_raw >= ctx->config.temp_explosive_thresh || dtdt >= ctx->config.dtdt_explosive_thresh) {
                stage = BATTERY_TR_STAGE_EXPLOSIVE;
            } else if (t_raw >= ctx->config.temp_acceleration_thresh || dtdt >= ctx->config.dtdt_acceleration_thresh) {
                stage = BATTERY_TR_STAGE_ACCELERATION;
            } else if (t_raw >= ctx->config.temp_self_heating_thresh || dtdt >= ctx->config.dtdt_self_heating_thresh) {
                stage = BATTERY_TR_STAGE_SELF_HEATING;
            } else if (t_raw >= ctx->config.temp_attention_thresh || dtdt >= ctx->config.dtdt_attention_thresh) {
                stage = BATTERY_TR_STAGE_ATTENTION;
            }
        }
        f->tr_stage = stage;

        if ((int)stage > (int)out_result->overall_tr_stage) {
            out_result->overall_tr_stage = stage;
        }

        // Collect Hotspots if stage >= ATTENTION
        if (t_valid && stage >= BATTERY_TR_STAGE_ATTENTION && out_result->num_hotspots < BATTERY_DIFFUSION_MAX_HOTSPOTS) {
            uint32_t h_idx = out_result->num_hotspots++;
            battery_diffusion_hotspot_t *h = &out_result->hotspots[h_idx];
            h->cell_index = i;
            memcpy(&h->coord, &ctx->coords[i], sizeof(battery_cell_coord_t));
            h->temperature = t_raw;
            h->dT_dt = dtdt;
            h->spatial_grad = max_grad;
            h->diffusion_vec_x = diff_x;
            h->diffusion_vec_y = diff_y;
            h->diffusion_vec_z = diff_z;
            h->stage = stage;
            // Estimated thermal front propagation speed: proportional to gradient / rise rate
            float flux_mag = sqrtf(diff_x * diff_x + diff_y * diff_y + diff_z * diff_z);
            h->estimated_propagation_speed = (flux_mag > EPSILON_FLOAT && dtdt > 0.0f) ? (dtdt / (max_grad + EPSILON_FLOAT)) : 0.0f;
        }

        // Outlier detection using Z-Score (3-Sigma)
        if (v_valid && v_std > EPSILON_FLOAT) {
            float z_v = fabsf(v_raw - v_mean) / v_std;
            if (z_v >= ctx->config.z_score_outlier_thresh && out_result->num_voltage_outliers < BATTERY_TOP_OUTLIERS_MAX) {
                uint32_t out_idx = out_result->num_voltage_outliers++;
                out_result->voltage_outliers[out_idx].cell_index = i;
                out_result->voltage_outliers[out_idx].value = v_raw;
                out_result->voltage_outliers[out_idx].z_score = z_v;
            }
        }

        if (t_valid && t_std > EPSILON_FLOAT) {
            float z_t = fabsf(t_raw - t_mean) / t_std;
            if (z_t >= ctx->config.z_score_outlier_thresh && out_result->num_temp_outliers < BATTERY_TOP_OUTLIERS_MAX) {
                uint32_t out_idx = out_result->num_temp_outliers++;
                out_result->temp_outliers[out_idx].cell_index = i;
                out_result->temp_outliers[out_idx].value = t_raw;
                out_result->temp_outliers[out_idx].z_score = z_t;
            }
        }
    }

    // Sort Hotspots descending by (stage descending, then temperature descending)
    for (uint32_t a = 0; a < out_result->num_hotspots; a++) {
        for (uint32_t b = a + 1; b < out_result->num_hotspots; b++) {
            bool swap = false;
            if (out_result->hotspots[b].stage > out_result->hotspots[a].stage) {
                swap = true;
            } else if (out_result->hotspots[b].stage == out_result->hotspots[a].stage) {
                if (out_result->hotspots[b].temperature > out_result->hotspots[a].temperature) {
                    swap = true;
                }
            }
            if (swap) {
                battery_diffusion_hotspot_t tmp = out_result->hotspots[a];
                out_result->hotspots[a] = out_result->hotspots[b];
                out_result->hotspots[b] = tmp;
            }
        }
    }

    // Sort Voltage Outliers descending by z_score
    for (uint32_t a = 0; a < out_result->num_voltage_outliers; a++) {
        for (uint32_t b = a + 1; b < out_result->num_voltage_outliers; b++) {
            if (out_result->voltage_outliers[b].z_score > out_result->voltage_outliers[a].z_score) {
                battery_outlier_t tmp = out_result->voltage_outliers[a];
                out_result->voltage_outliers[a] = out_result->voltage_outliers[b];
                out_result->voltage_outliers[b] = tmp;
            }
        }
    }

    // Sort Temperature Outliers descending by z_score
    for (uint32_t a = 0; a < out_result->num_temp_outliers; a++) {
        for (uint32_t b = a + 1; b < out_result->num_temp_outliers; b++) {
            if (out_result->temp_outliers[b].z_score > out_result->temp_outliers[a].z_score) {
                battery_outlier_t tmp = out_result->temp_outliers[a];
                out_result->temp_outliers[a] = out_result->temp_outliers[b];
                out_result->temp_outliers[b] = tmp;
            }
        }
    }

    uint64_t t_end = get_time_monotonic_us();
    out_result->compute_duration_us = t_end - t_start;

    return 0;
}
