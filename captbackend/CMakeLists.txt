include(CheckCXXSourceCompiles)

check_cxx_source_compiles("
#include <stop_token>
int main() {
    std::stop_token token;
    return 0;
}
" HAVE_STOP_TOKEN)

if(NOT HAVE_STOP_TOKEN)
    message(STATUS "Compiler does not support std::stop_token, using fallback implementation")
    set(HAVE_STOP_TOKEN 0)
endif()

configure_file(Config.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/Config.hpp @ONLY)

find_package(libcapt)
if(NOT libcapt_FOUND)
    message(STATUS "libcapt not found, using FetchContent")
    include(FetchContent)
    FetchContent_Declare(
        libcapt
        GIT_REPOSITORY https://github.com/darkvision77/libcapt.git
        GIT_TAG v0.15.0
    )
    FetchContent_MakeAvailable(libcapt)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

add_library(libcaptbackend STATIC)
set_target_properties(libcaptbackend PROPERTIES PREFIX "" OUTPUT_NAME "lib${CAPTPPD_BACKEND_NAME}")
target_include_directories(libcaptbackend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${LIBUSB_INCLUDE_DIRS})
target_link_libraries(libcaptbackend PUBLIC libcapt::libcapt ${LIBUSB_LIBRARIES})

target_compile_options(libcaptbackend PUBLIC ${CUPS_CFLAGS})
target_link_libraries(libcaptbackend PUBLIC ${CUPS_LIBS} ${CUPS_LDFLAGS})

add_executable(captbackend main.cpp)
set_target_properties(captbackend PROPERTIES OUTPUT_NAME "${CAPTPPD_BACKEND_NAME}")
target_link_libraries(captbackend PRIVATE libcaptbackend)

add_subdirectory(Core)
add_subdirectory(Cups)
add_subdirectory(UsbBackend)

install(
    TARGETS captbackend
    DESTINATION "${CUPS_SERVER_BIN}/backend"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    COMPONENT base
)
