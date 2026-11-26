add_executable(app main.c)
target_link_libraries(app PRIVATE $<TARGET_NAME_IF_EXISTS:helper>)
