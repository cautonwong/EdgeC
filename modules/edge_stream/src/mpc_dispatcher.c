/**
 * @file mpc_dispatcher.c
 * @brief Model Predictive Control (MPC) Energy Dispatcher Implementation (AEP-730 / AEP-752)
 */

#include "mpc_dispatcher.h"
#include <math.h>
#include <string.h>

struct mpc_dispatcher {
    mpc_dispatcher_config_t config;
    mpc_dispatcher_ports_t  ports;
    
    /* Precomputed QP matrices in static memory (0 heap allocations) */
    double H[MPC_MAX_HORIZON][MPC_MAX_HORIZON];
    double f[MPC_MAX_HORIZON];
    double u_opt[MPC_MAX_HORIZON];
    double lambda[2 * MPC_MAX_HORIZON];
    
    double last_power_kw;
    uint32_t step_count;
};

/* -------------------------------------------------------------------------- */
/* Compile-Time Size & Alignment Assertions (AEP-752)                          */
/* -------------------------------------------------------------------------- */
_Static_assert(sizeof(struct mpc_dispatcher) <= sizeof(mpc_dispatcher_storage_t),
               "mpc_dispatcher_storage_t buffer too small for struct mpc_dispatcher");
_Static_assert(_Alignof(struct mpc_dispatcher) <= _Alignof(mpc_dispatcher_storage_t),
               "mpc_dispatcher_storage_t alignment mismatch for struct mpc_dispatcher");

static void precompute_qp_hessian(struct mpc_dispatcher *mpc) {
    uint32_t N = mpc->config.horizon;
    double dt = mpc->config.dt_s;
    double cap_s = mpc->config.capacity_ah * 3600.0;
    double b = dt / (cap_s > 1.0 ? cap_s : 3600.0);
    double Q = mpc->config.q_soc_weight > 0 ? mpc->config.q_soc_weight : 1000.0;
    double R = mpc->config.r_power_weight > 0 ? mpc->config.r_power_weight : 0.1;

    /* Formulation of H = 2 * (S_u^T * Q_bar * S_u + R_bar) */
    for (uint32_t i = 0; i < N; ++i) {
        for (uint32_t j = 0; j < N; ++j) {
            double h_val = 0.0;
            uint32_t max_ij = i > j ? i : j;
            for (uint32_t k = max_ij; k < N; ++k) {
                h_val += b * b * Q;
            }
            if (i == j) {
                h_val += R;
            }
            mpc->H[i][j] = h_val;
        }
    }
}

mpc_dispatcher_t *mpc_dispatcher_create_in(mpc_dispatcher_storage_t *store,
                                           const mpc_dispatcher_config_t *cfg,
                                           const mpc_dispatcher_ports_t *ports) {
    if (!store || !cfg) return NULL;
    if (cfg->horizon == 0 || cfg->horizon > MPC_MAX_HORIZON || cfg->dt_s <= 0.0 || cfg->capacity_ah <= 0.0) {
        return NULL;
    }

    mpc_dispatcher_t *mpc = (mpc_dispatcher_t *)store->raw;
    memset(mpc, 0, sizeof(struct mpc_dispatcher));

    mpc->config = *cfg;
    if (ports) {
        mpc->ports = *ports;
    }

    if (mpc->config.max_power_kw <= 0.0) mpc->config.max_power_kw = 100.0;
    if (mpc->config.min_power_kw >= 0.0) mpc->config.min_power_kw = -100.0;
    if (mpc->config.q_soc_weight <= 0.0) mpc->config.q_soc_weight = 1000.0;
    if (mpc->config.r_power_weight <= 0.0) mpc->config.r_power_weight = 0.1;

    precompute_qp_hessian(mpc);
    return mpc;
}

