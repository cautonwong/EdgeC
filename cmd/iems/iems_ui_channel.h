/**
 * @file iems_ui_channel.h
 * @brief IEMS UI 伴随通道（UDS 上把 DataHub 的 ring/shadow fd 交给 UI 进程）
 */

#ifndef IEMS_UI_CHANNEL_H
#define IEMS_UI_CHANNEL_H

#include <uv.h>

#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uv_pipe_t *srv;
    datahub_t *hub;  /**< 借用，不持有生命周期 */
} iems_ui_channel_t;

/**
 * @brief 监听 UI 伴随 socket；失败只记录原因，不影响主流程。
 */
void iems_ui_channel_start(iems_ui_channel_t *ch, uv_loop_t *loop, datahub_t *hub,
                           const char *path);

/**
 * @brief 关闭监听句柄。
 */
void iems_ui_channel_close(iems_ui_channel_t *ch);

#ifdef __cplusplus
}
#endif

#endif  // IEMS_UI_CHANNEL_H
