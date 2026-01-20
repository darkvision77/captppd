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
option(CAPTPPD_DITHERING_OPT "Enable dithering option in PPD" ON)
set(CAPTPPD_BACKEND_NAME "captusb" CACHE STRING "Backend name")

add_compile_options(-Wall -Wextra -Wpedantic)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(CheckIPOSupported)
check_ipo_supported(RESULT LTO_SUPPORTED)
if(LTO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO OFF)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)
endif()

if(CAPTPPD_COVERAGE)
    include(Coverage)
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

include(Packaging)
include(Summary)
