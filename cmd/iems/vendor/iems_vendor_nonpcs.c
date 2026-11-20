#include "iems_vendor_semantics_internal.h"

/* ------------------------------------------------------------------------- */
/* 1. 通用类别适配器（P3-00 批次 C）                                          */
/*                                                                           */
/*    absolute_PV/absolute/bms/grid/percentage_relative 只转发模板 function    */
/*    set_power/get_power（v3 pv/absolute_pv.c:19-33、mppt/absolute.c:19-33、  */
/*    bms/bms.c:22-34、meter/meter_grid.c:22-34、                             */
/*    mppt/percentage_relative.c:43-46）；模板 tag 映射不在适配器源码内，      */
/*    EdgeC 端口用 function 名作为 tag。                                      */
/* ------------------------------------------------------------------------- */

static int forward_set_power(iems_vendor_device *dev, double power) {
    return write_tag(dev, "set_power", power);
}

static int forward_get_power(iems_vendor_device *dev, double *power) {
    return read_tag(dev, "get_power", power);
}

#define DEFINE_FORWARD_VENDOR(name, compatible_string, set_power_method)      \
    static const char *const name##_compatible[] = {compatible_string, NULL}; \
    DECLARE_EXTENDED_METHODS(name);                                           \
    const iems_vendor_semantics name##_ops = {                                \
        .compatible = name##_compatible,                                      \
        .init = vendor_init,                                                  \
        .set_power = set_power_method,                                        \
        .set_repower = unsupported_set_repower,                               \
        .set_mode = unsupported_set_int,                                      \
        .set_onoff = unsupported_set_int,                                     \
        .set_connect = unsupported_set_int,                                   \
        .get_power = forward_get_power,                                       \
        .get_repower = unsupported_get_double,                                \
        .get_mode = unsupported_get_int,                                      \
        .get_onoff = unsupported_get_int,                                     \
        .get_connect = unsupported_get_int,                                   \
        .set_value = name##_set_value,                                        \
        .get_int_value = name##_get_int_value,                                \
        .get_double_value = name##_get_double_value,                          \
    };                                                                        \
    DEFINE_EXTENDED_METHODS(name)

DEFINE_FORWARD_VENDOR(absolute_pv, "absolute_PV", forward_set_power);
DEFINE_FORWARD_VENDOR(absolute_mppt, "absolute", forward_set_power);

/* bms/grid 在 v3 也只读 get_power，读失败填 0；这里按框架约定上报 -1。
 * percentage_relative 的写路径 v3 自注「严禁使用」（mppt/percentage_relative.c:21），
 * 表达式还除以从未赋值的 Current（:39）；只迁移可读的 get_power。 */
DEFINE_FORWARD_VENDOR(bms, "bms", unsupported_set_power);
DEFINE_FORWARD_VENDOR(grid, "grid", unsupported_set_power);
DEFINE_FORWARD_VENDOR(percentage_relative, "percentage_relative", unsupported_set_power);

/* v3 percentage_abs：读当前功率，按 期望/当前*100 写 OutPowerSet
 * （mppt/percentage_abs.c:22-37）。当前功率为 0 时 v3 会除零，这里返回 -1。 */
static int percentage_abs_set_power(iems_vendor_device *dev, double exp_power) {
    double current = 0.0;
    if (read_tag(dev, "get_power", &current) != 0) {
        return -1;
    }
    if (current == 0.0) {
        return -1;
    }
    return write_tag(dev, "OutPowerSet", exp_power / current * 100.0);
}

static const char *const percentage_abs_compatible[] = {"percentage_abs", NULL};

DECLARE_EXTENDED_METHODS(percentage_abs);
const iems_vendor_semantics percentage_abs_ops = {
    .compatible = percentage_abs_compatible,
    .init = vendor_init,
    .set_power = percentage_abs_set_power,
    .set_repower = unsupported_set_repower,
    .set_mode = unsupported_set_int,
    .set_onoff = unsupported_set_int,
    .set_connect = unsupported_set_int,
    .get_power = forward_get_power,
    .get_repower = unsupported_get_double,
    .get_mode = unsupported_get_int,
    .get_onoff = unsupported_get_int,
    .get_connect = unsupported_get_int,
    .set_value = percentage_abs_set_value,
    .get_int_value = percentage_abs_get_int_value,
    .get_double_value = percentage_abs_get_double_value,
};
DEFINE_EXTENDED_METHODS(percentage_abs);

