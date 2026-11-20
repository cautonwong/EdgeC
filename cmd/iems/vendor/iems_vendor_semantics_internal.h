/**
 * @file iems_vendor_semantics_internal.h
 * @brief 厂商语义框架与各厂商模块共享的内部声明，不属于公共 API。
 *
 * 厂商 vtable 按类别拆到独立源文件后，框架策略、tag I/O 辅助与扩展派发需要跨
 * 编译单元使用；公共 API 仍只经 iems_vendor_semantics.h 暴露。
 */

#ifndef IEMS_VENDOR_SEMANTICS_INTERNAL_H
#define IEMS_VENDOR_SEMANTICS_INTERNAL_H

#include <stddef.h>
#include <string.h>

#include "iems_vendor_semantics.h"

#define ARRAY_LEN(array) (sizeof(array) / sizeof((array)[0]))

/* 开关量约定：读写寄存器与 on/off 编码。 */
typedef struct {
    const char *write_tag;
    const char *read_tag;
    int on_value;
    int off_value;
} onoff_convention;

/* 标准五函数骨架的厂商 tag 表。 */
typedef struct {
    const char *power_set;
    const char *repower_set;
    const char *power_read;
    const char *repower_read;
    const char *mode;
    const char *connect;
    const onoff_convention *onoff;
} std5_tags;

extern const onoff_convention onoff_01;
extern const onoff_convention onoff_00ff;
extern const onoff_convention onoff_5555;

/* tag I/O 辅助：端口统一 double，整型寄存器由策略显式截断。 */
int read_tag(const iems_vendor_device *dev, const char *tag_name, double *value);
int write_tag(const iems_vendor_device *dev, const char *tag_name, double value);
int read_int(const iems_vendor_device *dev, const char *tag_name, int *value);
int write_int(const iems_vendor_device *dev, const char *tag_name, int value);
int read_int_as_double(const iems_vendor_device *dev, const char *tag_name, double *value);

int vendor_init(iems_vendor_device *dev, void *par);
int unsupported_set_power(iems_vendor_device *dev, double value);
int unsupported_set_repower(iems_vendor_device *dev, double value);
int unsupported_set_int(iems_vendor_device *dev, int value);
int unsupported_get_double(iems_vendor_device *dev, double *value);
int unsupported_get_int(iems_vendor_device *dev, int *value);

int dispatch_set_value(const iems_vendor_semantics *ops, iems_vendor_device *dev,
                       const char *function, double value);
int dispatch_get_int_value(const iems_vendor_semantics *ops, iems_vendor_device *dev,
                           const char *function, int *value);
int dispatch_get_double_value(const iems_vendor_semantics *ops, iems_vendor_device *dev,
                              const char *function, double *value);

/* 扩展接口按函数名派发到各自 vtable；v3 里 get_mode/get_connect 误派到
 * get_onoff 的实现不能照抄，这里调用同名方法。 */
#define DECLARE_EXTENDED_METHODS(name)                                                          \
    static int name##_set_value(iems_vendor_device *dev, const char *function, double value);   \
    static int name##_get_int_value(iems_vendor_device *dev, const char *function, int *value); \
    static int name##_get_double_value(iems_vendor_device *dev, const char *function, double *value)

