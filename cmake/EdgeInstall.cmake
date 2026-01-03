# EdgeOS Product Installation Module
# ------------------------------------------------------------------------------

# @brief 统一产品的安装行为
# 
# @param PROD_NAME 产品目标名称 (如 iems, dcu)
macro(install_edge_product PROD_NAME)
    string(TOUPPER ${PROD_NAME} PROD_UPPER)
    set(COMP_NAME "${PROD_UPPER}")
    
    message(STATUS "[Install] Registering component: ${COMP_NAME}")

    # 1. 安装二进制文件
    install(TARGETS ${PROD_NAME} 
            DESTINATION bin 
            COMPONENT ${COMP_NAME})
    
    # 2. 物理安装 Web 产物 (从构建目录直接拷贝)
    if(IS_DIRECTORY "${CMAKE_BINARY_DIR}/products/${PROD_NAME}/www")
        install(DIRECTORY "${CMAKE_BINARY_DIR}/products/${PROD_NAME}/www/" 
                DESTINATION www/${PROD_NAME} 
                COMPONENT ${COMP_NAME})
    endif()

    # 3. 契约与配置 (使用 OPTIONAL 容错)
    install(FILES "${CMAKE_BINARY_DIR}/products/${PROD_NAME}/generated/contracts.yaml" 
            DESTINATION etc/${PROD_NAME} 
            COMPONENT ${COMP_NAME}
            OPTIONAL)
    
    if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/config/${PROD_NAME}")
        install(DIRECTORY "${CMAKE_SOURCE_DIR}/config/${PROD_NAME}/" 
                DESTINATION etc/${PROD_NAME} 
                COMPONENT ${COMP_NAME}
                OPTIONAL)
    endif()
endmacro()
