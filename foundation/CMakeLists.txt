# foundation: pure shared code. Standard library only; no IO, no globals, no business rules.
add_library(edge_foundation STATIC
    src/iec_cp56time.c
    src/simd_cdd_codec.c
    src/multibus_bridge.c
)
target_include_directories(edge_foundation PUBLIC include)
