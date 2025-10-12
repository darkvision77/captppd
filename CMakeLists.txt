cmake_minimum_required(VERSION 3.21)

project(
    captppd
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(CAPTPPD_BUILD_TESTS "Build tests" OFF)
option(CAPTPPD_COVERAGE "Enable code coverage" OFF)
option(CAPTPPD_SANITIZE "Enable address and undefined sanitizers" OFF)
option(CAPTPPD_DITHERING_OPT "Enable dithering option in PPD" OFF)
set(CAPTPPD_BACKEND_NAME "captusb" CACHE STRING "Backend name")

add_compile_options(-Wall -Wextra)

if(CAPTPPD_COVERAGE)
    add_compile_options(--coverage -g)
    add_link_options(--coverage)

    find_program(LCOV NAMES lcov REQUIRED)
    find_program(GENHTML NAMES genhtml REQUIRED)

    set(LCOV_DIR "${CMAKE_BINARY_DIR}/coverage")
    set(LCOV_FILE "${LCOV_DIR}/coverage.info")
    file(MAKE_DIRECTORY "${LCOV_DIR}")
    add_custom_target(
        coverage
        COMMAND
        "${LCOV}"
        --capture --all
        --config-file "${CMAKE_SOURCE_DIR}/.lcovrc"
        --directory "${CMAKE_BINARY_DIR}"
        --base-directory "${CMAKE_SOURCE_DIR}"
        --output-file "${LCOV_FILE}"
        --demangle-cpp
        --parallel ${CMAKE_BUILD_PARALLEL_LEVEL}
        --quiet
        COMMAND
        "${GENHTML}"
        -o "${LCOV_DIR}"
        "${LCOV_FILE}"
        --parallel ${CMAKE_BUILD_PARALLEL_LEVEL}
        COMMAND echo "Coverage report can be found at file://${LCOV_DIR}/index.html"
        VERBATIM
    )

    add_custom_target(
        coverage-reset
        COMMAND
        "${LCOV}"
        --directory "${CMAKE_BINARY_DIR}"
        --zerocounters
        VERBATIM
    )
endif()

if(CAPTPPD_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
endif()

find_program(CUPS_CONFIG NAMES cups-config REQUIRED)
function(cups_config_opt VAR OPT)
    execute_process(
        COMMAND "${CUPS_CONFIG}" ${OPT}
        OUTPUT_VARIABLE out
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(${VAR} ${out} PARENT_SCOPE)
endfunction()

if(NOT CUPS_SERVER_BIN)
    cups_config_opt(CUPS_SERVER_BIN --serverbin)
endif()
if(NOT CUPS_DATA_DIR)
    cups_config_opt(CUPS_DATA_DIR --datadir)
endif()
if(NOT CUPS_CFLAGS)
    cups_config_opt(CUPS_CFLAGS --cflags)
endif()
if(NOT CUPS_LDFLAGS)
    cups_config_opt(CUPS_LDFLAGS --ldflags)
endif()
if(NOT CUPS_LIBS)
    cups_config_opt(CUPS_LIBS --libs)
endif()

if(CAPTPPD_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(captbackend)
add_subdirectory(ppd)
add_subdirectory(dist)
if(CAPTPPD_BUILD_TESTS)
    add_subdirectory(tests)
endif()

message(STATUS "Summary")
message(STATUS "  System                : ${CMAKE_SYSTEM_NAME}")
message(STATUS "  C++ compiler          : ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  CMake version         : ${CMAKE_VERSION}")
message(STATUS "  Build type            : ${CMAKE_BUILD_TYPE}")
message(STATUS "  CXXFLAGS              : ${CMAKE_CXX_FLAGS}")
message(STATUS "  CUPS_SERVER_BIN       : ${CUPS_SERVER_BIN}")
message(STATUS "  CUPS_DATA_DIR         : ${CUPS_DATA_DIR}")
message(STATUS "  CUPS_CFLAGS           : ${CUPS_CFLAGS}")
message(STATUS "  CUPS_LDFLAGS          : ${CUPS_LDFLAGS}")
message(STATUS "  CUPS_LIBS             : ${CUPS_LIBS}")
message(STATUS "  CAPTPPD_BUILD_TESTS   : ${CAPTPPD_BUILD_TESTS}")
message(STATUS "  CAPTPPD_COVERAGE      : ${CAPTPPD_COVERAGE}")
message(STATUS "  CAPTPPD_SANITIZE      : ${CAPTPPD_SANITIZE}")
message(STATUS "  CAPTPPD_DITHERING_OPT : ${CAPTPPD_DITHERING_OPT}")
message(STATUS "  CAPTPPD_BACKEND_NAME  : ${CAPTPPD_BACKEND_NAME}")
