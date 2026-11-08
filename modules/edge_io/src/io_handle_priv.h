/**
 * @file io_handle_priv.h
 * @brief io_handle_t 的内部布局（不对外公开）
 *
 * 产品层只用 io_executor.h 里的公开操作；这个头只给 edge_io 自己的实现与
 * 白盒单测用。
 */

#ifndef IO_HANDLE_PRIV_H
#define IO_HANDLE_PRIV_H

#include "io_executor.h"

struct io_handle_s {
    const io_ops_t *ops;
    void (*cb)(io_handle_t *h, io_event_t evt, void *payload, void *user_data);
    void *user_data;
};

#endif  // IO_HANDLE_PRIV_H
