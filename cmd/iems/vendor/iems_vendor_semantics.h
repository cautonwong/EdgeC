/**
 * @file iems_vendor_semantics.h
 * @brief 厂商语义适配框架：消费者定义 tag I/O 端口，策略 vtable 只描述 tag 语义。
 *
 * 对齐 iems_v3 src/adapter/pe_adapter.h 的 pe_adapter 形状。适配器不直接做
 * Modbus/CAN 等 I/O，只通过 read_tag/write_tag 读写模板 tag。
 */

#ifndef IEMS_VENDOR_SEMANTICS_H
#define IEMS_VENDOR_SEMANTICS_H

#define IEMS_VENDOR_OFF 0
#define IEMS_VENDOR_ON 1
#define IEMS_VENDOR_STANDBY 2

#define IEMS_VENDOR_CONNECT_GRID 1
#define IEMS_VENDOR_DISCONNECT_GRID 0

#define IEMS_VENDOR_FN_SET_POWER "set_power"
#define IEMS_VENDOR_FN_SET_REPOWER "set_repower"
#define IEMS_VENDOR_FN_SET_MODE "set_mode"
#define IEMS_VENDOR_FN_SET_ONOFF "set_onoff"
#define IEMS_VENDOR_FN_SET_CONNECT "set_connect"
#define IEMS_VENDOR_FN_GET_POWER "get_power"
#define IEMS_VENDOR_FN_GET_REPOWER "get_repower"
#define IEMS_VENDOR_FN_GET_MODE "get_mode"
#define IEMS_VENDOR_FN_GET_ONOFF "get_onoff"
#define IEMS_VENDOR_FN_GET_CONNECT "get_connect"

typedef struct iems_vendor_io {
    int (*read_tag)(void *ctx, const char *dev_id, const char *tag_name, double *value);
    int (*write_tag)(void *ctx, const char *dev_id, const char *tag_name, double value);
} iems_vendor_io;

typedef struct iems_vendor_device {
    const iems_vendor_io *io;
    void *ctx;
    const char *dev_id;
    int compatible_index;
} iems_vendor_device;

typedef struct iems_vendor_semantics iems_vendor_semantics;

/* 与 v3 pe_adapter 同形；不支持的方法返回 -1。 */
struct iems_vendor_semantics {
    const char *const *compatible;
    int (*init)(iems_vendor_device *dev, void *par);
    int (*set_power)(iems_vendor_device *dev, double power);
    int (*set_repower)(iems_vendor_device *dev, double repower);
    int (*set_mode)(iems_vendor_device *dev, int mode);
    int (*set_onoff)(iems_vendor_device *dev, int onoff);
    int (*set_connect)(iems_vendor_device *dev, int connect);
    int (*get_power)(iems_vendor_device *dev, double *power);
    int (*get_repower)(iems_vendor_device *dev, double *repower);
    int (*get_mode)(iems_vendor_device *dev, int *mode);
    int (*get_onoff)(iems_vendor_device *dev, int *onoff);
    int (*get_connect)(iems_vendor_device *dev, int *connect);
    int (*set_value)(iems_vendor_device *dev, const char *function, double value);
    int (*get_int_value)(iems_vendor_device *dev, const char *function, int *value);
    int (*get_double_value)(iems_vendor_device *dev, const char *function, double *value);
};

/* compatible 命中时返回 vtable 并写入别名下标；未命中返回 NULL 且下标为 -1。 */
const iems_vendor_semantics *iems_vendor_registry_lookup(const char *compatible,
                                                         int *compatible_index);

#endif /* IEMS_VENDOR_SEMANTICS_H */
