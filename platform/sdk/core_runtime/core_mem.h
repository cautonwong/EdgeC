#ifndef CORE_MEM_H
#define CORE_MEM_H

#include <stddef.h>
#include <stdint.h>

// 类似 Zephyr 的 net_buf，但在用户态简化
typedef struct net_buf {
  struct net_buf *next;  // 链表指针
  uint8_t *data;         // 有效数据起点
  uint16_t len;          // 有效数据长度
  uint16_t size;         // 总容量
  uint8_t ref;           // 引用计数
  uint8_t _storage[];    // 柔性数组，实际内存
} net_buf_t;

// 初始化内存池
int mem_sys_init(void);

// 申请 buffer
net_buf_t *net_buf_alloc(void);

// 增加引用
net_buf_t *net_buf_ref(net_buf_t *buf);

// 释放 (引用减为0时回收)
void net_buf_unref(net_buf_t *buf);

// 向后追加数据 (类似 memcpy)
void *net_buf_add(net_buf_t *buf, const void *data, size_t len);

#endif
