add_library(foundation_base STATIC src/base.c)
set(LIBS alpha PARENT_SCOPE)
target_link_libraries(foundation_base ${LIBS})
