#include "iems_vendor_semantics_internal.h"

int read_tag(const iems_vendor_device *dev, const char *tag_name, double *value) {
    if (dev == NULL || dev->io == NULL || dev->io->read_tag == NULL) {
        return -1;
    }
    return dev->io->read_tag(dev->ctx, dev->dev_id, tag_name, value);
}

int write_tag(const iems_vendor_device *dev, const char *tag_name, double value) {
    if (dev == NULL || dev->io == NULL || dev->io->write_tag == NULL) {
        return -1;
    }
    return dev->io->write_tag(dev->ctx, dev->dev_id, tag_name, value);
}

/* 端口统一 double；整型寄存器（开关量、模式）在这里显式截断。 */
int read_int(const iems_vendor_device *dev, const char *tag_name, int *value) {
    double raw = 0.0;
    if (read_tag(dev, tag_name, &raw) != 0) {
        return -1;
    }
    *value = (int)raw;
    return 0;
}

int write_int(const iems_vendor_device *dev, const char *tag_name, int value) {
    return write_tag(dev, tag_name, (double)value);
}

/* v3 用 int 访问器读功率（gsstes_pcs.c:64-74, kelong_pcs.c:64-74）；端口是
 * double，这里显式截断，避免隐式转换悄悄丢小数。 */
int read_int_as_double(const iems_vendor_device *dev, const char *tag_name, double *value) {
    double raw = 0.0;
    if (read_tag(dev, tag_name, &raw) != 0) {
        return -1;
    }
    *value = (double)(int)raw;
    return 0;
}
int vendor_init(iems_vendor_device *dev, void *par) {
    (void)dev;
    (void)par;
    return 0;
}

int unsupported_set_power(iems_vendor_device *dev, double value) {
    (void)dev;
    (void)value;
    return -1;
}

int unsupported_set_repower(iems_vendor_device *dev, double value) {
    (void)dev;
    (void)value;
    return -1;
}

int unsupported_set_int(iems_vendor_device *dev, int value) {
    (void)dev;
    (void)value;
    return -1;
}

int unsupported_get_double(iems_vendor_device *dev, double *value) {
    (void)dev;
    (void)value;
    return -1;
}

int unsupported_get_int(iems_vendor_device *dev, int *value) {
    (void)dev;
    (void)value;
    return -1;
}
int dispatch_set_value(const iems_vendor_semantics *ops, iems_vendor_device *dev,
                       const char *function, double value) {
    if (function == NULL) {
        return -1;
    }
    if (strcmp(function, IEMS_VENDOR_FN_SET_POWER) == 0) {
        return ops->set_power(dev, value);
    }
    if (strcmp(function, IEMS_VENDOR_FN_SET_REPOWER) == 0) {
        return ops->set_repower(dev, value);
    }
    if (strcmp(function, IEMS_VENDOR_FN_SET_MODE) == 0) {
        return ops->set_mode(dev, (int)value);
    }
    if (strcmp(function, IEMS_VENDOR_FN_SET_ONOFF) == 0) {
        return ops->set_onoff(dev, (int)value);
    }
    if (strcmp(function, IEMS_VENDOR_FN_SET_CONNECT) == 0) {
        return ops->set_connect(dev, (int)value);
    }
    return -1;
}

int dispatch_get_int_value(const iems_vendor_semantics *ops, iems_vendor_device *dev,
                           const char *function, int *value) {
    if (function == NULL) {
        return -1;
    }
    if (strcmp(function, IEMS_VENDOR_FN_GET_MODE) == 0) {
        return ops->get_mode(dev, value);
    }
    if (strcmp(function, IEMS_VENDOR_FN_GET_ONOFF) == 0) {
        return ops->get_onoff(dev, value);
    }
    if (strcmp(function, IEMS_VENDOR_FN_GET_CONNECT) == 0) {
        return ops->get_connect(dev, value);
    }
    return -1;
}

