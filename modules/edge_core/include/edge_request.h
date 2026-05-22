#ifndef EDGE_REQUEST_H
#define EDGE_REQUEST_H

#include "net_buf.h"

typedef struct edge_req_s edge_req_t;

// [Aetheris] 北向请求专用结构 (指令下发/参数同步)
typedef struct {
    uint32_t tag_id;
    void *payload;
    size_t len;
} edge_nb_req_t;

// 状态码
#define REQ_STATUS_DONE 0
#define REQ_STATUS_CONTINUE 1
#define REQ_STATUS_ERROR -1

typedef struct {
    // 初始化资源 (如加载驱动)
    int (*setup)(edge_req_t *req, void *ctx);

    // 编码：返回 0 表示有数据要发，<0 表示错误
    int (*encode)(edge_req_t *req, net_buf_t *buf);

    // 解码：返回 DONE/CONTINUE/ERROR, 并通过 consumed 返回消耗字节数
    int (*decode)(edge_req_t *req, net_buf_t *buf, size_t *consumed);

    // 清理
    void (*teardown)(edge_req_t *req);

    // 触发用户回调
    void (*on_finish)(edge_req_t *req, int status);
} request_ops_t;

struct edge_req_s {
    const request_ops_t *ops;
    void *priv_data;  // 场景私有数据
    void *user_cb;    // 用户回调
    uint32_t timeout;
};

#endif