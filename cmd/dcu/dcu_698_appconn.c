/**
 * @file dcu_698_appconn.c
 * @brief Class 20 应用连接对象：对象列表（属性 2）的唯一来源（ADR-0015）
 *
 * 表182 的结构按 A-XDR Data 编码：array { structure { OI, structure { 属性[], 方法[] } } }。
 * 权限值是标准枚举：0 不可访问、1 只读、2 只写、3 可读写。
 */
#include "dcu_698_appconn.h"

#include <stddef.h>
#include <stdint.h>

#include "edge_slave_plane.h"

#include "dcu_698_param.h"

typedef struct {
    uint8_t id;
    uint8_t rights;
} acl_attr_t;

typedef struct {
    uint16_t oi;
    const acl_attr_t *attrs;
    uint8_t n_attrs;
    const uint8_t *methods;
    uint8_t n_methods;
} acl_object_t;

static const acl_attr_t k_attrs_0010[] = {{2, 1}, {3, 1}, {4, 1}, {5, 1}};
static const acl_attr_t k_attrs_0020[] = {{2, 1}, {3, 1}, {4, 1}, {5, 1}};
static const acl_attr_t k_attrs_2000[] = {{2, 1}, {3, 1}};
static const acl_attr_t k_attrs_2001[] = {{2, 1}, {3, 1}};
static const acl_attr_t k_attrs_2004[] = {{2, 1}, {3, 1}};
static const acl_attr_t k_attrs_200A[] = {{2, 1}, {3, 1}};
static const acl_attr_t k_attrs_200F[] = {{2, 1}, {3, 1}};
static const acl_attr_t k_attrs_freeze[] = {{2, 1}, {3, 1}};
static const acl_attr_t k_attrs_6000[] = {{2, 1}};
static const acl_attr_t k_attrs_6001[] = {{2, 1}};
static const acl_attr_t k_attrs_6012[] = {{2, 1}};
static const acl_attr_t k_attrs_6013[] = {{2, 1}};
static const acl_attr_t k_attrs_6014[] = {{2, 1}};
static const acl_attr_t k_attrs_6015[] = {{2, 1}};
static const acl_attr_t k_attrs_6002[] = {{2, 1}, {3, 1}, {6, 1}, {10, 1}};
static const acl_attr_t k_attrs_f200[] = {{2, 1}, {3, 1}, {4, 3}};
static const acl_attr_t k_attrs_f2[] = {{2, 1}};
static const acl_attr_t k_attrs_f203[] = {{2, 1}, {4, 1}};
static const acl_attr_t k_attrs_f209[] = {{2, 1}, {5, 1}, {6, 1}};
static const acl_attr_t k_attrs_f20c[] = {{2, 1}, {4, 1}, {5, 1}};
static const acl_attr_t k_attrs_4000[] = {{2, 3}, {3, 3}, {4, 3}};
static const acl_attr_t k_attrs_param[] = {{2, 3}};
/* A6 是只读记录表：写路径只接受 A11/A12，ACL 不能把 A6 广告成可写（能力与广告必须一致）。
 * A8/A9 的 mask 关闭由门面回 DAR 5；ACL 保留可写让该分类可达。 */
static const acl_attr_t k_attrs_event[] = {{2, 1}, {3, 1},  {4, 1},  {5, 1},  {6, 1}, {8, 3},
                                           {9, 3}, {10, 1}, {11, 3}, {12, 3}, {14, 1}};
static const acl_attr_t k_attrs_demand[] = {{1, 1}, {2, 1}, {3, 1}};
static const acl_attr_t k_attrs_harmonic[] = {{1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}};
static const acl_attr_t k_attrs_stat[] = {{1, 1}, {2, 1}, {3, 1}};
static const acl_attr_t k_attrs_display[] = {{2, 1}, {3, 1}, {4, 1}};
static const acl_attr_t k_attrs_7010[] = {{2, 1}};
/* 表157 定义 A1..A19；ACL schema 只到 16，16 归属有冲突记 HOLD，故广告 1..15。 */
static const acl_attr_t k_attrs_2401[] = {{1, 1},  {2, 1},  {3, 1},  {4, 1},  {5, 1},
                                          {6, 1},  {7, 1},  {8, 1},  {9, 1},  {10, 1},
                                          {11, 1}, {12, 1}, {13, 1}, {14, 1}, {15, 1}};
