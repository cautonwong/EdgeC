add_library(beta STATIC src/beta.c)
target_link_libraries(beta PUBLIC alpha)
