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
static const acl_attr_t k_attrs_5004[] = {{2, 1}};
static const acl_attr_t k_attrs_6000[] = {{2, 1}};
static const acl_attr_t k_attrs_6001[] = {{2, 1}};
static const acl_attr_t k_attrs_6012[] = {{2, 1}};
static const acl_attr_t k_attrs_6013[] = {{2, 1}};
static const acl_attr_t k_attrs_6002[] = {{2, 1}, {3, 1}};
static const acl_attr_t k_attrs_6003[] = {{2, 1}};
static const acl_attr_t k_attrs_f200[] = {{2, 1}, {3, 1}, {4, 3}};
/* 类 8 白名单：4000 另有 OI 专属属性 3/4（参数模块实现），其余只有属性 2。 */
static const acl_attr_t k_attrs_4000[] = {{2, 3}, {3, 3}, {4, 3}};
static const acl_attr_t k_attrs_param[] = {{2, 3}};
static const acl_attr_t k_attrs_event[] = {{2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 3},
                                           {8, 3}, {9, 3}, {10, 1}, {11, 3}, {12, 3}, {14, 1}};
static const uint8_t k_methods_8000[] = {127, 128, 129, 130, 131};
static const uint8_t k_methods_6000[] = {127, 128, 129, 130, 131, 132, 133, 134};
static const uint8_t k_methods_6012[] = {1, 127, 128, 129, 130};

static const acl_object_t k_objects[] = {
    {0x0010, k_attrs_0010, 4, NULL, 0},
    {0x0020, k_attrs_0020, 4, NULL, 0},
    {0x2000, k_attrs_2000, 2, NULL, 0},
    {0x2001, k_attrs_2001, 2, NULL, 0},
    {0x2004, k_attrs_2004, 2, NULL, 0},
    {0x200A, k_attrs_200A, 2, NULL, 0},
    {0x200F, k_attrs_200F, 2, NULL, 0},
    {0x3000, k_attrs_event, 11, NULL, 0},
    {0x3005, k_attrs_event, 11, NULL, 0},
    {0x5004, k_attrs_5004, 1, NULL, 0},
    {0x6000, k_attrs_6000, 1, k_methods_6000, 8},
    {0x6001, k_attrs_6001, 1, NULL, 0},
    {0x6002, k_attrs_6002, 2, NULL, 0},
    {0x6003, k_attrs_6003, 1, NULL, 0},
    {0x6012, k_attrs_6012, 1, k_methods_6012, 5},
    {0x6013, k_attrs_6013, 1, NULL, 0},
    {0x8000, NULL, 0, k_methods_8000, 5},
    {0xF200, k_attrs_f200, 3, NULL, 0},
    {0x4000, k_attrs_4000, 3, NULL, 0},
    {0x4001, k_attrs_param, 1, NULL, 0},
    {0x4002, k_attrs_param, 1, NULL, 0},
    {0x4003, k_attrs_param, 1, NULL, 0},
    {0x4004, k_attrs_param, 1, NULL, 0},
    {0x401C, k_attrs_param, 1, NULL, 0},
    {0x401D, k_attrs_param, 1, NULL, 0},
    {0x401E, k_attrs_param, 1, NULL, 0},
    {0x401F, k_attrs_param, 1, NULL, 0},
    {0x4020, k_attrs_param, 1, NULL, 0},
    {0x4021, k_attrs_param, 1, NULL, 0},
};

#define OBJECT_COUNT (sizeof(k_objects) / sizeof(k_objects[0]))

static int _put(uint8_t *buf, size_t cap, size_t *off, const uint8_t *src, size_t n) {
    if (*off + n > cap) return -1;
    for (size_t i = 0; i < n; i++) buf[(*off)++] = src[i];
    return 0;
}

static int _put_u8(uint8_t *buf, size_t cap, size_t *off, uint8_t v) {
    return _put(buf, cap, off, &v, 1);
}

int dcu_698_appconn_object_list(uint8_t *buf, size_t cap, size_t *len) {
    if (!buf || !len) return -1;
    size_t off = 0;
    if (_put_u8(buf, cap, &off, 0x01) != 0 || _put_u8(buf, cap, &off, OBJECT_COUNT) != 0) {
        return -1;
    }
    for (size_t i = 0; i < OBJECT_COUNT; i++) {
        const acl_object_t *obj = &k_objects[i];
        uint8_t head[5] = {0x02, 0x02, 0x50, (uint8_t)(obj->oi >> 8), (uint8_t)obj->oi};
        if (_put(buf, cap, &off, head, sizeof(head)) != 0) return -1;
        if (_put_u8(buf, cap, &off, 0x02) != 0 || _put_u8(buf, cap, &off, 0x02) != 0 ||
            _put_u8(buf, cap, &off, 0x01) != 0 ||
            _put_u8(buf, cap, &off, obj->n_attrs) != 0) {
            return -1;
        }
        for (uint8_t a = 0; a < obj->n_attrs; a++) {
            uint8_t attr[5] = {0x02, 0x02, 0x11, obj->attrs[a].id, 0x16};
            if (_put(buf, cap, &off, attr, sizeof(attr)) != 0 ||
                _put_u8(buf, cap, &off, obj->attrs[a].rights) != 0) {
                return -1;
            }
        }
        if (_put_u8(buf, cap, &off, 0x01) != 0 ||
            _put_u8(buf, cap, &off, obj->n_methods) != 0) {
            return -1;
        }
        for (uint8_t m = 0; m < obj->n_methods; m++) {
            uint8_t meth[5] = {0x02, 0x02, 0x11, obj->methods[m], 0x03};
            if (_put(buf, cap, &off, meth, sizeof(meth)) != 0 ||
                _put_u8(buf, cap, &off, 1) != 0) {
                return -1;
            }
        }
    }
    *len = off;
    return 0;
}

int dcu_698_appconn_resolve(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    (void)ctx;
    if (oad != 0x44000200u) return -1;
    return dcu_698_appconn_object_list(buf, cap, len);
}
