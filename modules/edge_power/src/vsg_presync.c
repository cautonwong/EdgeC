#include "vsg_control_internal.h"

/* -------------------------------------------------------------------------- */
/* AEP-723 Seamless Grid-Forming Transition & Pre-Sync Implementation         */
/* -------------------------------------------------------------------------- */
int vsg_controller_set_mode(vsg_controller_t *vsg, vsg_grid_mode_t mode) {
    if (!vsg) return -1;
    vsg->transition.current_mode = mode;
    if (mode == VSG_GRID_MODE_CONNECTED) {
        vsg->transition.sync_locked = false;
        vsg->transition.lock_hold_cycles = 0;
        vsg->sync_theta_integ = 0.0;
    }
    return 0;
}

vsg_grid_mode_t vsg_controller_get_mode(const vsg_controller_t *vsg) {
    return vsg ? vsg->transition.current_mode : VSG_GRID_MODE_CONNECTED;
}

int vsg_controller_trigger_unplanned_islanding(vsg_controller_t *vsg,
                                              double grid_v, double grid_f, double grid_theta) {
    if (!vsg) return -1;

    /* Microsecond-level state handover to ensure zero voltage/phase jump */
    if (grid_v > 0.0) {
        vsg->voltage_v = grid_v;
    }
    if (grid_f > 0.0) {
        vsg->frequency_hz = grid_f;
        vsg->omega = 2.0 * M_PI * grid_f;
    }
    vsg->theta = fmod(grid_theta, 2.0 * M_PI);
    if (vsg->theta < 0.0) vsg->theta += 2.0 * M_PI;

    vsg->transition.current_mode = VSG_GRID_MODE_ISLANDED;
    vsg->transition.sync_locked = false;
    vsg->transition.lock_hold_cycles = 0;
    vsg->sync_theta_integ = 0.0;

    return 0;
}

int vsg_controller_step_presync(vsg_controller_t *vsg,
                                double grid_v, double grid_f, double grid_theta, double dt_s) {
    if (!vsg || dt_s <= 0.0 || grid_v <= 0.0 || grid_f <= 0.0) return -1;

    vsg->transition.current_mode = VSG_GRID_MODE_PRE_SYNC;

    /* 1. Calculate phase, voltage, and frequency errors */
    double delta_theta = _wrap_pi(grid_theta - vsg->theta);
    double delta_v = grid_v - vsg->voltage_v;
    double delta_f = grid_f - vsg->frequency_hz;

    vsg->transition.delta_theta_rad = delta_theta;
    vsg->transition.delta_volt_v = delta_v;
    vsg->transition.delta_freq_hz = delta_f;

    /* 2. PI phase tracking trim */
    vsg->sync_theta_integ += delta_theta * dt_s;
    if (vsg->sync_theta_integ > 10.0) vsg->sync_theta_integ = 10.0;
    if (vsg->sync_theta_integ < -10.0) vsg->sync_theta_integ = -10.0;

    double omega_trim = vsg->config.sync_kp_theta * delta_theta +
                        vsg->config.sync_ki_theta * vsg->sync_theta_integ;

    vsg->omega = 2.0 * M_PI * grid_f + omega_trim;
    vsg->frequency_hz = vsg->omega / (2.0 * M_PI);

    /* 3. Voltage magnitude tracking trim */
    double v_trim = vsg->config.sync_kp_v * delta_v;
    vsg->voltage_v += v_trim * dt_s * 10.0;
    if (vsg->voltage_v > 1.2 * vsg->config.v_nominal) vsg->voltage_v = 1.2 * vsg->config.v_nominal;
    if (vsg->voltage_v < 0.8 * vsg->config.v_nominal) vsg->voltage_v = 0.8 * vsg->config.v_nominal;

    /* 4. Update phase angle */
    vsg->theta += vsg->omega * dt_s;
    vsg->theta = fmod(vsg->theta, 2.0 * M_PI);
    if (vsg->theta < 0.0) vsg->theta += 2.0 * M_PI;

    /* 5. Check if tolerances are satisfied for lock hold */
    bool in_tol = (fabs(delta_theta) <= vsg->config.sync_theta_tol_rad) &&
                  (fabs(delta_v) <= vsg->config.sync_volt_tol_v) &&
                  (fabs(delta_f) <= vsg->config.sync_freq_tol_hz);

    if (in_tol) {
        vsg->transition.lock_hold_cycles++;
        if (vsg->transition.lock_hold_cycles >= 5) {
            vsg->transition.sync_locked = true;
        }
    } else {
        vsg->transition.lock_hold_cycles = 0;
        vsg->transition.sync_locked = false;
    }

    vsg->step_count++;
    return 0;
}

bool vsg_controller_is_sync_locked(const vsg_controller_t *vsg) {
    return vsg ? vsg->transition.sync_locked : false;
}

int vsg_controller_get_transition_status(const vsg_controller_t *vsg,
                                         vsg_transition_status_t *status) {
    if (!vsg || !status) return -1;
    *status = vsg->transition;
    return 0;
}

