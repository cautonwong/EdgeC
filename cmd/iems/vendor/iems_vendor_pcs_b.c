#include "iems_vendor_semantics_internal.h"

/* ------------------------------------------------------------------------- */
/* 1. 厂商适配器（批次 B：sinexcel、unsigned_pow、xiankong、xinai、yingbo、    */
/*    zhongteng）                                                             */
/*                                                                           */
/*    v3 各家的 get_int_value 都把 get_mode/get_connect 误派到 get_onoff       */
/*    （pcs/*.c 的同名函数）；扩展派发按同名方法，不复制该缺陷。               */
/* ------------------------------------------------------------------------- */

/* sinexcel（v3 pcs/sinexcel.c:14-92）：功率写 ActPowSet/ReactPowSet，读回写侧
 * 同一 tag 并按 v3 int 访问器截断；开关 TotalStart/TotalStop 各写 1。
 * v3 set_connect 是空实现且返回 0（sinexcel.c:49-53），这里按未实现返回 -1。 */
static const char *const sinexcel_compatible[] = {"sinexcel", NULL};

static int sinexcel_set_power(iems_vendor_device *dev, double power) {
    return write_tag(dev, "ActPowSet", power);
}

static int sinexcel_set_repower(iems_vendor_device *dev, double repower) {
    return write_tag(dev, "ReactPowSet", repower);
}

static int sinexcel_set_mode(iems_vendor_device *dev, int mode) {
    return write_int(dev, "WorkMode", mode);
}

static int sinexcel_set_onoff(iems_vendor_device *dev, int onoff) {
    if (onoff == IEMS_VENDOR_ON) {
        return write_int(dev, "TotalStart", 1);
    }
    if (onoff == IEMS_VENDOR_OFF) {
        return write_int(dev, "TotalStop", 1);
    }
    return -1;
}

static int sinexcel_get_power(iems_vendor_device *dev, double *power) {
    return read_int_as_double(dev, "ActPowSet", power);
}

static int sinexcel_get_repower(iems_vendor_device *dev, double *repower) {
    return read_int_as_double(dev, "ReactPowSet", repower);
}

static int sinexcel_get_mode(iems_vendor_device *dev, int *mode) {
    return read_int(dev, "WorkMode", mode);
}

/* v3 只认 RunStatu==1 为开机，其余一律按关机（sinexcel.c:73-86）。 */
static int sinexcel_get_onoff(iems_vendor_device *dev, int *onoff) {
    int run_status = 0;
    if (read_int(dev, "RunStatu", &run_status) != 0) {
        return -1;
    }
    *onoff = (run_status == 1) ? IEMS_VENDOR_ON : IEMS_VENDOR_OFF;
    return 0;
}

static int sinexcel_get_connect(iems_vendor_device *dev, int *connect) {
    return read_int(dev, "setConnect", connect);
}

DECLARE_EXTENDED_METHODS(sinexcel);
const iems_vendor_semantics sinexcel_ops = {
    .compatible = sinexcel_compatible,
    .init = vendor_init,
    .set_power = sinexcel_set_power,
    .set_repower = sinexcel_set_repower,
    .set_mode = sinexcel_set_mode,
    .set_onoff = sinexcel_set_onoff,
    .set_connect = unsupported_set_int,
    .get_power = sinexcel_get_power,
    .get_repower = sinexcel_get_repower,
    .get_mode = sinexcel_get_mode,
    .get_onoff = sinexcel_get_onoff,
    .get_connect = sinexcel_get_connect,
    .set_value = sinexcel_set_value,
    .get_int_value = sinexcel_get_int_value,
    .get_double_value = sinexcel_get_double_value,
};
DEFINE_EXTENDED_METHODS(sinexcel);

/* unsigned_pow（v3 pcs/unsigned_pow.c:14-135）：功率按方向写充放电指令
 * 0xAA/0xBB/0xCC 与 |power|，写入前确保 EnergyManageMode=3（外部 EMS 模式）；
 * 读功率/无功/模式走 int 访问器并按显式截断；开关 206(off)/207(on)。 */
