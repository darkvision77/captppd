find_package(GTest)
if(NOT GTest_FOUND)
    message(STATUS "GTest not found, using FetchContent")
    set(INSTALL_GTEST OFF)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
    )
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)
endif()

set(
    TEST_FILES
    "PrinterInfoTest"
    "StatusMessageTest"
    "StateReporterTest"
)

foreach(file ${TEST_FILES})
    add_executable(${file} ${file}.cpp)
    target_link_libraries(${file} PRIVATE GTest::gtest_main GTest::gmock libcaptbackend)
    gtest_discover_tests(${file})
endforeach()
