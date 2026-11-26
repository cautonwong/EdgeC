set(ALPHA_TARGET alpha)
add_library(${ALPHA_TARGET} STATIC src/alpha.c)
target_link_libraries(${ALPHA_TARGET} PUBLIC foundation_base)
add_library(public STATIC src/alpha.c)