/* v3 mppt_ytsn 只累加四路 InputPower1..4（mppt/yuntianshuneng.c:15-24），
 * 复用 mppt_sum4 策略；set_power 在 v3 为 NULL，按约定返回 -1。 */
static const char *const mppt_ytsn_compatible[] = {"mppt_ytsn", NULL};

DECLARE_EXTENDED_METHODS(mppt_ytsn);
const iems_vendor_semantics mppt_ytsn_ops = {
    .compatible = mppt_ytsn_compatible,
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
    .set_value = mppt_ytsn_set_value,
    .get_int_value = mppt_ytsn_get_int_value,
    .get_double_value = mppt_ytsn_get_double_value,
};
DEFINE_EXTENDED_METHODS(mppt_ytsn);

/* ------------------------------------------------------------------------- */
/* 2. 混合逆变器 livotek_hybrid（v3 hybrid/livotek_hybrid.c:1-655）           */
/*                                                                           */
/*    写命令前强制外部 EMS 模式，并把不参与本次命令的功率环置无效值；          */
/*    总功率命令切 ExportPowerMode=0xA0，分相命令切 0xA1。                    */
/* ------------------------------------------------------------------------- */

#define LIVOTEK_INVALID_FLOAT 21474836.475
#define LIVOTEK_EXPORT_TOTAL 0xA0
#define LIVOTEK_EXPORT_PHASE 0xA1
#define LIVOTEK_RUN_ON 0xCF
#define LIVOTEK_RUN_OFF 0xCE
#define LIVOTEK_GRID_OFF 0x55
#define LIVOTEK_MASTER 0xA0
/* v3 常量名为 PE_ADP_ON_WRITE=206 / PE_ADP_OFF_WRITE=207，但 set_onoff 对
 * OFF 写 206、ON 写 207（hybrid/livotek_hybrid.c:17-18,254-269）；保留实际写值。 */
#define LIVOTEK_OFF_WRITE 206
#define LIVOTEK_ON_WRITE 207

/* v3 用 dev_get 读缺失 tag 时得到 0（collector-api.c:516-528），随后按“不等于
 * 目标值”重写；这里读失败同样落入写入分支。 */
static int livotek_force_int(iems_vendor_device *dev, const char *tag_name, int expected) {
    double raw = 0.0;
    if (read_tag(dev, tag_name, &raw) == 0 && (((int)raw) & 0xFF) == expected) {
        return 0;
    }
    return write_int(dev, tag_name, expected);
}

static int livotek_force_float(iems_vendor_device *dev, const char *tag_name, double expected) {
    double raw = 0.0;
    if (read_tag(dev, tag_name, &raw) == 0 && raw == expected) {
        return 0;
    }
    return write_tag(dev, tag_name, expected);
}

static int livotek_force_ems_mode(iems_vendor_device *dev) {
    return livotek_force_int(dev, "EnergyManageMode", 3);
}

static int livotek_begin_ac_command(iems_vendor_device *dev, int export_mode) {
    if (livotek_force_ems_mode(dev) != 0) {
        return -1;
    }
    if (livotek_force_int(dev, "ExportPowerMode", export_mode) != 0) {
        return -1;
    }
    if (livotek_force_float(dev, "PVPowerLimit", LIVOTEK_INVALID_FLOAT) != 0) {
        return -1;
    }
    return livotek_force_float(dev, "BatPower", LIVOTEK_INVALID_FLOAT);
}

static int livotek_set_power(iems_vendor_device *dev, double power) {
    if (livotek_begin_ac_command(dev, LIVOTEK_EXPORT_TOTAL) != 0) {
        return -1;
    }
    return write_tag(dev, "InverterPower", power);
}

static int livotek_set_phase_power(iems_vendor_device *dev, double power) {
    if (livotek_begin_ac_command(dev, LIVOTEK_EXPORT_PHASE) != 0) {
        return -1;
    }
    double phase = power / 3.0;
    if (write_tag(dev, "L1Power", phase) != 0) {
        return -1;
    }
    if (write_tag(dev, "L2Power", phase) != 0) {
        return -1;
    }
    return write_tag(dev, "L3Power", phase);
}

static int livotek_set_single_phase_power(iems_vendor_device *dev, const char *tag_name,
                                          double power) {
    if (livotek_begin_ac_command(dev, LIVOTEK_EXPORT_PHASE) != 0) {
        return -1;
    }
    return write_tag(dev, tag_name, power);
}

