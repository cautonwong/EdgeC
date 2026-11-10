#include "vsg_control_internal.h"

/* -------------------------------------------------------------------------- */
/* AEP-724 Microgrid Black Start & Multi-Unit Lockstep Engine APIs            */
/* -------------------------------------------------------------------------- */

int vsg_controller_trigger_black_start(vsg_controller_t *vsg) {
    if (!vsg) return -1;

    vsg->transition.current_mode = VSG_GRID_MODE_BLACK_START;
    vsg->bs_status.state = VSG_BS_STATE_RAMPING;
    vsg->bs_status.ramp_progress_pct = 0.0;
    vsg->bs_status.v_ref_v = 0.0;
    vsg->bs_status.f_ref_hz = 0.0;
    vsg->bs_status.elapsed_time_s = 0.0;
    vsg->bs_status.is_ready_for_load = false;
    vsg->bs_settle_timer_s = 0.0;

    vsg->voltage_v = 0.0;
    vsg->frequency_hz = 0.0;
    vsg->omega = 0.0;
    vsg->theta = 0.0;
    vsg->transition.sync_locked = false;
    vsg->transition.lock_hold_cycles = 0;

    return 0;
}

int vsg_controller_step_black_start(vsg_controller_t *vsg, double i_meas_a, double sync_angle_rad,
                                    double dt_s, double *out_freq_hz, double *out_voltage_v) {
    if (!vsg || dt_s <= 0.0) return -1;

    if (vsg->bs_status.state == VSG_BS_STATE_IDLE || vsg->bs_status.state == VSG_BS_STATE_FAULT) {
        if (out_freq_hz) *out_freq_hz = 0.0;
        if (out_voltage_v) *out_voltage_v = 0.0;
        return 0;
    }

    vsg->bs_status.elapsed_time_s += dt_s;

    /* 1. Inrush current detection and soft throttling */
    double throttle_factor = 1.0;
    if (vsg->config.black_start.inrush_i_limit_a > 0.0) {
        double i_lim = vsg->config.black_start.inrush_i_limit_a;
        if (i_meas_a > 2.0 * i_lim) {
            /* Severe inrush fault trip */
            vsg->bs_status.state = VSG_BS_STATE_FAULT;
            vsg->voltage_v = 0.0;
            vsg->frequency_hz = 0.0;
            vsg->omega = 0.0;
            if (out_freq_hz) *out_freq_hz = 0.0;
            if (out_voltage_v) *out_voltage_v = 0.0;
            return 0;
        } else if (i_meas_a > 0.8 * i_lim) {
            /* Soft throttle ramp rate */
            double excess = (i_meas_a - 0.8 * i_lim) / (0.4 * i_lim);
            if (excess > 1.0) excess = 1.0;
            throttle_factor = 1.0 - 0.8 * excess;
            if (throttle_factor < 0.1) throttle_factor = 0.1;
        }
    }

    /* 2. Soft ramp-up profile */
    double target_v = vsg->config.v_nominal;
    double target_f = vsg->config.f_nominal;

    double dv_rate = (target_v / vsg->config.black_start.ramp_time_s) * throttle_factor;
    if (dv_rate > vsg->config.black_start.max_volt_slew_v_s) {
        dv_rate = vsg->config.black_start.max_volt_slew_v_s;
    }

    double df_rate = (target_f / vsg->config.black_start.ramp_time_s) * throttle_factor;
    if (df_rate > vsg->config.black_start.max_freq_slew_hz_s) {
        df_rate = vsg->config.black_start.max_freq_slew_hz_s;
    }

    vsg->bs_status.v_ref_v += dv_rate * dt_s;
    if (vsg->bs_status.v_ref_v > target_v) {
        vsg->bs_status.v_ref_v = target_v;
    }

    vsg->bs_status.f_ref_hz += df_rate * dt_s;
    if (vsg->bs_status.f_ref_hz > target_f) {
        vsg->bs_status.f_ref_hz = target_f;
    }

    double prog_v = (target_v > 0.0) ? (vsg->bs_status.v_ref_v / target_v) * 100.0 : 100.0;
    double prog_f = (target_f > 0.0) ? (vsg->bs_status.f_ref_hz / target_f) * 100.0 : 100.0;
    vsg->bs_status.ramp_progress_pct = (prog_v < prog_f) ? prog_v : prog_f;
    if (vsg->bs_status.ramp_progress_pct > 100.0) {
        vsg->bs_status.ramp_progress_pct = 100.0;
    }

    /* 3. Multi-unit lockstep virtual rotor phase alignment */
    double delta_theta = 0.0;
    double omega_ref = 2.0 * M_PI * vsg->bs_status.f_ref_hz;
    double omega_trim = 0.0;

    if (sync_angle_rad >= 0.0) {
        delta_theta = _wrap_pi(sync_angle_rad - vsg->theta);
        if (fabs(delta_theta) > vsg->config.black_start.sync_lock_angle_tol_rad &&
            vsg->bs_status.state != VSG_BS_STATE_ESTABLISHED) {
            vsg->bs_status.state = VSG_BS_STATE_SYNC_LOCK;
        } else if (vsg->bs_status.state == VSG_BS_STATE_SYNC_LOCK) {
            vsg->bs_status.state = VSG_BS_STATE_RAMPING;
        }
        omega_trim = 10.0 * delta_theta;
    }

    vsg->omega = omega_ref + omega_trim;
    vsg->frequency_hz = (vsg->omega > 0.0) ? (vsg->omega / (2.0 * M_PI)) : 0.0;
    vsg->voltage_v = vsg->bs_status.v_ref_v;

    vsg->theta += vsg->omega * dt_s;
    vsg->theta = fmod(vsg->theta, 2.0 * M_PI);
    if (vsg->theta < 0.0) vsg->theta += 2.0 * M_PI;

    /* 4. Check stabilization condition */
    bool v_reached = (target_v - vsg->voltage_v) <= 0.01 * target_v;
    bool f_reached = (target_f - vsg->frequency_hz) <= 0.01 * target_f;
    bool sync_locked = (sync_angle_rad < 0.0) ||
                       (fabs(delta_theta) <= vsg->config.black_start.sync_lock_angle_tol_rad);

    if (v_reached && f_reached && sync_locked) {
        vsg->bs_settle_timer_s += dt_s;
        if (vsg->bs_settle_timer_s >= vsg->config.black_start.settle_time_s) {
            vsg->bs_status.state = VSG_BS_STATE_ESTABLISHED;
            vsg->bs_status.is_ready_for_load = true;
            vsg->transition.current_mode = VSG_GRID_MODE_ISLANDED;
        }
    } else {
        vsg->bs_settle_timer_s = 0.0;
    }

    if (out_freq_hz) *out_freq_hz = vsg->frequency_hz;
    if (out_voltage_v) *out_voltage_v = vsg->voltage_v;

    vsg->step_count++;
    return 0;
}

vsg_black_start_state_t vsg_controller_get_black_start_state(const vsg_controller_t *vsg) {
    return vsg ? vsg->bs_status.state : VSG_BS_STATE_IDLE;
}

int vsg_controller_get_black_start_status(const vsg_controller_t *vsg, vsg_black_start_status_t *status) {
    if (!vsg || !status) return -1;
    *status = vsg->bs_status;
    return 0;
}

void vsg_controller_abort_black_start(vsg_controller_t *vsg) {
    if (!vsg) return;
    vsg->bs_status.state = VSG_BS_STATE_FAULT;
    vsg->bs_status.is_ready_for_load = false;
    vsg->voltage_v = 0.0;
    vsg->frequency_hz = 0.0;
    vsg->omega = 0.0;
}

