# modules/edge_security_policy/CMakeLists.txt

add_library(edge_security_policy STATIC
    src/edge_acl.c
)

target_include_directories(edge_security_policy PUBLIC include)
target_link_libraries(edge_security_policy PUBLIC edge_common_interface)