int dispatch_get_double_value(const iems_vendor_semantics *ops, iems_vendor_device *dev,
                              const char *function, double *value) {
    if (function == NULL) {
        return -1;
    }
    if (strcmp(function, IEMS_VENDOR_FN_GET_POWER) == 0) {
        return ops->get_power(dev, value);
    }
    if (strcmp(function, IEMS_VENDOR_FN_GET_REPOWER) == 0) {
        return ops->get_repower(dev, value);
    }
    return -1;
}

/* ------------------------------------------------------------------------- */
/* 1. 标准 PCS 五函数（v3 pcs/coepo_pcs.c:9-81）                              */
/* ------------------------------------------------------------------------- */

static const struct {
    const char *power;
    const char *repower;
    const char *mode;
    const char *onoff;
    const char *connect;
} standard_tags = {
    .power = "OutPowerSet",
    .repower = "OutReactivePower",
    .mode = "WorkMode",
    .onoff = "StopCtr",
    .connect = "setConnect",
};

int standard_set_power(iems_vendor_device *dev, double power) {
    return write_tag(dev, standard_tags.power, power);
}

int standard_set_repower(iems_vendor_device *dev, double repower) {
    return write_tag(dev, standard_tags.repower, repower);
}

int standard_set_mode(iems_vendor_device *dev, int mode) {
    return write_int(dev, standard_tags.mode, mode);
}

int standard_set_onoff(iems_vendor_device *dev, int onoff) {
    return write_int(dev, standard_tags.onoff, onoff);
}

int standard_set_connect(iems_vendor_device *dev, int connect) {
    if (connect == IEMS_VENDOR_CONNECT_GRID) {
        return write_int(dev, standard_tags.connect, 0);
    }
    if (connect == IEMS_VENDOR_DISCONNECT_GRID) {
        return write_int(dev, standard_tags.connect, 1);
    }
    return -1;
}

int standard_get_power(iems_vendor_device *dev, double *power) {
    return read_tag(dev, standard_tags.power, power);
}

int standard_get_repower(iems_vendor_device *dev, double *repower) {
    return read_tag(dev, standard_tags.repower, repower);
}

int standard_get_mode(iems_vendor_device *dev, int *mode) {
    return read_int(dev, standard_tags.mode, mode);
}

int standard_get_onoff(iems_vendor_device *dev, int *onoff) {
    int value = 0;
    if (read_int(dev, standard_tags.onoff, &value) != 0) {
        return -1;
    }
    if (value == 0) {
        *onoff = IEMS_VENDOR_OFF;
        return 0;
    }
    if (value == 1) {
        *onoff = IEMS_VENDOR_ON;
        return 0;
    }
    return -1;
}

/* v3 coepo 写入时反相（并网写 0），读取却返回原值；这里保持 v3 读法。 */
int standard_get_connect(iems_vendor_device *dev, int *connect) {
    return read_int(dev, standard_tags.connect, connect);
}

static const char *const standard_compatible[] = {"pcs_standard", NULL};
DECLARE_EXTENDED_METHODS(pcs_standard);
static const iems_vendor_semantics pcs_standard_ops = {
    .compatible = standard_compatible,
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
    .set_value = pcs_standard_set_value,
    .get_int_value = pcs_standard_get_int_value,
    .get_double_value = pcs_standard_get_double_value,
};
DEFINE_EXTENDED_METHODS(pcs_standard);

/* ------------------------------------------------------------------------- */
/* 2. 别名/下标策略（v3 pcs/inpower_pcs.c:43-91,238）                         */
/*                                                                           */
/*    compatible_index 决定分相还是单点：0/3 分相，1/2 单点。                 */
/* ------------------------------------------------------------------------- */

static const char *const alias_compatible[] = {"pcs_alias_index", "pcs_alias_index-3p4w",
                                               "pcs_alias_index_x3", "pcs_alias_index_x4", NULL};
static const int alias_split_by_index[] = {1, 0, 0, 1};

