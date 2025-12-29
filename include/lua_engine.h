#ifndef EDGE_LUA_ENGINE_H
#define EDGE_LUA_ENGINE_H

#include "stream_engine.h"

/**
 * @brief 初始化一个 Pipeline 专用的 Lua 状态机
 * 对齐规范 9.2：每 pipeline 一个 Lua state
 */
void *lua_engine_init_pipe_state(const char *script_path);

/**
 * @brief 执行 Lua 算法步骤
 */
stage_ret_t stage_lua_proc(pipeline_stage_t *stage, pipe_event_t *ev);

#endif
