#include "iems_vendor_semantics_internal.h"

/* ------------------------------------------------------------------------- */
/* 1. 厂商适配器（P3-00 已注册：coepo、inpower、gsstes、kelong、xinhuasan）    */
/*                                                                           */
/*    厂商 vtable 复用已钉住的策略方法，只换 compatible；不再复制一套语义。    */
/* ------------------------------------------------------------------------- */

static const char *const coepo_compatible[] = {"coepo", NULL};

/* v3 coepo 的 get_int_value 把 get_mode/get_connect 误派到 get_onoff
 * （pcs/coepo_pcs.c:113-131）；vtable 与扩展派发都按同名方法。 */
DECLARE_EXTENDED_METHODS(coepo);
const iems_vendor_semantics coepo_ops = {
    .compatible = coepo_compatible,
    .init = vendor_init,
    .set_power = standard_set_power,
    .set_repower = standard_set_repower,
    .set_mode = standard_set_mode,
    .set_onoff = standard_set_onoff,
    .set_connect = standard_set_connect,
    .get_power = standard_get_power,
    .get_repower = standard_get_repower,
    .get_mode = standard_get_mode,
    .get_onoff = standard_get_onoff,
    .get_connect = standard_get_connect,
    .set_value = coepo_set_value,
    .get_int_value = coepo_get_int_value,
    .get_double_value = coepo_get_double_value,
};
DEFINE_EXTENDED_METHODS(coepo);

static const char *const inpower_compatible[] = {"inpower", "inpower-3p4w", "inpower_x3",
                                                 "inpower_x4", NULL};

/* v3 inpower 的 set_mode 空实现、get_mode/get_connect 不写输出
 * （pcs/inpower_pcs.c:97-100,146-169）；按 vtable 约定返回 -1。 */
DECLARE_EXTENDED_METHODS(inpower);
const iems_vendor_semantics inpower_ops = {
    .compatible = inpower_compatible,
    .init = vendor_init,
    .set_power = alias_set_power,
    .set_repower = alias_set_repower,
    .set_mode = unsupported_set_int,
    .set_onoff = alias_set_onoff,
    .set_connect = alias_set_connect,
    .get_power = alias_get_power,
    .get_repower = alias_get_repower,
    .get_mode = unsupported_get_int,
    .get_onoff = alias_get_onoff,
    .get_connect = unsupported_get_int,
    .set_value = inpower_set_value,
    .get_int_value = inpower_get_int_value,
    .get_double_value = inpower_get_double_value,
};
DEFINE_EXTENDED_METHODS(inpower);

/* gsstes/kelong 共用标准五函数骨架，差异只在功率读 tag 与开关量约定。
 * v3 两家 set_connect 原样写 1/0（adapter_define.h:15-16），不像 coepo 反相。 */

static const std5_tags gsstes_tags = {
    .power_set = "ActPowSet",
    .repower_set = "ReactPowSet",
    .power_read = "ActivePower",
    .repower_read = "ReactivePower",
    .mode = "WorkMode",
    .connect = "setConnect",
    .onoff = &onoff_00ff,
};

static const std5_tags kelong_tags = {
    .power_set = "ActPowSet",
    .repower_set = "ReactPowSet",
    .power_read = "ActPowSet",
    .repower_read = "ReactPowSet",
    .mode = "WorkMode",
    .connect = "setConnect",
    .onoff = &onoff_01,
};

DEFINE_STD5_VENDOR(gsstes, gsstes_tags);
DEFINE_STD5_VENDOR(kelong, kelong_tags);

/* v3 xinhuasan 下标 0 写总功率、下标 1（3p4w）写 A/B/C 分相各值/3；
 * 三相读逐相覆盖返回 C 相，不累加（pcs/xinhuasan.c:46-142）。 */
static const char *const xinhuasan_compatible[] = {"xinhuasan", "xinhuasan-3p4w", NULL};

DECLARE_EXTENDED_METHODS(xinhuasan);
const iems_vendor_semantics xinhuasan_ops = {
    .compatible = xinhuasan_compatible,
    .init = vendor_init,
    .set_power = split_set_power,
    .set_repower = split_set_repower,
    .set_mode = split_set_mode,
    .set_onoff = split_set_onoff,
    .set_connect = split_set_connect,
    .get_power = split_get_power,
    .get_repower = split_get_repower,
    .get_mode = split_get_mode,
    .get_onoff = split_get_onoff,
    .get_connect = split_get_connect,
    .set_value = xinhuasan_set_value,
    .get_int_value = xinhuasan_get_int_value,
    .get_double_value = xinhuasan_get_double_value,
};
DEFINE_EXTENDED_METHODS(xinhuasan);

/* ------------------------------------------------------------------------- */
/* 2. 厂商适配器（P3-00 批次 A：enjoy、feilton、huazhi、inovance、nengfeng）  */
/* ------------------------------------------------------------------------- */