/* 表196/199 的类定义属性只读广告；方法 1（复位）广告只读，ACTION 才能越过 ACL
 * 到对象表如实回 DAR 5。不广告方法 2：标准只定义方法 1。 */
static const acl_attr_t k_attrs_wireless[] = {{1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1},
                                              {7, 1}, {8, 1}, {9, 1}, {10, 1}, {11, 1}};
static const acl_attr_t k_attrs_ethernet[] = {{1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}};
/* 表178 的 A1..A10 只读广告；方法 1..6 广告只读，ACTION 才能越过 ACL 到对象表
 * 如实回 DAR 5。4307..4309 的 127..129 计入 resolver 定义但 ACL 每对象方法上限 8
 * 已被类方法占用，不广告（主站回 DAR 3）。 */
static const acl_attr_t k_attrs_devmgr[] = {{1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1},
                                            {6, 1}, {7, 1}, {8, 1}, {9, 1}, {10, 1}};
static const uint8_t k_methods_devmgr[] = {1, 2, 3, 4, 5, 6};
/* 表184 的 A1..A15 只读广告；方法 1..12 定义但 ACL 每对象方法上限 8，故广告 1..8，
 * 9..12 记 HOLD（主站 ACTION 回 DAR 3）。产品无 ESAM 处理器，广告的方法只回 DAR 5。 */
static const acl_attr_t k_attrs_esam[] = {{1, 1},  {2, 1},  {3, 1},  {4, 1},  {5, 1},
                                          {6, 1},  {7, 1},  {8, 1},  {9, 1},  {10, 1},
                                          {11, 1}, {12, 1}, {13, 1}, {14, 1}, {15, 1}};
static const uint8_t k_methods_esam[] = {1, 2, 3, 4, 5, 6, 7, 8};
/* 表190 的 A1..A18、方法 1..5；ACL 属性 schema 只到 16，故广告 A1..A16，
 * A17/A18 记 HOLD（主站 GET 回 DAR 3）。方法 1..5 全部广告只读。 */
static const acl_attr_t k_attrs_group[] = {
    {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1}, {8, 1},
    {9, 1}, {10, 1}, {11, 1}, {12, 1}, {13, 1}, {14, 1}, {15, 1}, {16, 1}};
static const uint8_t k_methods_group[] = {1, 2, 3, 4, 5};
/* 表175/表A.11：F000 广告 A1..A5，F001/F002 广告 A1..A4；方法 1..8 全部广告。
 * F001 的 OI 方法 9/10 已计入 resolver 定义，但 ACL 方法槽 8 已满，记 HOLD。 */
static const acl_attr_t k_attrs_file_frame[] = {{1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}};
static const acl_attr_t k_attrs_file[] = {{1, 1}, {2, 1}, {3, 1}, {4, 1}};
static const uint8_t k_methods_file[] = {1, 2, 3, 4, 5, 6, 7, 8};
static const uint8_t k_methods_comm_reset[] = {1};
static const uint8_t k_methods_reset_exec[] = {1, 2};
static const uint8_t k_methods_stat[] = {1, 2, 3, 4};
/* 方法 1..4 广告只读，ACTION 才能越过 ACL 到对象表如实回 DAR 5；撤掉广告会静默变成 DAR 3。 */
static const uint8_t k_methods_pulse[] = {1, 2, 3, 4};
static const uint8_t k_methods_display[] = {1, 2, 3, 4, 5, 6};
static const uint8_t k_methods_freeze[] = {3, 4, 5, 7, 8};
static const acl_attr_t k_attrs_8000[] = {{2, 1}, {4, 1}, {5, 1}};
static const uint8_t k_methods_8000[] = {127, 128, 129, 130, 131};
static const uint8_t k_methods_6000[] = {127, 128, 129, 130, 131, 132, 133, 134};
static const uint8_t k_methods_6002[] = {127, 128, 129};
static const uint8_t k_methods_6012[] = {1, 127, 128, 129, 130};
static const uint8_t k_methods_6014[] = {127, 128, 129, 130};
static const uint8_t k_methods_6016[] = {127, 128, 129, 130};
static const uint8_t k_methods_6018[] = {127, 128, 129, 130, 131};
static const uint8_t k_methods_601C[] = {127, 128, 129};
static const uint8_t k_methods_601E[] = {127, 128, 129};
static const uint8_t k_methods_7000[] = {127, 128, 129};
static const uint8_t k_methods_7010[] = {127, 128, 129, 130};

