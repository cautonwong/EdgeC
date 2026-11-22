#ifndef DCU_698_APPCONN_H
#define DCU_698_APPCONN_H

#include <stddef.h>
#include <stdint.h>

/* Class 20 对象列表（属性 2）的 A-XDR 字节，供 698 驱动的会话 ACL 使用。
 * 返回 0 表示写入成功；容量不足返回 -1，绝不截断。 */
int dcu_698_appconn_object_list(uint8_t *buf, size_t cap, size_t *len);

/* 端口回调形状：只应答 Class 20 属性 2，其余 OAD 返回 -1。 */
int dcu_698_appconn_resolve(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len);

#endif /* DCU_698_APPCONN_H */
