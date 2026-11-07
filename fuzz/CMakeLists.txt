# fuzz/CMakeLists.txt - EdgeOS LibFuzzer & Standalone Fuzzing Harnesses

if(ENABLE_FUZZING)
    message(STATUS "[FUZZ] Building with LLVM libFuzzer instrumentation...")
    add_executable(fuzz_protocol_decoder fuzz_protocol_decoder.c)
    target_compile_options(fuzz_protocol_decoder PRIVATE -fsanitize=fuzzer,address,undefined)
    target_link_options(fuzz_protocol_decoder PRIVATE -fsanitize=fuzzer,address,undefined)
else()
    # Standalone verification runner without libFuzzer dependency (runs seed corpus)
    add_executable(fuzz_protocol_decoder fuzz_protocol_decoder.c)
    target_compile_definitions(fuzz_protocol_decoder PRIVATE STANDALONE_FUZZER=1)
    target_link_libraries(fuzz_protocol_decoder PRIVATE edge_common_interface)
    add_test(NAME test_fuzz_protocol_seed COMMAND fuzz_protocol_decoder)

    add_executable(fuzz_cp56time fuzz_cp56time.c)
    target_compile_definitions(fuzz_cp56time PRIVATE STANDALONE_FUZZER=1)
    target_link_libraries(fuzz_cp56time PRIVATE edge_foundation edge_common_interface)
    add_test(NAME test_fuzz_cp56time_seed COMMAND fuzz_cp56time)

    add_executable(fuzz_simd_cdd fuzz_simd_cdd.c)
    target_compile_definitions(fuzz_simd_cdd PRIVATE STANDALONE_FUZZER=1)
    target_link_libraries(fuzz_simd_cdd PRIVATE edge_foundation edge_common_interface)
    add_test(NAME test_fuzz_simd_cdd_seed COMMAND fuzz_simd_cdd)
endif()