static const acl_object_t k_objects[] = {
    {0x0010, k_attrs_0010, 4, NULL, 0},           {0x0020, k_attrs_0020, 4, NULL, 0},
    {0x2000, k_attrs_2000, 2, NULL, 0},           {0x2001, k_attrs_2001, 2, NULL, 0},
    {0x2004, k_attrs_2004, 2, NULL, 0},           {0x200A, k_attrs_200A, 2, NULL, 0},
    {0x200F, k_attrs_200F, 2, NULL, 0},           {0x3000, k_attrs_event, 11, NULL, 0},
    {0x3005, k_attrs_event, 11, NULL, 0},         {0x5000, k_attrs_freeze, 2, k_methods_freeze, 5},           {0x5001, k_attrs_freeze, 2, k_methods_freeze, 5},
    {0x5002, k_attrs_freeze, 2, k_methods_freeze, 5},           {0x5003, k_attrs_freeze, 2, k_methods_freeze, 5},
    {0x5004, k_attrs_freeze, 2, k_methods_freeze, 5},           {0x5005, k_attrs_freeze, 2, k_methods_freeze, 5},
    {0x5006, k_attrs_freeze, 2, k_methods_freeze, 5},           {0x5007, k_attrs_freeze, 2, k_methods_freeze, 5},
    {0x6000, k_attrs_6000, 1, k_methods_6000, 8}, {0x6001, k_attrs_6001, 1, NULL, 0},
    {0x6002, k_attrs_6002, 4, k_methods_6002, 3},
    {0x6012, k_attrs_6012, 1, k_methods_6012, 5}, {0x6013, k_attrs_6013, 1, NULL, 0},
    {0x6014, k_attrs_6014, 1, k_methods_6014, 4}, {0x6015, k_attrs_6015, 1, NULL, 0},
    {0x6016, k_attrs_6014, 1, k_methods_6016, 4}, {0x6017, k_attrs_6014, 1, NULL, 0},
    {0x6018, k_attrs_6014, 1, k_methods_6018, 5}, {0x6019, k_attrs_6014, 1, NULL, 0},
    {0x601A, k_attrs_6014, 1, NULL, 0},           {0x601B, k_attrs_6014, 1, NULL, 0},
    {0x601C, k_attrs_6014, 1, k_methods_601C, 3}, {0x601D, k_attrs_6014, 1, NULL, 0},
    {0x601E, k_attrs_6014, 1, k_methods_601E, 3}, {0x601F, k_attrs_6014, 1, NULL, 0},
    {0x6032, k_attrs_6014, 1, NULL, 0},           {0x6033, k_attrs_6014, 1, NULL, 0},
    {0x6034, k_attrs_6014, 1, NULL, 0},           {0x6035, k_attrs_6014, 1, NULL, 0},
    {0x6040, k_attrs_6014, 1, NULL, 0},           {0x6041, k_attrs_6014, 1, NULL, 0},
    {0x6042, k_attrs_6014, 1, NULL, 0},
    {0x7000, NULL, 0, k_methods_7000, 3},
    {0x7010, k_attrs_7010, 1, k_methods_7010, 4},
    {0x1010, k_attrs_demand, 3, k_methods_reset_exec, 2},
    {0x1011, k_attrs_demand, 3, k_methods_reset_exec, 2},
    {0x1012, k_attrs_demand, 3, k_methods_reset_exec, 2},
    {0x1013, k_attrs_demand, 3, k_methods_reset_exec, 2},
    {0x200D, k_attrs_harmonic, 6, k_methods_reset_exec, 2},
    {0x200E, k_attrs_harmonic, 6, k_methods_reset_exec, 2},
    {0x2100, k_attrs_stat, 3, k_methods_stat, 4}, {0x2101, k_attrs_stat, 3, k_methods_stat, 4},
    {0x2102, k_attrs_stat, 3, k_methods_stat, 4}, {0x2103, k_attrs_stat, 3, k_methods_stat, 4},
    {0x2104, k_attrs_stat, 3, k_methods_stat, 4}, {0x2110, k_attrs_stat, 3, k_methods_stat, 4},
    {0x2111, k_attrs_stat, 3, k_methods_stat, 4}, {0x2112, k_attrs_stat, 3, k_methods_stat, 4},
    {0x2113, k_attrs_stat, 3, k_methods_stat, 4}, {0x2114, k_attrs_stat, 3, k_methods_stat, 4},
    {0x2120, k_attrs_stat, 3, k_methods_stat, 4}, {0x2121, k_attrs_stat, 3, k_methods_stat, 4},
    {0x2122, k_attrs_stat, 3, k_methods_stat, 4}, {0x2123, k_attrs_stat, 3, k_methods_stat, 4},
    {0x2124, k_attrs_stat, 3, k_methods_stat, 4},
    {0x2401, k_attrs_2401, 15, k_methods_pulse, 4},
    {0x4300, k_attrs_devmgr, 10, k_methods_devmgr, 6},
    {0x4307, k_attrs_devmgr, 10, k_methods_devmgr, 6},
    {0x4308, k_attrs_devmgr, 10, k_methods_devmgr, 6},
    {0x4309, k_attrs_devmgr, 10, k_methods_devmgr, 6},
    {0xF100, k_attrs_esam, 15, k_methods_esam, 8},
    {0x2301, k_attrs_group, 16, k_methods_group, 5},
    {0x2302, k_attrs_group, 16, k_methods_group, 5},
    {0x2303, k_attrs_group, 16, k_methods_group, 5},
    {0x2304, k_attrs_group, 16, k_methods_group, 5},
    {0x2305, k_attrs_group, 16, k_methods_group, 5},
    {0x2306, k_attrs_group, 16, k_methods_group, 5},
    {0x2307, k_attrs_group, 16, k_methods_group, 5},
    {0x2308, k_attrs_group, 16, k_methods_group, 5},
    {0xF000, k_attrs_file_frame, 5, k_methods_file, 8},
    {0xF001, k_attrs_file, 4, k_methods_file, 8},
    {0xF002, k_attrs_file, 4, k_methods_file, 8},
    {0x4500, k_attrs_wireless, 11, k_methods_comm_reset, 1},
    {0x4501, k_attrs_wireless, 11, k_methods_comm_reset, 1},
    {0x4510, k_attrs_ethernet, 5, k_methods_comm_reset, 1},
    {0x4511, k_attrs_ethernet, 5, k_methods_comm_reset, 1},
    {0x4512, k_attrs_ethernet, 5, k_methods_comm_reset, 1},
    {0x4513, k_attrs_ethernet, 5, k_methods_comm_reset, 1},
    {0x4514, k_attrs_ethernet, 5, k_methods_comm_reset, 1},
    {0x4515, k_attrs_ethernet, 5, k_methods_comm_reset, 1},
    {0x4516, k_attrs_ethernet, 5, k_methods_comm_reset, 1},
    {0x4517, k_attrs_ethernet, 5, k_methods_comm_reset, 1},
    {0xF300, k_attrs_display, 3, k_methods_display, 6},
    {0xF301, k_attrs_display, 3, k_methods_display, 6},
    {0x8000, k_attrs_8000, 3, k_methods_8000, 5},         {0xF200, k_attrs_f200, 3, NULL, 0},
    {0xF201, k_attrs_f2, 1, NULL, 0},             {0xF202, k_attrs_f2, 1, NULL, 0},
    {0xF203, k_attrs_f203, 2, NULL, 0},           {0xF204, k_attrs_f203, 2, NULL, 0},
    {0xF205, k_attrs_f2, 1, NULL, 0},             {0xF206, k_attrs_f203, 2, NULL, 0},
    {0xF207, k_attrs_f2, 1, NULL, 0},             {0xF208, k_attrs_f2, 1, NULL, 0},
    {0xF209, k_attrs_f209, 3, NULL, 0},           {0xF20A, k_attrs_f2, 1, NULL, 0},
    {0xF20B, k_attrs_f2, 1, NULL, 0},             {0xF20C, k_attrs_f20c, 3, NULL, 0},
};