static const char *const enjoy_compatible[] = {"enjoy", NULL};

/* v3 enjoy 开关机带联锁：Online 在线且 SetGridModel 为 0 才写 StartStopCtr
 * （pcs/enjoy_pcs.c:30-57）；联锁失败返回 -1，v3 的 -9999 不照抄。 */
static int enjoy_set_onoff(iems_vendor_device *dev, int onoff) {
    int online = 0;
    int grid_model = 0;
    if (read_int(dev, "Online", &online) != 0 || online == 0) {
        return -1;
    }
    if (read_int(dev, "SetGridModel", &grid_model) != 0 || grid_model != 0) {
        return -1;
    }
    return onoff_set(dev, &onoff_01, onoff);
}

static int enjoy_set_power(iems_vendor_device *dev, double power) {
    return write_tag(dev, "ActPowSet", power);
}

static int enjoy_set_repower(iems_vendor_device *dev, double repower) {
    return write_tag(dev, "ReactPowSet", repower);
}

static int enjoy_set_mode(iems_vendor_device *dev, int mode) {
    return write_int(dev, "WorkMode", mode);
}

/* v3 用 int 访问器读回设定值（pcs/enjoy_pcs.c:64-74），显式截断。 */
static int enjoy_get_power(iems_vendor_device *dev, double *power) {
    return read_int_as_double(dev, "ActPowSet", power);
}

static int enjoy_get_repower(iems_vendor_device *dev, double *repower) {
    return read_int_as_double(dev, "ReactPowSet", repower);
}

static int enjoy_get_mode(iems_vendor_device *dev, int *mode) {
    return read_int(dev, "WorkMode", mode);
}

static int enjoy_get_onoff(iems_vendor_device *dev, int *onoff) {
    return onoff_get(dev, &onoff_01, onoff);
}

DECLARE_EXTENDED_METHODS(enjoy);
const iems_vendor_semantics enjoy_ops = {
    .compatible = enjoy_compatible,
    .init = vendor_init,
    .set_power = enjoy_set_power,
    .set_repower = enjoy_set_repower,
    .set_mode = enjoy_set_mode,
    .set_onoff = enjoy_set_onoff,
    .set_connect = standard_set_connect,
    .get_power = enjoy_get_power,
    .get_repower = enjoy_get_repower,
    .get_mode = enjoy_get_mode,
    .get_onoff = enjoy_get_onoff,
    .get_connect = standard_get_connect,
    .set_value = enjoy_set_value,
    .get_int_value = enjoy_get_int_value,
    .get_double_value = enjoy_get_double_value,
};
DEFINE_EXTENDED_METHODS(enjoy);

static const char *const feilton_compatible[] = {"feilton", NULL};

/* v3 按 SetPwrMode 选择单点/分相给定：0 写 WrtActPowSet，512 三相各写 1/3，
 * 其他值拒绝（pcs/feilton_pcs.c:10-29）。 */
static int feilton_set_power(iems_vendor_device *dev, double power) {
    int pwr_mode = 0;
    if (read_int(dev, "SetPwrMode", &pwr_mode) != 0) {
        return -1;
    }
    if (pwr_mode == 0) {
        return write_tag(dev, "WrtActPowSet", power);
    }
    if (pwr_mode != 512) {
        return -1;
    }
    double phase = power / 3.0;
    if (write_tag(dev, "WrtActPowSetA", phase) != 0) {
        return -1;
    }
    if (write_tag(dev, "WrtActPowSetB", phase) != 0) {
        return -1;
    }
    return write_tag(dev, "WrtActPowSetC", phase);
}

/* v3 启动前要求 StartReady == 1，否则 -2 且不写（pcs/feilton_pcs.c:41-66）。 */
static int feilton_set_onoff(iems_vendor_device *dev, int onoff) {
    if (onoff == IEMS_VENDOR_OFF) {
        return write_int(dev, "Stop", 5461072);
    }
    if (onoff != IEMS_VENDOR_ON) {
        return -1;
    }
    int ready = 0;
    if (read_int(dev, "StartReady", &ready) != 0 || ready != 1) {
        return -1;
    }
    return write_int(dev, "Start", 5395790);
}

/* v3 StopState 0 表示运行、1 表示停机（pcs/feilton_pcs.c:88-106）。 */
static int feilton_get_onoff(iems_vendor_device *dev, int *onoff) {
    int state = 0;
    if (read_int(dev, "StopState", &state) != 0) {
        return -1;
    }
    if (state == 0) {
        *onoff = IEMS_VENDOR_ON;
        return 0;
    }
    if (state == 1) {
        *onoff = IEMS_VENDOR_OFF;
        return 0;
    }
    return -1;
}

