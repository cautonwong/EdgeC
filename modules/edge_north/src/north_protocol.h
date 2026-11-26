#ifndef NORTH_PROTOCOL_H
#define NORTH_PROTOCOL_H

#include "northbound_mgr.h"

/* 按 mgr->protocol_name 创建驱动实例（线程入口调用）。 */
int north_protocol_attach(northbound_mgr_t* mgr);

#endif /* NORTH_PROTOCOL_H */
