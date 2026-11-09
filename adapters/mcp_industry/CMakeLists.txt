# adapters/mcp_industry/CMakeLists.txt
#
# Product-specific AEP-508 MCP tool pack. Kept outside edge_mcp so the generic
# dispatcher stays product-agnostic; products opt in by linking this adapter.
add_library(edge_mcp_industry STATIC
    src/mcp_tools_power_quality.c
    src/mcp_tools_transformer.c
    src/mcp_tools_tou.c
    src/mcp_tools_interlock.c
    src/mcp_tools_microservices.c
    src/mcp_tools_registration.c
)

target_include_directories(edge_mcp_industry PUBLIC include)
target_link_libraries(edge_mcp_industry PUBLIC edge_mcp edge_log)
