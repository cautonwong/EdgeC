# P6-04b linkage: run the real converter on the sanitized fixture source,
# then load the produced files with the linked test. No iems_v3 snapshot.
file(REMOVE_RECURSE "${OUT}")
file(MAKE_DIRECTORY "${OUT}")

foreach(family device port)
    execute_process(
        COMMAND "${PYTHON}" "${CONVERTER}" --family ${family} --root "${SOURCE}" --out "${OUT}"
        RESULT_VARIABLE convert_rc
        OUTPUT_VARIABLE convert_out
        ERROR_VARIABLE convert_err)
    if(NOT convert_rc EQUAL 0)
        message(FATAL_ERROR "convert ${family} failed (${convert_rc})\n${convert_out}\n${convert_err}")
    endif()
endforeach()

execute_process(
    COMMAND "${TEST_BIN}" "${OUT}"
    RESULT_VARIABLE load_rc
    OUTPUT_VARIABLE load_out
    ERROR_VARIABLE load_err)
if(NOT load_rc EQUAL 0)
    message(FATAL_ERROR "linked config load failed (${load_rc})\n${load_out}\n${load_err}")
endif()
message(STATUS "linked config load OK")
