# Nexus Lab: Industrial Algorithm Laboratory
# ------------------------------------------------------------------------------

add_executable(nexus_lab main.c)

# 链接 edge_sdk 获得核心能力，链接 GSL 获得数学加速
# 必须链接一个具体的 Contract (如 contract_iems) 以提供元数据索引
target_link_libraries(nexus_lab PRIVATE 
    edge_sdk
    contract_iems
    GSL::gsl
    GSL::gslcblas
    m
)

target_include_directories(nexus_lab PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../algo/include
)

install(TARGETS nexus_lab RUNTIME DESTINATION bin)
