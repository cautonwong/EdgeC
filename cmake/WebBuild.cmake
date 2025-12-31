# cmake/WebBuild.cmake

function(add_web_product PRODUCT_NAME WEB_SRC_DIR)
    set(WEB_BUILD_DIR "${CMAKE_BINARY_DIR}/products/${PRODUCT_NAME}/www")
    set(WEB_STAMP "${CMAKE_CURRENT_BINARY_DIR}/web_build_${PRODUCT_NAME}.stamp")

    message(STATUS "Configuring Web UI for ${PRODUCT_NAME} from ${WEB_SRC_DIR}")

    # 1. 自动执行 npm install 和 build
    add_custom_command(
        OUTPUT "${WEB_STAMP}"
        COMMAND npm install
        COMMAND npm run build
        COMMAND ${CMAKE_COMMAND} -E make_directory "${WEB_BUILD_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "dist" "${WEB_BUILD_DIR}"
        COMMAND ${CMAKE_COMMAND} -E touch "${WEB_STAMP}"
        WORKING_DIRECTORY "${WEB_SRC_DIR}"
        COMMENT "Building Web UI for ${PRODUCT_NAME}..."
        VERBATIM
    )

    # 2. 创建一个伪目标供产品二进制文件依赖
    add_custom_target(web_ui_${PRODUCT_NAME} ALL DEPENDS "${WEB_STAMP}")

    # 3. 在安装时，将静态资源放入 config/www
    # 注意：此处我们将其安装到产品私有的目录中，防止多产品冲突
    install(DIRECTORY "${WEB_BUILD_DIR}/" 
            DESTINATION "config/${PRODUCT_NAME}/www"
            COMPONENT ${PRODUCT_NAME})
endfunction()
