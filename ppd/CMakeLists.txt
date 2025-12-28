find_program(PPDC NAMES ppdc REQUIRED)
find_package(Python REQUIRED)

set(
    PPD_FILES
    "${CMAKE_CURRENT_BINARY_DIR}/LBP800CAPTPPD.ppd"
    "${CMAKE_CURRENT_BINARY_DIR}/LBP810CAPTPPD.ppd"
    "${CMAKE_CURRENT_BINARY_DIR}/LBP1120CAPTPPD.ppd"
    "${CMAKE_CURRENT_BINARY_DIR}/LBP1210CAPTPPD.ppd"
    "${CMAKE_CURRENT_BINARY_DIR}/LBP3200CAPTPPD.ppd"
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_LIST_DIR}/captmedia.defs"
    COMMAND
    "${Python_EXECUTABLE}"
    "${CMAKE_CURRENT_LIST_DIR}/genmedia.py"
    "${CMAKE_CURRENT_LIST_DIR}/captmedia.defs"
    DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/genmedia.py"
    VERBATIM
)

add_custom_command(
    OUTPUT ${PPD_FILES}
    COMMAND
    "${PPDC}"
    -D "CAPTBACKEND_VERSION=${PROJECT_VERSION}"
    -D "HAVE_DITHERING_TYPE=$<IF:$<BOOL:${CAPTPPD_DITHERING_OPT}>,1,0>"
    -d "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/captppd.drv"
    DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/captppd.drv"
    "${CMAKE_CURRENT_LIST_DIR}/captmedia.defs"
    VERBATIM
)

add_custom_target(genppd ALL DEPENDS ${PPD_FILES})

install(
    FILES ${PPD_FILES}
    DESTINATION "${CUPS_DATA_DIR}/model"
    COMPONENT base
)
