# app/CMakeLists.txt

add_executable(sim_sensor_app
    main.c
    sim_driver.c
)

target_link_libraries(sim_sensor_app
    PRIVATE
        cedgex::sdk
        uv
)

install(TARGETS sim_sensor_app DESTINATION bin)


target_include_directories(sim_sensor_app
  PRIVATE
    ${LIBUV_INCLUDE_DIRS}
)

target_compile_definitions(sim_sensor_app PRIVATE _GNU_SOURCE)


# --- Set Target Properties ---

set_target_properties(sim_sensor_app
  PROPERTIES
    OUTPUT_NAME "sim_sensor_app"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Apply global compiler options (like ASan)
if(ENABLE_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(sim_sensor_app PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
  target_link_options(sim_sensor_app PRIVATE -fsanitize=address)
endif()

target_include_directories(sim_sensor_app
  PRIVATE
    ${LIBUV_INCLUDE_DIRS}
)