#define COUNT_PARAM_OI(oi) +1
#define PARAM_OI_COUNT (0 DCU_698_PARAM_OIS(COUNT_PARAM_OI))

#define OBJECT_COUNT (sizeof(k_objects) / sizeof(k_objects[0]) + PARAM_OI_COUNT)

_Static_assert(OBJECT_COUNT <= EDGE_SLAVE_ACL_MAX_OBJECTS,
               "object list exceeds the 698 driver ACL cap");

static int _put(uint8_t *buf, size_t cap, size_t *off, const uint8_t *src, size_t n) {
    if (*off + n > cap) return -1;
    for (size_t i = 0; i < n; i++) buf[(*off)++] = src[i];
    return 0;
}

static int _put_u8(uint8_t *buf, size_t cap, size_t *off, uint8_t v) {
    return _put(buf, cap, off, &v, 1);
}

static int _put_object(uint8_t *buf, size_t cap, size_t *off, uint16_t oi, const acl_attr_t *attrs,
                       uint8_t n_attrs, const uint8_t *methods, uint8_t n_methods) {
    uint8_t head[5] = {0x02, 0x02, 0x50, (uint8_t)(oi >> 8), (uint8_t)oi};
    if (_put(buf, cap, off, head, sizeof(head)) != 0) return -1;
    if (_put_u8(buf, cap, off, 0x02) != 0 || _put_u8(buf, cap, off, 0x02) != 0 ||
        _put_u8(buf, cap, off, 0x01) != 0 || _put_u8(buf, cap, off, n_attrs) != 0) {
        return -1;
    }
    for (uint8_t a = 0; a < n_attrs; a++) {
        uint8_t attr[5] = {0x02, 0x02, 0x11, attrs[a].id, 0x16};
        if (_put(buf, cap, off, attr, sizeof(attr)) != 0 ||
            _put_u8(buf, cap, off, attrs[a].rights) != 0) {
            return -1;
        }
    }
    if (_put_u8(buf, cap, off, 0x01) != 0 || _put_u8(buf, cap, off, n_methods) != 0) return -1;
    for (uint8_t m = 0; m < n_methods; m++) {
        uint8_t meth[5] = {0x02, 0x02, 0x11, methods[m], 0x03};
        if (_put(buf, cap, off, meth, sizeof(meth)) != 0 || _put_u8(buf, cap, off, 1) != 0) {
            return -1;
        }
    }
    return 0;
}

