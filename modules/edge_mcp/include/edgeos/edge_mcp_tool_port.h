#ifndef EDGE_MCP_TOOL_PORT_H
#define EDGE_MCP_TOOL_PORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_MCP_TAG_NAME_MAX 64
#define EDGE_MCP_TAG_UNIT_MAX 16
#define EDGE_MCP_TAG_CATEGORY_MAX 32

/** Tag value types carried by the tool port (mirrors PipeMsg scalar/event/metric). */
typedef enum {
    EDGE_MCP_TAG_TYPE_SCALAR = 1,
    EDGE_MCP_TAG_TYPE_EVENT = 4,
    EDGE_MCP_TAG_TYPE_METRIC = 5
} edge_mcp_tag_type_t;

/** Component-owned boundary value; tools never see DataHub's PipeMsg. */
typedef struct {
    uint32_t tag_id;
    double value;
    uint8_t quality;
    uint16_t type;
    uint64_t timestamp_ms;
} edge_mcp_tag_value_t;

/** Component-owned tag metadata; no infra pointers cross the boundary. */
typedef struct {
    uint32_t tag_id;
    char name[EDGE_MCP_TAG_NAME_MAX];
    char unit[EDGE_MCP_TAG_UNIT_MAX];
    char category[EDGE_MCP_TAG_CATEGORY_MAX];
    double min_value;
    double max_value;
} edge_mcp_tag_meta_t;

/**
 * @brief Capability port through which MCP tools read and write tag values.
 *
 * Defined by the consumer (edge_mcp), implemented by the composition root's
 * adapter (for example the DataHub adapter in edge_bootstrap). Implementations
 * must not be called after the owner destroys the context.
 */
typedef struct {
    void *ctx;
    int (*read_tag)(void *ctx, uint32_t tag_id, edge_mcp_tag_value_t *out);
    int (*write_tag)(void *ctx, const edge_mcp_tag_value_t *value);
    int (*get_tag_meta)(void *ctx, uint32_t tag_id, edge_mcp_tag_meta_t *out);
} edge_mcp_tool_port_t;

static inline int edge_mcp_tool_read_tag(const edge_mcp_tool_port_t *tools, uint32_t tag_id,
                                         edge_mcp_tag_value_t *out) {
    return (tools && tools->read_tag) ? tools->read_tag(tools->ctx, tag_id, out) : -1;
}

static inline int edge_mcp_tool_write_tag(const edge_mcp_tool_port_t *tools,
                                          const edge_mcp_tag_value_t *value) {
    return (tools && tools->write_tag) ? tools->write_tag(tools->ctx, value) : -1;
}

static inline int edge_mcp_tool_get_tag_meta(const edge_mcp_tool_port_t *tools, uint32_t tag_id,
                                             edge_mcp_tag_meta_t *out) {
    return (tools && tools->get_tag_meta) ? tools->get_tag_meta(tools->ctx, tag_id, out) : -1;
}

#ifdef __cplusplus
}
#endif

#endif /* EDGE_MCP_TOOL_PORT_H */
