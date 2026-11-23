# Use OUTPUT_FILE instead of shell redirection so paths with spaces are safe.
foreach(_mode lst asm)
    if(_mode STREQUAL "lst")
        set(_options --source --all-headers --demangle -M xw --line-numbers --wide)
    else()
        set(_options -S -d -M xw)
    endif()

    execute_process(
        COMMAND "${OBJDUMP}" ${_options} "${ELF_FILE}"
        OUTPUT_FILE "${OUTPUT_PREFIX}.${_mode}"
        ERROR_VARIABLE _error
        RESULT_VARIABLE _result
    )
    if(NOT "${_result}" STREQUAL "0")
        message(FATAL_ERROR "Failed to generate ${OUTPUT_PREFIX}.${_mode}: ${_result}\n${_error}")
    endif()
endforeach()