int mpc_dispatcher_step(mpc_dispatcher_t *mpc, double current_soc, double *out_optimal_power_kw) {
    if (!mpc) return -1;

    uint32_t N = mpc->config.horizon;
    double dt = mpc->config.dt_s;
    double cap_s = mpc->config.capacity_ah * 3600.0;
    double b = dt / (cap_s > 1.0 ? cap_s : 3600.0);
    double Q = mpc->config.q_soc_weight;

    /* Current SOC tracking error x0 = current_soc - target_soc */
    double x0 = current_soc - mpc->config.target_soc;

    /* Compute linear gradient f_vec = S_u^T * Q_bar * S_x * x0 */
    for (uint32_t i = 0; i < N; ++i) {
        double f_val = 0.0;
        for (uint32_t k = i; k < N; ++k) {
            f_val += b * Q * x0;
        }
        mpc->f[i] = f_val;
    }

    /* Hildreth / Projected Gauss-Seidel for box-constrained QP:
     * min 1/2 u^T H u + f^T u  s.t. min_p <= u_i <= max_p
     */
    for (uint32_t i = 0; i < N; ++i) {
        mpc->u_opt[i] = 0.0;
    }

    /* Iterative coordinate descent */
    for (int iter = 0; iter < 30; ++iter) {
        for (uint32_t i = 0; i < N; ++i) {
            double h_ii = mpc->H[i][i];
            if (h_ii < 1e-9) h_ii = 1e-9;

            double sum_other = 0.0;
            for (uint32_t j = 0; j < N; ++j) {
                if (i != j) {
                    sum_other += mpc->H[i][j] * mpc->u_opt[j];
                }
            }

            double u_unc = -(mpc->f[i] + sum_other) / h_ii;
            /* Box projection */
            if (u_unc > mpc->config.max_power_kw) u_unc = mpc->config.max_power_kw;
            if (u_unc < mpc->config.min_power_kw) u_unc = mpc->config.min_power_kw;

            mpc->u_opt[i] = u_unc;
        }
    }

    /* Optimal first-stage control action */
    double p_first = mpc->u_opt[0];
    mpc->last_power_kw = p_first;
    mpc->step_count++;

    if (out_optimal_power_kw) {
        *out_optimal_power_kw = p_first;
    }
    return 0;
}

int mpc_dispatcher_step_ports(mpc_dispatcher_t *mpc, uint32_t tag_soc_in, uint32_t tag_p_out) {
    if (!mpc) return -1;

    double soc_val = mpc->config.target_soc;
    if (mpc->ports.reader.read_tag && tag_soc_in) {
        mpc->ports.reader.read_tag(mpc->ports.reader.ctx, tag_soc_in, &soc_val, NULL);
    }

    /* If SOC is passed as percentage 0..100, normalize to 0..1 */
    if (soc_val > 1.0) {
        soc_val /= 100.0;
    }

    double p_opt = 0.0;
    int rc = mpc_dispatcher_step(mpc, soc_val, &p_opt);
    if (rc != 0) return rc;

    uint64_t now_us = 0;
    if (mpc->ports.clock.now_us) {
        now_us = mpc->ports.clock.now_us(mpc->ports.clock.ctx);
    }

    if (mpc->ports.writer.write_tag && tag_p_out) {
        mpc->ports.writer.write_tag(mpc->ports.writer.ctx, tag_p_out, p_opt, now_us);
    }

    return 0;
}

void mpc_dispatcher_reset(mpc_dispatcher_t *mpc) {
    if (!mpc) return;
    for (uint32_t i = 0; i < MPC_MAX_HORIZON; ++i) {
        mpc->u_opt[i] = 0.0;
        mpc->f[i] = 0.0;
    }
    mpc->last_power_kw = 0.0;
    mpc->step_count = 0;
}

double mpc_dispatcher_get_last_power(const mpc_dispatcher_t *mpc) {
    return mpc ? mpc->last_power_kw : 0.0;
}

uint32_t mpc_dispatcher_get_step_count(const mpc_dispatcher_t *mpc) {
    return mpc ? mpc->step_count : 0;
}

double mpc_dispatcher_get_target_soc(const mpc_dispatcher_t *mpc) {
    return mpc ? mpc->config.target_soc : 0.0;
}