static const char *const unsigned_pow_compatible[] = {"unsigned_pow", NULL};

static int unsigned_pow_set_power(iems_vendor_device *dev, double power) {
    int work_mode = 0;
    if (read_int(dev, "EnergyManageMode", &work_mode) != 0) {
        return -1;
    }
    if ((work_mode & 0xFF) != 3 && write_int(dev, "EnergyManageMode", 3) != 0) {
        return -1;
    }
    if (power < -0.001) {
        if (write_int(dev, "BatteryChargeDischargeCmd", 0xAA) != 0) {
            return -1;
        }
        return write_tag(dev, "ActPowSet", -power);
    }
    if (power > 0.001) {
        if (write_int(dev, "BatteryChargeDischargeCmd", 0xBB) != 0) {
            return -1;
        }
        return write_tag(dev, "ActPowSet", power);
    }
    if (write_int(dev, "BatteryChargeDischargeCmd", 0xCC) != 0) {
        return -1;
    }
    return write_tag(dev, "ActPowSet", 0.0);
}

static int unsigned_pow_set_repower(iems_vendor_device *dev, double repower) {
    return write_tag(dev, "QT", repower);
}

static int unsigned_pow_set_mode(iems_vendor_device *dev, int mode) {
    return write_int(dev, "WorkMode", mode);
}

static const onoff_convention unsigned_pow_onoff = {
    .write_tag = "StartStopCtr",
    .read_tag = "RunStatu",
    .on_value = 0xCF,
    .off_value = 0xCE,
};

static int unsigned_pow_set_onoff(iems_vendor_device *dev, int onoff) {
    return onoff_set(dev, &unsigned_pow_onoff, onoff);
}

/* v3 把 connect 原样当本机取值：0 关闭写 0x55、1 并网写 0xA5；其余静默返回 0
 * 且不写（unsigned_pow.c:69-84），这里对未定义取值返回 -1。 */
static int unsigned_pow_set_connect(iems_vendor_device *dev, int connect) {
    if (connect == IEMS_VENDOR_CONNECT_GRID) {
        return write_int(dev, "GridMode", 0xA5);
    }
    if (connect == IEMS_VENDOR_DISCONNECT_GRID) {
        return write_int(dev, "GridMode", 0x55);
    }
    return -1;
}

static int unsigned_pow_get_power(iems_vendor_device *dev, double *power) {
    return read_int_as_double(dev, "ActivePower", power);
}

static int unsigned_pow_get_repower(iems_vendor_device *dev, double *repower) {
    return read_int_as_double(dev, "ReactivePower", repower);
}

static int unsigned_pow_get_mode(iems_vendor_device *dev, int *mode) {
    return read_int(dev, "RunStatus", mode);
}

static int unsigned_pow_get_onoff(iems_vendor_device *dev, int *onoff) {
    return onoff_get(dev, &unsigned_pow_onoff, onoff);
}

/* v3 读侧：GridStatus 0x55 视为离线，其余一律并网（unsigned_pow.c:124-135）。 */
static int unsigned_pow_get_connect(iems_vendor_device *dev, int *connect) {
    int grid_status = 0;
    if (read_int(dev, "GridStatus", &grid_status) != 0) {
        return -1;
    }
    *connect = (grid_status == 0x55) ? IEMS_VENDOR_DISCONNECT_GRID : IEMS_VENDOR_CONNECT_GRID;
    return 0;
}

DECLARE_EXTENDED_METHODS(unsigned_pow);
const iems_vendor_semantics unsigned_pow_ops = {
    .compatible = unsigned_pow_compatible,
    .init = vendor_init,
    .set_power = unsigned_pow_set_power,
    .set_repower = unsigned_pow_set_repower,
    .set_mode = unsigned_pow_set_mode,
    .set_onoff = unsigned_pow_set_onoff,
    .set_connect = unsigned_pow_set_connect,
    .get_power = unsigned_pow_get_power,
    .get_repower = unsigned_pow_get_repower,
    .get_mode = unsigned_pow_get_mode,
    .get_onoff = unsigned_pow_get_onoff,
    .get_connect = unsigned_pow_get_connect,
    .set_value = unsigned_pow_set_value,
    .get_int_value = unsigned_pow_get_int_value,
    .get_double_value = unsigned_pow_get_double_value,
};
DEFINE_EXTENDED_METHODS(unsigned_pow);