static int livotek_set_a_phase_power(iems_vendor_device *dev, double power) {
    return livotek_set_single_phase_power(dev, "L1Power", power);
}

static int livotek_set_b_phase_power(iems_vendor_device *dev, double power) {
    return livotek_set_single_phase_power(dev, "L2Power", power);
}

static int livotek_set_c_phase_power(iems_vendor_device *dev, double power) {
    return livotek_set_single_phase_power(dev, "L3Power", power);
}

static int livotek_set_pv_power(iems_vendor_device *dev, double power) {
    if (livotek_force_ems_mode(dev) != 0) {
        return -1;
    }
    if (livotek_force_float(dev, "InverterPower", LIVOTEK_INVALID_FLOAT) != 0) {
        return -1;
    }
    if (livotek_force_float(dev, "BatPower", LIVOTEK_INVALID_FLOAT) != 0) {
        return -1;
    }
    return write_tag(dev, "PVPowerLimit", power);
}

static int livotek_set_bat_power(iems_vendor_device *dev, double power) {
    if (livotek_force_ems_mode(dev) != 0) {
        return -1;
    }
    if (livotek_force_float(dev, "InverterPower", LIVOTEK_INVALID_FLOAT) != 0) {
        return -1;
    }
    if (livotek_force_float(dev, "PVPowerLimit", LIVOTEK_INVALID_FLOAT) != 0) {
        return -1;
    }
    return write_tag(dev, "BatPower", power);
}

static int livotek_set_gen_on_off_func(iems_vendor_device *dev, double value) {
    int func_value = (int)value;
    double raw = 0.0;
    if (read_tag(dev, "GenFunction", &raw) == 0 && (int)raw == func_value) {
        return 0;
    }
    return write_int(dev, "GenFunction", func_value);
}

static int livotek_set_onoff(iems_vendor_device *dev, int onoff) {
    if (onoff == IEMS_VENDOR_OFF) {
        return write_int(dev, "StartStopCtr", LIVOTEK_OFF_WRITE);
    }
    if (onoff == IEMS_VENDOR_ON) {
        return write_int(dev, "StartStopCtr", LIVOTEK_ON_WRITE);
    }
    return -1;
}

static int livotek_get_power(iems_vendor_device *dev, double *power) {
    return read_tag(dev, "ActivePower", power);
}

static int livotek_get_repower(iems_vendor_device *dev, double *repower) {
    return read_tag(dev, "ReactivePower", repower);
}

static int livotek_get_onoff(iems_vendor_device *dev, int *onoff) {
    int status = 0;
    if (read_int(dev, "RunStatu", &status) != 0) {
        return -1;
    }
    if (status == LIVOTEK_RUN_ON) {
        *onoff = IEMS_VENDOR_ON;
        return 0;
    }
    if (status == LIVOTEK_RUN_OFF) {
        *onoff = IEMS_VENDOR_OFF;
        return 0;
    }
    return -1;
}

/* GridStatus=0x55 是关闭/离网，其余按并网（v3 hybrid/livotek_hybrid.c:428-439，
 * 与 unsigned_pow.c:69-84 的 0x55=关闭一致）。 */
static int livotek_get_connect(iems_vendor_device *dev, int *connect) {
    int status = 0;
    if (read_int(dev, "GridStatus", &status) != 0) {
        return -1;
    }
    *connect =
        (status == LIVOTEK_GRID_OFF) ? IEMS_VENDOR_DISCONNECT_GRID : IEMS_VENDOR_CONNECT_GRID;
    return 0;
}

static int livotek_get_is_host(iems_vendor_device *dev, int *is_host) {
    int value = 0;
    if (read_int(dev, "MasterSlaveSet", &value) != 0) {
        return -1;
    }
    *is_host = (value == LIVOTEK_MASTER) ? 1 : 0;
    return 0;
}

/* get_double_value 的扩展 function -> tag 表（v3 hybrid/livotek_hybrid.c:539-626）。
 * int_tag 为 1 的 tag 在 v3 用 int 访问器读，再转 double。 */
