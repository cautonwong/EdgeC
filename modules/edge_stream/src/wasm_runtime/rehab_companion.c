#define IMPORT(name) __attribute__((import_module("env"), import_name(name)))

IMPORT("host_get_tag") int host_get_tag(unsigned int tag_id, double *val_ptr);
IMPORT("host_set_tag") int host_set_tag(unsigned int tag_id, double val);
IMPORT("ext_log") void ext_log(unsigned int level, const char *msg);

// Wasm 状态机历史计数
static int g_fall_state = 0;      // 0=正常, 1=探测失重, 2=等待SMA判定
static int g_sma_ticks = 0;       // 碰撞后的 tick 计数器
static double g_sma_sum = 0.0;    // SMA 累加器

// 连续失重计数
static int g_weightless_consecutive = 0;
// 碰撞时间窗计数 (500ms)
static int g_collision_window_ticks = 0;

// Wasm 滑动窗口记录最新 100 个点 (1s)
static double g_acc_history[100];
static int g_acc_idx = 0;

__attribute__((export_name("on_tick")))
void on_tick() {
    double acc = 1.0;
    double hr = 75.0;

    // 1. 读取实时原始加速度与心率值
    if (host_get_tag(12006, &acc) != 0) {
        acc = 1.0;
    }
    if (host_get_tag(12005, &hr) != 0) {
        hr = 75.0;
    }

    // 压入滑动窗口
    g_acc_history[g_acc_idx] = acc;
    g_acc_idx = (g_acc_idx + 1) % 100;

    // 2. 三重判定跌倒检测算法 (失重 + 碰撞 + SMA 恢复状态)
    double svm = acc;
    int detected_fall = 0;

    // 统计连续失重相采样点
    if (svm < 0.2) {
        g_weightless_consecutive++;
    } else {
        g_weightless_consecutive = 0;
    }

    if (g_fall_state == 0) {
        // 失重相判定：若连续 10 个采样点其 SVM < 0.2G，判定进入空中坠落状态
        if (g_weightless_consecutive >= 10) {
            g_fall_state = 1;
            g_collision_window_ticks = 0;
            ext_log(2, "WASM: Weightlessness threshold triggered consecutively for 10 ticks!");
        }
    } else if (g_fall_state == 1) {
        g_collision_window_ticks++;
        // 冲击碰撞判定：在失重信号触发后的 500ms 时间窗内（50个采样点），如果检索到任意采样点 SVM > 3.2G
        if (svm > 3.2) {
            g_fall_state = 2;
            g_sma_ticks = 0;
            g_sma_sum = 0.0;
            ext_log(3, "WASM: Fall collision impact detected! Starting SMA monitoring...");
        } else if (g_collision_window_ticks > 50) {
            // 超出 500ms 未检测到碰撞，重置回到状态 0
            g_fall_state = 0;
            ext_log(2, "WASM: Collision window timed out. Resetting state.");
        }
    } else if (g_fall_state == 2) {
        g_sma_ticks++;
        // 强碰撞后 1.0s (100个tick) 开始，采集后续 1.0s (100个tick) 内的活动面积
        if (g_sma_ticks > 100 && g_sma_ticks <= 200) {
            // 累加差分加速度以评估身体动能 (在静态/躺下时，偏离 1.0G 的波动极低)
            double dev = acc - 1.0;
            if (dev < 0) dev = -dev;
            g_sma_sum += dev;
        } else if (g_sma_ticks > 200) {
            double sma = g_sma_sum / 100.0;
            if (sma < 0.4) {
                // SMA 动力学极低，判定失去知觉，触发真实跌倒
                detected_fall = 1;
                ext_log(4, "WASM ALERT: Fall confirmed by low SMA (static posture)!");
            } else {
                ext_log(2, "WASM: Posture recovered. Fall false alarm cleared.");
            }
            g_fall_state = 0; // 重置状态机
        }
    }

    if (detected_fall) {
        host_set_tag(12003, 1.0); // BOX_FALL_ALERT = 1
        host_set_tag(12002, 2.0); // LED = 红灯闪烁
        host_set_tag(12001, 1.0); // BOX_SYSTEM_STATUS = 故障
    }

    // 3. 心率波动警报 (超过 140 或低于 50)
    if (hr < 50.0 || hr > 140.0) {
        ext_log(3, "WASM ALERT: Abnormal heart rate detected!");
        host_set_tag(12004, 1.0); // BOX_HEART_RATE_ALERT = 1
        host_set_tag(12002, 2.0); // LED = 红灯闪烁
        host_set_tag(12001, 1.0); // BOX_SYSTEM_STATUS = 故障
    }

    // 4. 指示灯状态与警报状态同步 (不自动重置警报，由 Go 网关消费并回写确认后方可恢复)
    double f_alert = 0.0;
    double h_alert = 0.0;
    host_get_tag(12003, &f_alert);
    host_get_tag(12004, &h_alert);
    
    double current_status = 0.0;
    double current_led = 0.0;
    host_get_tag(12001, &current_status);
    host_get_tag(12002, &current_led);

    if (f_alert == 0.0 && h_alert == 0.0) {
        if (current_status == 1.0) {
            host_set_tag(12001, 0.0); // 系统状态恢复正常
        }
        if (current_led == 2.0) {
            host_set_tag(12002, 1.0); // 恢复绿灯常亮
        }
    }
}