/* xiankong（v3 pcs/xiankong_pcs.c:12-126）：功率写 int ActPowSet；
 * set_repower/set_mode 与全部功率读接口为 -1；开写 OnOffSet，启动需 StartReady。
 * 模式/并网读 CurWorkMode：1 离线、2 并网。 */
static const char *const xiankong_compatible[] = {"xiankong", NULL};

static int xiankong_set_power(iems_vendor_device *dev, double power) {
    return write_int(dev, "ActPowSet", (int)power);
}

static int xiankong_set_onoff(iems_vendor_device *dev, int onoff) {
    if (onoff == IEMS_VENDOR_OFF) {
        return write_int(dev, "OnOffSet", 0);
    }
    if (onoff == IEMS_VENDOR_ON) {
        int start_ready = 0;
        if (read_int(dev, "StartReady", &start_ready) != 0) {
            return -1;
        }
        if (start_ready == 0) {
            return -2; /* v3 用 -2 表示启动未就绪（xiankong_pcs.c:37-45） */
        }
        return write_int(dev, "OnOffSet", 1);
    }
    return -1;
}

static int xiankong_set_connect(iems_vendor_device *dev, int connect) {
    if (connect == IEMS_VENDOR_CONNECT_GRID) {
        return write_int(dev, "WorkMode", 1);
    }
    if (connect == IEMS_VENDOR_DISCONNECT_GRID) {
        return write_int(dev, "WorkMode", 0);
    }
    return -1;
}

static int xiankong_get_onoff(iems_vendor_device *dev, int *onoff) {
    int status = 0;
    if (read_int(dev, "ModeOnOffStatus", &status) != 0) {
        return -1;
    }
    if (status == 1) {
        *onoff = IEMS_VENDOR_ON;
        return 0;
    }
    if (status == 0) {
        *onoff = IEMS_VENDOR_OFF;
        return 0;
    }
    return -1;
}

static int xiankong_get_connect(iems_vendor_device *dev, int *connect) {
    int work_mode = 0;
    if (read_int(dev, "CurWorkMode", &work_mode) != 0) {
        return -1;
    }
    if (work_mode == 1) {
        *connect = IEMS_VENDOR_DISCONNECT_GRID;
        return 0;
    }
    if (work_mode == 2) {
        *connect = IEMS_VENDOR_CONNECT_GRID;
        return 0;
    }
    return -1;
}

DECLARE_EXTENDED_METHODS(xiankong);
const iems_vendor_semantics xiankong_ops = {
    .compatible = xiankong_compatible,
    .init = vendor_init,
    .set_power = xiankong_set_power,
    .set_repower = unsupported_set_power,
    .set_mode = unsupported_set_int,
    .set_onoff = xiankong_set_onoff,
    .set_connect = xiankong_set_connect,
    .get_power = unsupported_get_double,
    .get_repower = unsupported_get_double,
    .get_mode = unsupported_get_int,
    .get_onoff = xiankong_get_onoff,
    .get_connect = xiankong_get_connect,
    .set_value = xiankong_set_value,
    .get_int_value = xiankong_get_int_value,
    .get_double_value = xiankong_get_double_value,
};
DEFINE_EXTENDED_METHODS(xiankong);

/* xinai（v3 pcs/xinai_pcs.c:21-107）：标准五函数骨架；功率/无功读回写侧 tag
 * 并按 int 截断；开关写 StartStopCtr 0/1，读 v3 用的 Alarm_3（0/1 严格映射）。 */
static const onoff_convention xinai_onoff = {
    .write_tag = "StartStopCtr",
    .read_tag = "Alarm_3",
    .on_value = 0x0001,
    .off_value = 0x0000,
};