static const struct {
    const char *split_power[3];
    const char *split_repower[3];
    const char *single_power;
    const char *single_repower;
    const char *power_read;
    const char *repower_read;
    const char *fault_run;
    const char *fault_stop;
    const char *run_status;
    const char *stoppage;
    const char *grid_mode;
} alias_tags = {
    .split_power = {"ASplitPhaseActPowSet", "BSplitPhaseActPowSet", "CSplitPhaseActPowSet"},
    .split_repower = {"ASplitPhaseReactPowSet", "BSplitPhaseReactPowSet", "CSplitPhaseReactPowSet"},
    .single_power = "ActPowSet",
    .single_repower = "ReactPowSet",
    .power_read = "ActivePower",
    .repower_read = "ReactivePower",
    .fault_run = "FaultRun",
    .fault_stop = "FaultStop",
    .run_status = "RunStatu",
    .stoppage = "Stoppage",
    .grid_mode = "SetGridModel",
};

static int alias_index_valid(const iems_vendor_device *dev) {
    return dev->compatible_index >= 0 &&
           dev->compatible_index < (int)ARRAY_LEN(alias_split_by_index);
}

int alias_set_power(iems_vendor_device *dev, double power) {
    if (!alias_index_valid(dev)) {
        return -1;
    }
    if (!alias_split_by_index[dev->compatible_index]) {
        return write_tag(dev, alias_tags.single_power, power);
    }
    double phase = power / 3.0;
    for (size_t i = 0; i < ARRAY_LEN(alias_tags.split_power); i++) {
        if (write_tag(dev, alias_tags.split_power[i], phase) != 0) {
            return -1;
        }
    }
    return 0;
}

int alias_set_repower(iems_vendor_device *dev, double repower) {
    if (!alias_index_valid(dev)) {
        return -1;
    }
    if (!alias_split_by_index[dev->compatible_index]) {
        return write_tag(dev, alias_tags.single_repower, repower);
    }
    double phase = repower / 3.0;
    for (size_t i = 0; i < ARRAY_LEN(alias_tags.split_repower); i++) {
        if (write_tag(dev, alias_tags.split_repower[i], phase) != 0) {
            return -1;
        }
    }
    return 0;
}

int alias_set_onoff(iems_vendor_device *dev, int onoff) {
    if (onoff == IEMS_VENDOR_ON) {
        return write_int(dev, alias_tags.fault_run, 1);
    }
    if (onoff == IEMS_VENDOR_OFF) {
        return write_int(dev, alias_tags.fault_stop, 1);
    }
    return -1;
}

int alias_set_connect(iems_vendor_device *dev, int connect) {
    if (connect == IEMS_VENDOR_CONNECT_GRID) {
        return write_int(dev, alias_tags.grid_mode, 0);
    }
    if (connect == IEMS_VENDOR_DISCONNECT_GRID) {
        return write_int(dev, alias_tags.grid_mode, 1);
    }
    return -1;
}

int alias_get_power(iems_vendor_device *dev, double *power) {
    return read_tag(dev, alias_tags.power_read, power);
}

int alias_get_repower(iems_vendor_device *dev, double *repower) {
    return read_tag(dev, alias_tags.repower_read, repower);
}

int alias_get_onoff(iems_vendor_device *dev, int *onoff) {
    int run_status = 0;
    int stoppage = 0;
    if (read_int(dev, alias_tags.run_status, &run_status) != 0) {
        return -1;
    }
    if (run_status != 0) {
        *onoff = IEMS_VENDOR_ON;
        return 0;
    }
    if (read_int(dev, alias_tags.stoppage, &stoppage) != 0) {
        return -1;
    }
    *onoff = (stoppage != 0) ? IEMS_VENDOR_OFF : IEMS_VENDOR_STANDBY;
    return 0;
}

