add_library(foundation_base STATIC src/base.c)
add_library(edge_local STATIC src/base.c)
set(LIBS "edge_forbidden\;edge_local")
target_link_libraries(foundation_base ${LIBS})
