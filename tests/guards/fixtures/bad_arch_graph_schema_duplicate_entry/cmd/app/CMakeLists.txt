set(NOISE [[target_link_libraries(app PRIVATE alpha)]])
set(OTHER [[(]])
#[[ target_link_libraries(foundation_base PRIVATE alpha) ]]
add_executable(app main.c)
target_link_libraries(app PRIVATE beta port_impl foundation_base)