DECLARE_EXTENDED_METHODS(pcs_alias_index);
static const iems_vendor_semantics pcs_alias_index_ops = {
    .compatible = alias_compatible,
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
    .set_value = pcs_alias_index_set_value,
    .get_int_value = pcs_alias_index_get_int_value,
    .get_double_value = pcs_alias_index_get_double_value,
};
DEFINE_EXTENDED_METHODS(pcs_alias_index);

/* ------------------------------------------------------------------------- */
/* 3. 三相拆分策略（v3 pcs/xinhuasan.c:46-142）                               */
/*                                                                           */
/*    下标 0 单点写读；下标 1 分相写，读侧逐相覆盖，最终返回 C 相值，不累加。 */
/* ------------------------------------------------------------------------- */

static const char *const split_compatible[] = {"pcs_split3", "pcs_split3-3p4w", NULL};

static const struct {
    const char *power_set;
    const char *repower_set;
    const char *power_read;
    const char *repower_read;
    const char *phase_power_set[3];
    const char *phase_repower_set[3];
    const char *phase_power_read[3];
    const char *phase_repower_read[3];
    const char *mode_set;
    const char *mode_read;
    const char *connect_set;
    const char *connect_read;
    const char *start_set;
    const char *stop_set;
    const char *onoff_read;
} split_tags = {
    .power_set = "ActPowSet",
    .repower_set = "ReactPowSet",
    .power_read = "ActivePower",
    .repower_read = "ReactivePower",
    .phase_power_set = {"ASplitPhaseActPowSet", "BSplitPhaseActPowSet", "CSplitPhaseActPowSet"},
    .phase_repower_set = {"ASplitPhaseReactPowSet", "BSplitPhaseReactPowSet",
                          "CSplitPhaseReactPowSet"},
    .phase_power_read = {"AphaseActPower", "BphaseActPower", "CphaseActPower"},
    .phase_repower_read = {"AphaseReactivePower", "BphaseReactivePower", "CphaseReactivePower"},
    .mode_set = "WorkMode",
    .mode_read = "RunStatu",
    .connect_set = "SetConnect",
    .connect_read = "GridOnOff",
    .start_set = "PCSStart",
    .stop_set = "PCSStop",
    .onoff_read = "Stoppage",
};

static int split_is_three_phase(const iems_vendor_device *dev) {
    return dev->compatible_index == 1;
}

int split_set_power(iems_vendor_device *dev, double power) {
    if (dev->compatible_index < 0 || dev->compatible_index > 1) {
        return -1;
    }
    if (!split_is_three_phase(dev)) {
        return write_tag(dev, split_tags.power_set, power);
    }
    double phase = power / 3.0;
    for (size_t i = 0; i < ARRAY_LEN(split_tags.phase_power_set); i++) {
        if (write_tag(dev, split_tags.phase_power_set[i], phase) != 0) {
            return -1;
        }
    }
    return 0;
}

int split_set_repower(iems_vendor_device *dev, double repower) {
    if (dev->compatible_index < 0 || dev->compatible_index > 1) {
        return -1;
    }
    if (!split_is_three_phase(dev)) {
        return write_tag(dev, split_tags.repower_set, repower);
    }
    double phase = repower / 3.0;
    for (size_t i = 0; i < ARRAY_LEN(split_tags.phase_repower_set); i++) {
        if (write_tag(dev, split_tags.phase_repower_set[i], phase) != 0) {
            return -1;
        }
    }
    return 0;
}

int split_set_mode(iems_vendor_device *dev, int mode) {
    return write_int(dev, split_tags.mode_set, mode);
}

int split_set_onoff(iems_vendor_device *dev, int onoff) {
    if (onoff == IEMS_VENDOR_ON) {
        return write_int(dev, split_tags.start_set, 1);
    }
    if (onoff == IEMS_VENDOR_OFF) {
        return write_int(dev, split_tags.stop_set, 1);
    }
    return -1;
}

