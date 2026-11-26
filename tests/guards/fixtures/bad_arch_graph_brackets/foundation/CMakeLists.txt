set(NOISE [[target_link_libraries(foundation_base PRIVATE alpha)]])
set(OTHER [[(]])
#[[ target_link_libraries(foundation_base PRIVATE alpha) ]]
add_library(foundation_base STATIC src/base.c)
target_link_libraries(foundation_base PRIVATE alpha)
