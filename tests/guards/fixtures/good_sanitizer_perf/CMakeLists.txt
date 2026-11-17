cmake_minimum_required(VERSION 3.16)
project(sanitizer_perf_fixture C)

if(ENABLE_ASAN OR ENABLE_TSAN OR ENABLE_UBSAN)
    target_compile_definitions(perf_probe PRIVATE EDGE_SANITIZER_BUILD=1)
endif()
