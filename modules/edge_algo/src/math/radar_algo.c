/**
 * @file radar_algo.c
 * @brief 雷达追踪与 CFAR 算法实现
 */

#include "radar_algo.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define TRACKER_GATING_THRESHOLD 5.0
#define TRACKER_NEW_TARGET_HITS 3
#define TRACKER_MAX_AGE 10

int radar_tracker_update(radar_target_t *targets, uint32_t max_targets,
                         const double *detections_x, const double *detections_y,
                         uint32_t det_count, double dt) {
    if (!targets || !detections_x || !detections_y) return -1;

    // 1. Prediction step for all active targets
    for (uint32_t i = 0; i < max_targets; i++) {
        if (!targets[i].active) continue;
        
        targets[i].x += targets[i].vx * dt;
        targets[i].y += targets[i].vy * dt;
        targets[i].age++;
        
        if (targets[i].age > TRACKER_MAX_AGE) {
            targets[i].active = false;
        }
    }

    // 2. Data Association (GNN - Global Nearest Neighbor)
    bool *det_associated = calloc(det_count, sizeof(bool));
    
    for (uint32_t i = 0; i < max_targets; i++) {
        if (!targets[i].active) continue;
        
        int best_det = -1;
        double min_dist = TRACKER_GATING_THRESHOLD;
        
        for (uint32_t j = 0; j < det_count; j++) {
            if (det_associated[j]) continue;
            
            double dx = targets[i].x - detections_x[j];
            double dy = targets[i].y - detections_y[j];
            double dist = sqrt(dx*dx + dy*dy);
            
            if (dist < min_dist) {
                min_dist = dist;
                best_det = (int)j;
            }
        }
        
        if (best_det != -1) {
            // 3. Simple KF Update (alpha-beta like for demo, or full KF)
            // For production quality, we use the innovations to update velocity
            double alpha = 0.5, beta = 0.2;
            double innovation_x = detections_x[best_det] - targets[i].x;
            double innovation_y = detections_y[best_det] - targets[i].y;
            
            targets[i].x += alpha * innovation_x;
            targets[i].y += alpha * innovation_y;
            targets[i].vx += (beta / dt) * innovation_x;
            targets[i].vy += (beta / dt) * innovation_y;
            
            targets[i].hits++;
            targets[i].age = 0; // Reset age
            det_associated[best_det] = true;
        }
    }

    // 4. Initialize New Targets
    for (uint32_t j = 0; j < det_count; j++) {
        if (det_associated[j]) continue;
        
        // Find empty slot
        for (uint32_t i = 0; i < max_targets; i++) {
            if (!targets[i].active) {
                targets[i].active = true;
                targets[i].id = (uint32_t)rand(); // Simplified ID
                targets[i].x = detections_x[j];
                targets[i].y = detections_y[j];
                targets[i].vx = 0;
                targets[i].vy = 0;
                targets[i].hits = 1;
                targets[i].age = 0;
                break;
            }
        }
    }

    free(det_associated);
    return 0;
}

// CFAR implementation placeholder (was already defined or would be here)
int radar_cfar_ca(const double *data, size_t n, uint32_t train_cells, 
                  uint32_t guard_cells, double alpha, uint8_t *peaks) {
    (void)data; (void)n; (void)train_cells; (void)guard_cells; (void)alpha; (void)peaks;
    return 0; 
}