int split_set_connect(iems_vendor_device *dev, int connect) {
    if (connect == IEMS_VENDOR_CONNECT_GRID) {
        return write_int(dev, split_tags.connect_set, 0);
    }
    if (connect == IEMS_VENDOR_DISCONNECT_GRID) {
        return write_int(dev, split_tags.connect_set, 1);
    }
    return -1;
}

int split_get_power(iems_vendor_device *dev, double *power) {
    if (dev->compatible_index < 0 || dev->compatible_index > 1) {
        return -1;
    }
    if (!split_is_three_phase(dev)) {
        return read_tag(dev, split_tags.power_read, power);
    }
    double value = 0.0;
    if (read_tag(dev, split_tags.phase_power_read[0], &value) != 0 ||
        read_tag(dev, split_tags.phase_power_read[1], &value) != 0 ||
        read_tag(dev, split_tags.phase_power_read[2], &value) != 0) {
        return -1;
    }
    /* v3 逐相覆盖而不是累加，最后一相（C）才是结果。 */
    *power = value;
    return 0;
}

int split_get_repower(iems_vendor_device *dev, double *repower) {
    if (dev->compatible_index < 0 || dev->compatible_index > 1) {
        return -1;
    }
    if (!split_is_three_phase(dev)) {
        return read_tag(dev, split_tags.repower_read, repower);
    }
    double value = 0.0;
    if (read_tag(dev, split_tags.phase_repower_read[0], &value) != 0 ||
        read_tag(dev, split_tags.phase_repower_read[1], &value) != 0 ||
        read_tag(dev, split_tags.phase_repower_read[2], &value) != 0) {
        return -1;
    }
    *repower = value;
    return 0;
}

int split_get_mode(iems_vendor_device *dev, int *mode) {
    return read_int(dev, split_tags.mode_read, mode);
}

int split_get_onoff(iems_vendor_device *dev, int *onoff) {
    int stoppage = 0;
    if (read_int(dev, split_tags.onoff_read, &stoppage) != 0) {
        return -1;
    }
    *onoff = (stoppage != 0) ? IEMS_VENDOR_OFF : IEMS_VENDOR_ON;
    return 0;
}

int split_get_connect(iems_vendor_device *dev, int *connect) {
    int grid = 0;
    if (read_int(dev, split_tags.connect_read, &grid) != 0) {
        return -1;
    }
    *connect = (grid != 0) ? IEMS_VENDOR_DISCONNECT_GRID : IEMS_VENDOR_CONNECT_GRID;
    return 0;
}

DECLARE_EXTENDED_METHODS(pcs_split3);
static const iems_vendor_semantics pcs_split3_ops = {
    .compatible = split_compatible,
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
    .set_value = pcs_split3_set_value,
    .get_int_value = pcs_split3_get_int_value,
    .get_double_value = pcs_split3_get_double_value,
};
DEFINE_EXTENDED_METHODS(pcs_split3);

/* ------------------------------------------------------------------------- */
/* 4. 开关量约定（v3 kelong_pcs.c:13-14, gsstes_pcs.c:13-14, huazhi_pcs.c:45） */
/* ------------------------------------------------------------------------- */

const onoff_convention onoff_01 = {
    .write_tag = "StartStopCtr",
    .read_tag = "StartStopCtr",
    .on_value = 0x0001,
    .off_value = 0x0000,
};

const onoff_convention onoff_00ff = {
    .write_tag = "StartStopCtr",
    .read_tag = "StartStopCtr",
    .on_value = 0xff00,
    .off_value = 0x00ff,
};

/* v3 huazhi 写 OnOff，读 StartStopCtr（0 停 / 2..6 开）；通用策略按写寄存器
 * 做往返，厂商适配 RTC 覆盖读侧映射。 */
const onoff_convention onoff_5555 = {
    .write_tag = "OnOff",
    .read_tag = "OnOff",
    .on_value = 43690,
    .off_value = 21845,
};

