set(ALPHA_DEP alpha)
add_library(beta STATIC src/beta.c)
target_link_libraries(beta PUBLIC ${ALPHA_DEP} foundation_base)