DECLARE_EXTENDED_METHODS(feilton);
const iems_vendor_semantics feilton_ops = {
    .compatible = feilton_compatible,
    .init = vendor_init,
    .set_power = feilton_set_power,
    .set_repower = unsupported_set_repower,
    .set_mode = unsupported_set_int,
    .set_onoff = feilton_set_onoff,
    .set_connect = unsupported_set_int,
    .get_power = unsupported_get_double,
    .get_repower = unsupported_get_double,
    .get_mode = unsupported_get_int,
    .get_onoff = feilton_get_onoff,
    .get_connect = unsupported_get_int,
    .set_value = feilton_set_value,
    .get_int_value = feilton_get_int_value,
    .get_double_value = feilton_get_double_value,
};
DEFINE_EXTENDED_METHODS(feilton);

static const char *const huazhi_compatible[] = {"RTC", NULL};

/* v3 写 OnOff（21845 停 / 43690 开），读 StartStopCtr（0 停 / 2..6 开）
 * （pcs/huazhi_pcs.c:39-57,82-105）。 */
static int huazhi_set_onoff(iems_vendor_device *dev, int onoff) {
    if (onoff == IEMS_VENDOR_ON) {
        return write_int(dev, "OnOff", 43690);
    }
    if (onoff == IEMS_VENDOR_OFF) {
        return write_int(dev, "OnOff", 21845);
    }
    return -1;
}

static int huazhi_get_onoff(iems_vendor_device *dev, int *onoff) {
    int state = 0;
    if (read_int(dev, "StartStopCtr", &state) != 0) {
        return -1;
    }
    if (state == 0) {
        *onoff = IEMS_VENDOR_OFF;
        return 0;
    }
    if (state >= 2 && state <= 6) {
        *onoff = IEMS_VENDOR_ON;
        return 0;
    }
    return -1;
}

static int huazhi_set_power(iems_vendor_device *dev, double power) {
    return write_tag(dev, "ActPowSet", power);
}

static int huazhi_set_repower(iems_vendor_device *dev, double repower) {
    return write_tag(dev, "ReactPowSet", repower);
}

static int huazhi_set_mode(iems_vendor_device *dev, int mode) {
    return write_int(dev, "WorkMode", mode);
}

static int huazhi_set_connect(iems_vendor_device *dev, int connect) {
    return write_int(dev, "setConnect", connect);
}

/* v3 用 int 访问器读回功率（pcs/huazhi_pcs.c:64-74），显式截断。 */
static int huazhi_get_power(iems_vendor_device *dev, double *power) {
    return read_int_as_double(dev, "ActPowSet", power);
}

static int huazhi_get_repower(iems_vendor_device *dev, double *repower) {
    return read_int_as_double(dev, "ReactPowSet", repower);
}

static int huazhi_get_mode(iems_vendor_device *dev, int *mode) {
    return read_int(dev, "WorkMode", mode);
}

DECLARE_EXTENDED_METHODS(huazhi);
const iems_vendor_semantics huazhi_ops = {
    .compatible = huazhi_compatible,
    .init = vendor_init,
    .set_power = huazhi_set_power,
    .set_repower = huazhi_set_repower,
    .set_mode = huazhi_set_mode,
    .set_onoff = huazhi_set_onoff,
    .set_connect = huazhi_set_connect,
    .get_power = huazhi_get_power,
    .get_repower = huazhi_get_repower,
    .get_mode = huazhi_get_mode,
    .get_onoff = huazhi_get_onoff,
    .get_connect = standard_get_connect,
    .set_value = huazhi_set_value,
    .get_int_value = huazhi_get_int_value,
    .get_double_value = huazhi_get_double_value,
};
DEFINE_EXTENDED_METHODS(huazhi);

static const char *const inovance_compatible[] = {"inovance", NULL};

/* v3 在下发功率前若 RunStatu 为 0（stopped）先写 StartCtr/StandbyCtr 各 1；
 * 这是 v3 注释自述的启动流程猜测，失败不阻断功率下发
 * （pcs/inovance_pcs.c:19-47）。 */
static void inovance_ensure_started(iems_vendor_device *dev) {
    int run_status = 0;
    if (read_int(dev, "RunStatu", &run_status) != 0 || run_status != 0) {
        return;
    }
    (void)write_int(dev, "StartCtr", 1);
    (void)write_int(dev, "StandbyCtr", 1);
}

static int inovance_set_power(iems_vendor_device *dev, double power) {
    inovance_ensure_started(dev);
    return write_tag(dev, "ActPowSet", power);
}

static int inovance_set_repower(iems_vendor_device *dev, double repower) {
    inovance_ensure_started(dev);
    return write_tag(dev, "ReactPowSet", repower);
}

static int inovance_set_mode(iems_vendor_device *dev, int mode) {
    return write_int(dev, "WorkMode", mode);
}