int onoff_set(const iems_vendor_device *dev, const onoff_convention *convention, int onoff) {
    if (onoff == IEMS_VENDOR_ON) {
        return write_int(dev, convention->write_tag, convention->on_value);
    }
    if (onoff == IEMS_VENDOR_OFF) {
        return write_int(dev, convention->write_tag, convention->off_value);
    }
    return -1;
}

int onoff_get(const iems_vendor_device *dev, const onoff_convention *convention, int *onoff) {
    int value = 0;
    if (read_int(dev, convention->read_tag, &value) != 0) {
        return -1;
    }
    if (value == convention->on_value) {
        *onoff = IEMS_VENDOR_ON;
        return 0;
    }
    if (value == convention->off_value) {
        *onoff = IEMS_VENDOR_OFF;
        return 0;
    }
    return -1;
}

#define DEFINE_ONOFF_STRATEGY(name, convention)                        \
    static int name##_set_onoff(iems_vendor_device *dev, int onoff) {  \
        return onoff_set(dev, &convention, onoff);                     \
    }                                                                  \
    static int name##_get_onoff(iems_vendor_device *dev, int *onoff) { \
        return onoff_get(dev, &convention, onoff);                     \
    }                                                                  \
    static const char *const name##_compatible[] = {#name, NULL};      \
    DECLARE_EXTENDED_METHODS(name);                                    \
    static const iems_vendor_semantics name##_ops = {                  \
        .compatible = name##_compatible,                               \
        .init = vendor_init,                                           \
        .set_power = unsupported_set_power,                            \
        .set_repower = unsupported_set_repower,                        \
        .set_mode = unsupported_set_int,                               \
        .set_onoff = name##_set_onoff,                                 \
        .set_connect = unsupported_set_int,                            \
        .get_power = unsupported_get_double,                           \
        .get_repower = unsupported_get_double,                         \
        .get_mode = unsupported_get_int,                               \
        .get_onoff = name##_get_onoff,                                 \
        .get_connect = unsupported_get_int,                            \
        .set_value = name##_set_value,                                 \
        .get_int_value = name##_get_int_value,                         \
        .get_double_value = name##_get_double_value,                   \
    };                                                                 \
    DEFINE_EXTENDED_METHODS(name)

DEFINE_ONOFF_STRATEGY(pcs_onoff_01, onoff_01);
DEFINE_ONOFF_STRATEGY(pcs_onoff_00ff, onoff_00ff);
DEFINE_ONOFF_STRATEGY(pcs_onoff_5555, onoff_5555);

/* ------------------------------------------------------------------------- */
/* 5. 整型到 double 的显式转换（v3 xiankong_pcs.c:12-16, kelong_pcs.c:64-74） */
/*                                                                           */
/*    v3 在 double 形参/返回值的 vtable 上用 int 访问器，隐式转换会悄悄丢      */
/*    小数；这里把截断集中成显式转换并测试钉住。                              */
/* ------------------------------------------------------------------------- */

static const char *const int_compatible[] = {"pcs_int_double", NULL};

static int int_set_power(iems_vendor_device *dev, double power) {
    return write_int(dev, "ActPowSet", (int)power);
}

static int int_set_repower(iems_vendor_device *dev, double repower) {
    return write_int(dev, "ReactPowSet", (int)repower);
}

static int int_get_power(iems_vendor_device *dev, double *power) {
    return read_int_as_double(dev, "ActivePower", power);
}

static int int_get_repower(iems_vendor_device *dev, double *repower) {
    return read_int_as_double(dev, "ReactivePower", repower);
}

DECLARE_EXTENDED_METHODS(pcs_int_double);
static const iems_vendor_semantics pcs_int_double_ops = {
    .compatible = int_compatible,
    .init = vendor_init,
    .set_power = int_set_power,
    .set_repower = int_set_repower,
    .set_mode = unsupported_set_int,
    .set_onoff = unsupported_set_int,
    .set_connect = unsupported_set_int,
    .get_power = int_get_power,
    .get_repower = int_get_repower,
    .get_mode = unsupported_get_int,
    .get_onoff = unsupported_get_int,
    .get_connect = unsupported_get_int,
    .set_value = pcs_int_double_set_value,
    .get_int_value = pcs_int_double_get_int_value,
    .get_double_value = pcs_int_double_get_double_value,
};
DEFINE_EXTENDED_METHODS(pcs_int_double);

