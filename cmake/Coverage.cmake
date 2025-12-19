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