static const std5_tags xinai_tags = {
    .power_set = "ActPowSet",
    .repower_set = "ReactPowSet",
    .power_read = "ActPowSet",
    .repower_read = "ReactPowSet",
    .mode = "WorkMode",
    .connect = "setConnect",
    .onoff = &xinai_onoff,
};

DEFINE_STD5_VENDOR(xinai, xinai_tags);

/* yingbo（v3 pcs/yingbo_100_modbus.c:43-131）：功率/无功固定按三相写各值/3；
 * 开关 PCSStart/PCSStop；Stoppage 非 0 判停机；GridOnOff 非 0 判并网。 */
static const char *const yingbo_compatible[] = {"yingbo", NULL};
static const char *const yingbo_phase_power_set[3] = {
    "ASplitPhaseActPowSet", "BSplitPhaseActPowSet", "CSplitPhaseActPowSet"};
static const char *const yingbo_phase_repower_set[3] = {
    "ASplitPhaseReactPowSet", "BSplitPhaseReactPowSet", "CSplitPhaseReactPowSet"};

static int yingbo_write_split(iems_vendor_device *dev, const char *const tags[3], double total) {
    double phase = total / 3.0;
    for (size_t i = 0; i < 3; i++) {
        if (write_tag(dev, tags[i], phase) != 0) {
            return -1;
        }
    }
    return 0;
}

static int yingbo_set_power(iems_vendor_device *dev, double power) {
    return yingbo_write_split(dev, yingbo_phase_power_set, power);
}

static int yingbo_set_repower(iems_vendor_device *dev, double repower) {
    return yingbo_write_split(dev, yingbo_phase_repower_set, repower);
}

static int yingbo_set_mode(iems_vendor_device *dev, int mode) {
    return write_int(dev, "WorkMode", mode);
}

/* v3 的 switch 没有 default 且总返回 0（yingbo_100_modbus.c:67-79）；
 * 未定义态这里返回 -1 且不写。 */
static int yingbo_set_onoff(iems_vendor_device *dev, int onoff) {
    if (onoff == IEMS_VENDOR_ON) {
        return write_int(dev, "PCSStart", 1);
    }
    if (onoff == IEMS_VENDOR_OFF) {
        return write_int(dev, "PCSStop", 1);
    }
    return -1;
}

/* v3 对未定义 connect 不写且返回 0（yingbo_100_modbus.c:80-87）。 */
static int yingbo_set_connect(iems_vendor_device *dev, int connect) {
    if (connect == IEMS_VENDOR_CONNECT_GRID) {
        return write_int(dev, "SetConnect", 1);
    }
    if (connect == IEMS_VENDOR_DISCONNECT_GRID) {
        return write_int(dev, "SetConnect", 0);
    }
    return -1;
}

static int yingbo_get_power(iems_vendor_device *dev, double *power) {
    return read_int_as_double(dev, "ActivePower", power);
}

static int yingbo_get_repower(iems_vendor_device *dev, double *repower) {
    return read_int_as_double(dev, "ReactivePower", repower);
}

static int yingbo_get_mode(iems_vendor_device *dev, int *mode) {
    return read_int(dev, "RunStatu", mode);
}

static int yingbo_get_onoff(iems_vendor_device *dev, int *onoff) {
    int stoppage = 0;
    if (read_int(dev, "Stoppage", &stoppage) != 0) {
        return -1;
    }
    *onoff = (stoppage != 0) ? IEMS_VENDOR_OFF : IEMS_VENDOR_ON;
    return 0;
}

static int yingbo_get_connect(iems_vendor_device *dev, int *connect) {
    int grid = 0;
    if (read_int(dev, "GridOnOff", &grid) != 0) {
        return -1;
    }
    *connect = (grid != 0) ? IEMS_VENDOR_CONNECT_GRID : IEMS_VENDOR_DISCONNECT_GRID;
    return 0;
}