/* ------------------------------------------------------------------------- */
/* 6. 只读/转发（v3 mppt/yuntianshuneng.c:15-43）                             */
/*                                                                           */
/*    v3 set_power 为 NULL（pe_adapter_init 会回落 common_set_power）；按任务  */
/*    约定，没实现的方法直接返回 -1。get_power 累加四路 InputPower。          */
/* ------------------------------------------------------------------------- */

static const char *const sum_compatible[] = {"mppt_sum4", NULL};
static const char *const sum_tags[] = {"InputPower1", "InputPower2", "InputPower3", "InputPower4"};

int sum_get_power(iems_vendor_device *dev, double *power) {
    double total = 0.0;
    for (size_t i = 0; i < ARRAY_LEN(sum_tags); i++) {
        double value = 0.0;
        if (read_tag(dev, sum_tags[i], &value) != 0) {
            return -1;
        }
        total += value;
    }
    *power = total;
    return 0;
}

DECLARE_EXTENDED_METHODS(mppt_sum4);
static const iems_vendor_semantics mppt_sum4_ops = {
    .compatible = sum_compatible,
    .init = vendor_init,
    .set_power = unsupported_set_power,
    .set_repower = unsupported_set_repower,
    .set_mode = unsupported_set_int,
    .set_onoff = unsupported_set_int,
    .set_connect = unsupported_set_int,
    .get_power = sum_get_power,
    .get_repower = unsupported_get_double,
    .get_mode = unsupported_get_int,
    .get_onoff = unsupported_get_int,
    .get_connect = unsupported_get_int,
    .set_value = mppt_sum4_set_value,
    .get_int_value = mppt_sum4_get_int_value,
    .get_double_value = mppt_sum4_get_double_value,
};
DEFINE_EXTENDED_METHODS(mppt_sum4);

/* ------------------------------------------------------------------------- */
/* 注册表                                                                     */
/* ------------------------------------------------------------------------- */

static const iems_vendor_semantics *const vendor_registry[] = {
    &pcs_standard_ops,
    &pcs_alias_index_ops,
    &pcs_split3_ops,
    &pcs_onoff_01_ops,
    &pcs_onoff_00ff_ops,
    &pcs_onoff_5555_ops,
    &pcs_int_double_ops,
    &mppt_sum4_ops,
    &coepo_ops,
    &inpower_ops,
    &gsstes_ops,
    &kelong_ops,
    &xinhuasan_ops,
    &sinexcel_ops,
    &unsigned_pow_ops,
    &xiankong_ops,
    &xinai_ops,
    &yingbo_ops,
    &zhongteng_ops,
    &absolute_pv_ops,
    &absolute_mppt_ops,
    &percentage_abs_ops,
    &percentage_relative_ops,
    &mppt_ytsn_ops,
    &bms_ops,
    &grid_ops,
    &livotek_hybrid_ops,
    &enjoy_ops,
    &feilton_ops,
    &huazhi_ops,
    &inovance_ops,
    &nengfeng_ops,
};

const iems_vendor_semantics *iems_vendor_registry_lookup(const char *compatible,
                                                         int *compatible_index) {
    if (compatible_index != NULL) {
        *compatible_index = -1;
    }
    if (compatible == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < ARRAY_LEN(vendor_registry); i++) {
        const iems_vendor_semantics *ops = vendor_registry[i];
        for (int index = 0; ops->compatible[index] != NULL; index++) {
            if (strcmp(ops->compatible[index], compatible) == 0) {
                if (compatible_index != NULL) {
                    *compatible_index = index;
                }
                return ops;
            }
        }
    }
    return NULL;
}