static const struct {
    const char *function;
    const char *tag_name;
    int int_tag;
} livotek_double_reads[] = {
    {"get_pv_power", "TotalDCPower", 0},
    {"get_bat_power", "BatteryPowera", 1},
    {"get_bat_soc", "BatteryPackSOC", 1},
    {"get_total_input_energy", "TotalACInputEnergy", 0},
    {"get_total_output_energy", "TotalACOutputEnergy", 0},
    {"get_pv_cumulative_energy", "TotalGenEnergy", 0},
    {"get_bat_charge_energy", "TotalChargeEnergy", 0},
    {"get_bat_discharge_energy", "TotalDischargeEnergy", 0},
    {"get_bat_voltage", "BatteryVoltage", 0},
    {"get_bat_current", "BatteryCurrent", 0},
    {"func_get_a_phase_power", "AphaseActPower", 1},
    {"func_get_b_phase_power", "BphaseActPower", 1},
    {"func_get_c_phase_power", "CphaseActPower", 1},
    {"get_inveter_discharge_max_actual", "RatedActivePwr", 0},
    {"get_inveter_charge_max_actual", "RatedActivePwr", 0},
    {"get_grid_r_volt", "GridRVoltage", 0},
    {"get_grid_s_volt", "GridSVoltage", 0},
    {"get_grid_t_volt", "GridTVoltage", 0},
};

static int livotek_read_double(iems_vendor_device *dev, const char *function, double *value) {
    for (size_t i = 0; i < ARRAY_LEN(livotek_double_reads); i++) {
        if (strcmp(livotek_double_reads[i].function, function) == 0) {
            if (livotek_double_reads[i].int_tag) {
                return read_int_as_double(dev, livotek_double_reads[i].tag_name, value);
            }
            return read_tag(dev, livotek_double_reads[i].tag_name, value);
        }
    }
    return -1;
}

static const struct {
    const char *function;
    int (*set)(iems_vendor_device *dev, double value);
} livotek_setters[] = {
    {"set_phase_power", livotek_set_phase_power},
    {"func_set_a_phase_power", livotek_set_a_phase_power},
    {"func_set_b_phase_power", livotek_set_b_phase_power},
    {"func_set_c_phase_power", livotek_set_c_phase_power},
    {"set_pv_power", livotek_set_pv_power},
    {"set_bat_power", livotek_set_bat_power},
    {"set_gen_on_off_func", livotek_set_gen_on_off_func},
};

DECLARE_EXTENDED_METHODS(livotek_hybrid);

static const char *const livotek_hybrid_compatible[] = {"livotek_hybrid", NULL};

/* v3 的 set_repower/set_mode/set_connect 为空实现、get_mode 不写输出
 * （hybrid/livotek_hybrid.c:243-274,404-407）；按 vtable 约定返回 -1。 */
const iems_vendor_semantics livotek_hybrid_ops = {
    .compatible = livotek_hybrid_compatible,
    .init = vendor_init,
    .set_power = livotek_set_power,
    .set_repower = unsupported_set_repower,
    .set_mode = unsupported_set_int,
    .set_onoff = livotek_set_onoff,
    .set_connect = unsupported_set_int,
    .get_power = livotek_get_power,
    .get_repower = livotek_get_repower,
    .get_mode = unsupported_get_int,
    .get_onoff = livotek_get_onoff,
    .get_connect = livotek_get_connect,
    .set_value = livotek_hybrid_set_value,
    .get_int_value = livotek_hybrid_get_int_value,
    .get_double_value = livotek_hybrid_get_double_value,
};

static int livotek_hybrid_set_value(iems_vendor_device *dev, const char *function, double value) {
    if (function == NULL) {
        return -1;
    }
    int ret = dispatch_set_value(&livotek_hybrid_ops, dev, function, value);
    if (ret != -1) {
        return ret;
    }
    for (size_t i = 0; i < ARRAY_LEN(livotek_setters); i++) {
        if (strcmp(livotek_setters[i].function, function) == 0) {
            return livotek_setters[i].set(dev, value);
        }
    }
    return -1;
}

static int livotek_hybrid_get_int_value(iems_vendor_device *dev, const char *function, int *value) {
    if (function == NULL) {
        return -1;
    }
    if (strcmp(function, "get_is_host") == 0) {
        return livotek_get_is_host(dev, value);
    }
    return dispatch_get_int_value(&livotek_hybrid_ops, dev, function, value);
}

static int livotek_hybrid_get_double_value(iems_vendor_device *dev, const char *function,
                                           double *value) {
    if (function == NULL) {
        return -1;
    }
    int ret = dispatch_get_double_value(&livotek_hybrid_ops, dev, function, value);
    if (ret != -1) {
        return ret;
    }
    return livotek_read_double(dev, function, value);
}