DECLARE_EXTENDED_METHODS(yingbo);
const iems_vendor_semantics yingbo_ops = {
    .compatible = yingbo_compatible,
    .init = vendor_init,
    .set_power = yingbo_set_power,
    .set_repower = yingbo_set_repower,
    .set_mode = yingbo_set_mode,
    .set_onoff = yingbo_set_onoff,
    .set_connect = yingbo_set_connect,
    .get_power = yingbo_get_power,
    .get_repower = yingbo_get_repower,
    .get_mode = yingbo_get_mode,
    .get_onoff = yingbo_get_onoff,
    .get_connect = yingbo_get_connect,
    .set_value = yingbo_set_value,
    .get_int_value = yingbo_get_int_value,
    .get_double_value = yingbo_get_double_value,
};
DEFINE_EXTENDED_METHODS(yingbo);

/* zhongteng（v3 pcs/zhongteng.c:19-115）：功率/无功写回写侧 tag；模式值 0/1
 * 分别写 GridOff/GridOn 各置 1；开关写 StopCtr/StartCtr；读侧 OnOff/setConnect
 * 原值，模式读 GridModelOn/GridModelOff。 */
static const char *const zhongteng_compatible[] = {"zhongteng", NULL};

static int zhongteng_set_power(iems_vendor_device *dev, double power) {
    return write_tag(dev, "ActPowSet", power);
}

static int zhongteng_set_repower(iems_vendor_device *dev, double repower) {
    return write_tag(dev, "ReactPowSet", repower);
}

static int zhongteng_set_mode(iems_vendor_device *dev, int mode) {
    if (mode == IEMS_VENDOR_ON) {
        return write_int(dev, "GridOn", 1);
    }
    if (mode == IEMS_VENDOR_OFF) {
        return write_int(dev, "GridOff", 1);
    }
    return -1;
}

static int zhongteng_set_onoff(iems_vendor_device *dev, int onoff) {
    if (onoff == IEMS_VENDOR_ON) {
        return write_int(dev, "StartCtr", 1);
    }
    if (onoff == IEMS_VENDOR_OFF) {
        return write_int(dev, "StopCtr", 1);
    }
    return -1;
}

static int zhongteng_set_connect(iems_vendor_device *dev, int connect) {
    return write_int(dev, "setConnect", connect);
}

static int zhongteng_get_power(iems_vendor_device *dev, double *power) {
    return read_int_as_double(dev, "ActPowSet", power);
}

static int zhongteng_get_repower(iems_vendor_device *dev, double *repower) {
    return read_int_as_double(dev, "ReactPowSet", repower);
}

static int zhongteng_get_mode(iems_vendor_device *dev, int *mode) {
    int value = 0;
    if (read_int(dev, "GridModelOn", &value) != 0) {
        return -1;
    }
    if (value == 1) {
        *mode = IEMS_VENDOR_ON;
        return 0;
    }
    if (read_int(dev, "GridModelOff", &value) != 0) {
        return -1;
    }
    if (value == 1) {
        *mode = IEMS_VENDOR_OFF;
        return 0;
    }
    return -1;
}

static int zhongteng_get_onoff(iems_vendor_device *dev, int *onoff) {
    return read_int(dev, "OnOff", onoff);
}

static int zhongteng_get_connect(iems_vendor_device *dev, int *connect) {
    return read_int(dev, "setConnect", connect);
}

DECLARE_EXTENDED_METHODS(zhongteng);
const iems_vendor_semantics zhongteng_ops = {
    .compatible = zhongteng_compatible,
    .init = vendor_init,
    .set_power = zhongteng_set_power,
    .set_repower = zhongteng_set_repower,
    .set_mode = zhongteng_set_mode,
    .set_onoff = zhongteng_set_onoff,
    .set_connect = zhongteng_set_connect,
    .get_power = zhongteng_get_power,
    .get_repower = zhongteng_get_repower,
    .get_mode = zhongteng_get_mode,
    .get_onoff = zhongteng_get_onoff,
    .get_connect = zhongteng_get_connect,
    .set_value = zhongteng_set_value,
    .get_int_value = zhongteng_get_int_value,
    .get_double_value = zhongteng_get_double_value,
};
DEFINE_EXTENDED_METHODS(zhongteng);