static int inovance_set_onoff(iems_vendor_device *dev, int onoff) {
    return write_int(dev, "StartCtr", onoff);
}

static int inovance_get_power(iems_vendor_device *dev, double *power) {
    return read_int_as_double(dev, "ActPowSet", power);
}

static int inovance_get_repower(iems_vendor_device *dev, double *repower) {
    return read_int_as_double(dev, "ReactPowSet", repower);
}

static int inovance_get_mode(iems_vendor_device *dev, int *mode) {
    return read_int(dev, "WorkMode", mode);
}

/* v3 get_onoff 直接返回 RunStatu 原始枚举（0 stopped / 1 waited / 2 dischg /
 * 3 chg / 4 zero_run），违反 vtable 的 OFF/ON/STANDBY 约定；这里做映射。 */
static int inovance_get_onoff(iems_vendor_device *dev, int *onoff) {
    int run_status = 0;
    if (read_int(dev, "RunStatu", &run_status) != 0) {
        return -1;
    }
    switch (run_status) {
        case 0: /* stopped */
            *onoff = IEMS_VENDOR_OFF;
            return 0;
        case 1: /* waited */
            *onoff = IEMS_VENDOR_STANDBY;
            return 0;
        case 2: /* dischg */
        case 3: /* chg */
        case 4: /* zero_run */
            *onoff = IEMS_VENDOR_ON;
            return 0;
        default:
            return -1;
    }
}

DECLARE_EXTENDED_METHODS(inovance);
const iems_vendor_semantics inovance_ops = {
    .compatible = inovance_compatible,
    .init = vendor_init,
    .set_power = inovance_set_power,
    .set_repower = inovance_set_repower,
    .set_mode = inovance_set_mode,
    .set_onoff = inovance_set_onoff,
    /* v3 set_connect 空实现却返回 0（pcs/inovance_pcs.c:62-66），按约定返回 -1。 */
    .set_connect = unsupported_set_int,
    .get_power = inovance_get_power,
    .get_repower = inovance_get_repower,
    .get_mode = inovance_get_mode,
    .get_onoff = inovance_get_onoff,
    .get_connect = standard_get_connect,
    .set_value = inovance_set_value,
    .get_int_value = inovance_get_int_value,
    .get_double_value = inovance_get_double_value,
};
DEFINE_EXTENDED_METHODS(inovance);

static const char *const nengfeng_compatible[] = {"nengfeng", NULL};

/* v3 开关机要求 Online 在线（pcs/nengfeng_pcs.c:33-60），失败返回 -1。 */
static int nengfeng_set_onoff(iems_vendor_device *dev, int onoff) {
    int online = 0;
    if (read_int(dev, "Online", &online) != 0 || online == 0) {
        return -1;
    }
    return onoff_set(dev, &onoff_00ff, onoff);
}

static int nengfeng_set_power(iems_vendor_device *dev, double power) {
    return write_tag(dev, "ActPowSet", power);
}

static int nengfeng_set_repower(iems_vendor_device *dev, double repower) {
    return write_tag(dev, "ReactPowSet", repower);
}

static int nengfeng_set_mode(iems_vendor_device *dev, int mode) {
    return write_int(dev, "WorkMode", mode);
}

/* v3 用 int 访问器读回功率（pcs/nengfeng_pcs.c:67-77），显式截断。 */
static int nengfeng_get_power(iems_vendor_device *dev, double *power) {
    return read_int_as_double(dev, "ActPowSet", power);
}

static int nengfeng_get_repower(iems_vendor_device *dev, double *repower) {
    return read_int_as_double(dev, "ReactPowSet", repower);
}

static int nengfeng_get_mode(iems_vendor_device *dev, int *mode) {
    return read_int(dev, "WorkMode", mode);
}

static int nengfeng_get_onoff(iems_vendor_device *dev, int *onoff) {
    return onoff_get(dev, &onoff_00ff, onoff);
}

DECLARE_EXTENDED_METHODS(nengfeng);
const iems_vendor_semantics nengfeng_ops = {
    .compatible = nengfeng_compatible,
    .init = vendor_init,
    .set_power = nengfeng_set_power,
    .set_repower = nengfeng_set_repower,
    .set_mode = nengfeng_set_mode,
    .set_onoff = nengfeng_set_onoff,
    .set_connect = standard_set_connect,
    .get_power = nengfeng_get_power,
    .get_repower = nengfeng_get_repower,
    .get_mode = nengfeng_get_mode,
    .get_onoff = nengfeng_get_onoff,
    .get_connect = standard_get_connect,
    .set_value = nengfeng_set_value,
    .get_int_value = nengfeng_get_int_value,
    .get_double_value = nengfeng_get_double_value,
};
DEFINE_EXTENDED_METHODS(nengfeng);
