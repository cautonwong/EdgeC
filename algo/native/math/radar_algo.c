/**
 * @file radar_algo.c
 * @brief 工业级雷达信号处理算法实现
 */

#include "radar_algo.h"
#include <stdlib.h>

int radar_cfar_ca(const double *data, size_t n, uint32_t train_cells, 
                  uint32_t guard_cells, double alpha, uint8_t *peaks) {
    int count = 0;
    if (!data || !peaks || n < (2 * (train_cells + guard_cells) + 1)) return 0;

    for (size_t i = 0; i < n; i++) {
        peaks[i] = 0;
        
        // 边界检查：确保滑动窗口不越界
        if (i < (train_cells + guard_cells) || i >= n - (train_cells + guard_cells)) {
            continue;
        }

        double sum = 0;
        uint32_t actual_train_count = 0;

        // 累加左侧训练单元
        for (size_t j = i - train_cells - guard_cells; j < i - guard_cells; j++) {
            sum += data[j];
            actual_train_count++;
        }
        // 累加右侧训练单元
        for (size_t j = i + guard_cells + 1; j <= i + guard_cells + train_cells; j++) {
            sum += data[j];
            actual_train_count++;
        }

        double avg_noise = sum / actual_train_count;
        double threshold = avg_noise * alpha;

        if (data[i] > threshold) {
            peaks[i] = 1;
            count++;
        }
    }

    return count;
}
