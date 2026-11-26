add_library(alpha STATIC src/alpha.c)
target_link_libraries(alpha PUBLIC beta)