#define DEFINE_EXTENDED_METHODS(name)                                                            \
    static int name##_set_value(iems_vendor_device *dev, const char *function, double value) {   \
        return dispatch_set_value(&name##_ops, dev, function, value);                            \
    }                                                                                            \
    static int name##_get_int_value(iems_vendor_device *dev, const char *function, int *value) { \
        return dispatch_get_int_value(&name##_ops, dev, function, value);                        \
    }                                                                                            \
    static int name##_get_double_value(iems_vendor_device *dev, const char *function,            \
                                       double *value) {                                          \
        return dispatch_get_double_value(&name##_ops, dev, function, value);                     \
    }

/* 通用策略由框架源文件实现，厂商模块只引用方法。 */
int standard_set_power(iems_vendor_device *dev, double power);
int standard_set_repower(iems_vendor_device *dev, double repower);
int standard_set_mode(iems_vendor_device *dev, int mode);
int standard_set_onoff(iems_vendor_device *dev, int onoff);
int standard_set_connect(iems_vendor_device *dev, int connect);
int standard_get_power(iems_vendor_device *dev, double *power);
int standard_get_repower(iems_vendor_device *dev, double *repower);
int standard_get_mode(iems_vendor_device *dev, int *mode);
int standard_get_onoff(iems_vendor_device *dev, int *onoff);
int standard_get_connect(iems_vendor_device *dev, int *connect);

int alias_set_power(iems_vendor_device *dev, double power);
int alias_set_repower(iems_vendor_device *dev, double repower);
int alias_set_onoff(iems_vendor_device *dev, int onoff);
int alias_set_connect(iems_vendor_device *dev, int connect);
int alias_get_power(iems_vendor_device *dev, double *power);
int alias_get_repower(iems_vendor_device *dev, double *repower);
int alias_get_onoff(iems_vendor_device *dev, int *onoff);

int split_set_power(iems_vendor_device *dev, double power);
int split_set_repower(iems_vendor_device *dev, double repower);
int split_set_mode(iems_vendor_device *dev, int mode);
int split_set_onoff(iems_vendor_device *dev, int onoff);
int split_set_connect(iems_vendor_device *dev, int connect);
int split_get_power(iems_vendor_device *dev, double *power);
int split_get_repower(iems_vendor_device *dev, double *repower);
int split_get_mode(iems_vendor_device *dev, int *mode);
int split_get_onoff(iems_vendor_device *dev, int *onoff);
int split_get_connect(iems_vendor_device *dev, int *connect);

int onoff_set(const iems_vendor_device *dev, const onoff_convention *convention, int onoff);
int onoff_get(const iems_vendor_device *dev, const onoff_convention *convention, int *onoff);

int sum_get_power(iems_vendor_device *dev, double *power);

#define DEFINE_STD5_VENDOR(name, vendor_tags)                                 \
    static int name##_set_power(iems_vendor_device *dev, double power) {      \
        return write_tag(dev, vendor_tags.power_set, power);                  \
    }                                                                         \
    static int name##_set_repower(iems_vendor_device *dev, double repower) {  \
        return write_tag(dev, vendor_tags.repower_set, repower);              \
    }                                                                         \
    static int name##_set_mode(iems_vendor_device *dev, int mode) {           \
        return write_int(dev, vendor_tags.mode, mode);                        \
    }                                                                         \
    static int name##_set_onoff(iems_vendor_device *dev, int onoff) {         \
        return onoff_set(dev, vendor_tags.onoff, onoff);                      \
    }                                                                         \
    static int name##_set_connect(iems_vendor_device *dev, int connect) {     \
        return write_int(dev, vendor_tags.connect, connect);                  \
    }                                                                         \
    static int name##_get_power(iems_vendor_device *dev, double *power) {     \
        return read_int_as_double(dev, vendor_tags.power_read, power);        \
    }                                                                         \
    static int name##_get_repower(iems_vendor_device *dev, double *repower) { \
        return read_int_as_double(dev, vendor_tags.repower_read, repower);    \
    }                                                                         \
    static int name##_get_mode(iems_vendor_device *dev, int *mode) {          \
        return read_int(dev, vendor_tags.mode, mode);                         \
    }                                                                         \
    static int name##_get_onoff(iems_vendor_device *dev, int *onoff) {        \
        return onoff_get(dev, vendor_tags.onoff, onoff);                      \
    }                                                                         \
    static int name##_get_connect(iems_vendor_device *dev, int *connect) {    \
        return read_int(dev, vendor_tags.connect, connect);                   \
    }                                                                         \
    static const char *const name##_compatible[] = {#name, NULL};             \
    DECLARE_EXTENDED_METHODS(name);                                           \
    const iems_vendor_semantics name##_ops = {                                \
        .compatible = name##_compatible,                                      \
        .init = vendor_init,                                                  \
        .set_power = name##_set_power,                                        \
        .set_repower = name##_set_repower,                                    \
        .set_mode = name##_set_mode,                                          \
        .set_onoff = name##_set_onoff,                                        \
        .set_connect = name##_set_connect,                                    \
        .get_power = name##_get_power,                                        \
        .get_repower = name##_get_repower,                                    \
        .get_mode = name##_get_mode,                                          \
        .get_onoff = name##_get_onoff,                                        \
        .get_connect = name##_get_connect,                                    \
        .set_value = name##_set_value,                                        \
        .get_int_value = name##_get_int_value,                                \
        .get_double_value = name##_get_double_value,                          \
    };                                                                        \
    DEFINE_EXTENDED_METHODS(name)

/* 各厂商模块导出的 vtable；注册表在框架源文件内组合。 */
extern const iems_vendor_semantics coepo_ops;
extern const iems_vendor_semantics inpower_ops;
extern const iems_vendor_semantics gsstes_ops;
extern const iems_vendor_semantics kelong_ops;
extern const iems_vendor_semantics xinhuasan_ops;
extern const iems_vendor_semantics sinexcel_ops;
extern const iems_vendor_semantics unsigned_pow_ops;
extern const iems_vendor_semantics xiankong_ops;
extern const iems_vendor_semantics xinai_ops;
extern const iems_vendor_semantics yingbo_ops;
extern const iems_vendor_semantics zhongteng_ops;
extern const iems_vendor_semantics absolute_pv_ops;
extern const iems_vendor_semantics absolute_mppt_ops;
extern const iems_vendor_semantics percentage_abs_ops;
extern const iems_vendor_semantics percentage_relative_ops;
extern const iems_vendor_semantics mppt_ytsn_ops;
extern const iems_vendor_semantics bms_ops;
extern const iems_vendor_semantics grid_ops;
extern const iems_vendor_semantics livotek_hybrid_ops;
extern const iems_vendor_semantics enjoy_ops;
extern const iems_vendor_semantics feilton_ops;
extern const iems_vendor_semantics huazhi_ops;
extern const iems_vendor_semantics inovance_ops;
extern const iems_vendor_semantics nengfeng_ops;

#endif /* IEMS_VENDOR_SEMANTICS_INTERNAL_H */