int dcu_698_appconn_object_list(uint8_t *buf, size_t cap, size_t *len) {
    if (!buf || !len) return -1;
    size_t off = 0;
    if (_put_u8(buf, cap, &off, 0x01) != 0) return -1;
    if (OBJECT_COUNT < 0x80) {
        if (_put_u8(buf, cap, &off, (uint8_t)OBJECT_COUNT) != 0) return -1;
    } else {
        uint8_t len_bytes[sizeof(size_t)];
        uint8_t n = 0;
        size_t v = OBJECT_COUNT;
        while (v > 0) {
            len_bytes[sizeof(size_t) - 1 - n] = (uint8_t)v;
            v >>= 8;
            n++;
        }
        if (_put_u8(buf, cap, &off, (uint8_t)(0x80 | n)) != 0) return -1;
        if (_put(buf, cap, &off, len_bytes + sizeof(size_t) - n, n) != 0) return -1;
    }
    for (size_t i = 0; i < sizeof(k_objects) / sizeof(k_objects[0]); i++) {
        const acl_object_t *obj = &k_objects[i];
        if (_put_object(buf, cap, &off, obj->oi, obj->attrs, obj->n_attrs, obj->methods,
                        obj->n_methods) != 0) {
            return -1;
        }
    }
#define PUT_PARAM_OBJECT(oi)                                                                   \
    do {                                                                                       \
        int rc = (oi) == 0x4000                                                                \
                     ? _put_object(buf, cap, &off, 0x4000, k_attrs_4000, 3, NULL, 0)           \
                     : _put_object(buf, cap, &off, (uint16_t)(oi), k_attrs_param, 1, NULL, 0); \
        if (rc != 0) return -1;                                                                \
    } while (0);
    DCU_698_PARAM_OIS(PUT_PARAM_OBJECT)
#undef PUT_PARAM_OBJECT
    *len = off;
    return 0;
}

int dcu_698_appconn_resolve(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    (void)ctx;
    if (oad != 0x44000200u) return -1;
    return dcu_698_appconn_object_list(buf, cap, len);
}
