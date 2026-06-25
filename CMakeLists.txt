cmake_minimum_required(VERSION 3.16)
if (NOT WIN32)
    message(FATAL_ERROR "This project requires Windows platform")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(AoNami VERSION 0.1 LANGUAGES CXX)

set(MAIN_MODULE_URI "AoNami")
set(APP_ICON_RESOURCE_WINDOWS aonami.rc)
string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UPPER)

# MSYS2 root (only used for targeted hints, never added to PREFIX_PATH) 
set(MSYS2_ROOT "C:/msys64/mingw64")

# Qt (from Qt installer) 
find_package(Qt6 REQUIRED COMPONENTS
    Quick QuickControls2 Qml Concurrent Core5Compat Sql Network Svg)

# LibXml2 (from MSYS2) 
# Hint the FindLibXml2 module directly - no global path pollution.
set(LIBXML2_INCLUDE_DIR "${MSYS2_ROOT}/include/libxml2" CACHE PATH "" FORCE)
set(LIBXML2_LIBRARY     "${MSYS2_ROOT}/lib/libxml2.dll.a" CACHE FILEPATH "" FORCE)
find_package(LibXml2 REQUIRED)

# Crypto++ (from MSYS2, static) 
# Disabled: nothing in the build needs it. Uncomment this + the cryptopp link below to revive.
# add_library(cryptopp STATIC IMPORTED)
# set_target_properties(cryptopp PROPERTIES
#     IMPORTED_LOCATION             "${MSYS2_ROOT}/lib/libcryptopp.a"
#     INTERFACE_INCLUDE_DIRECTORIES "${MSYS2_ROOT}/include"
# )

# Project setup 
qt_standard_project_setup()
qt_policy(SET QTP0004 NEW)

file(GLOB_RECURSE SOURCES   CONFIGURE_DEPENDS RELATIVE ${PROJECT_SOURCE_DIR} src/*.cpp src/*.h src/*.hpp)
file(GLOB_RECURSE QML_FILES CONFIGURE_DEPENDS RELATIVE ${PROJECT_SOURCE_DIR} src/ui/qml/*.qml)
file(GLOB_RECURSE RESOURCES CONFIGURE_DEPENDS RELATIVE ${PROJECT_SOURCE_DIR} resources/*)

# Third-party vendored libs/bins (mpv, etc.)
file(GLOB LIBS ${PROJECT_SOURCE_DIR}/third-parties/lib/*)
file(GLOB BINS ${PROJECT_SOURCE_DIR}/third-parties/bin/*)

# MSYS2 runtime DLLs needed alongside the exe
list(APPEND BINS
    "${MSYS2_ROOT}/bin/libxml2-16.dll"
    "${MSYS2_ROOT}/bin/libiconv-2.dll"
    "${MSYS2_ROOT}/bin/liblzma-5.dll"
    "${MSYS2_ROOT}/bin/zlib1.dll"
)

# Executable 
qt_add_executable(${PROJECT_NAME} src/main.cpp ${APP_ICON_RESOURCE_WINDOWS})
set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
target_compile_definitions(${PROJECT_NAME} PRIVATE APP_VERSION="${PROJECT_VERSION}")

if(NOT BUILD_TYPE_UPPER STREQUAL "DEBUG")
    target_link_options(${PROJECT_NAME} PRIVATE -s)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/third-parties/includes
    src
    src/player
    src/app
)

# MSYS2 includes as SYSTEM to avoid header shadowing
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    ${LIBXML2_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt6::Quick Qt6::QuickControls2 Qt6::Qml
    Qt6::Concurrent Qt6::Core5Compat Qt6::Sql Qt6::Network Qt6::Svg
    LibXml2::LibXml2
    # cryptopp   # disabled - see the commented imported target above
    ${LIBS}
)
target_link_libraries(${PROJECT_NAME} PRIVATE bcrypt)  # Windows CNG (allanime AES-GCM)

# QML Module 
set_source_files_properties(src/ui/qml/Globals.qml src/ui/qml/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

set(_qml_extra_args "")
if(BUILD_TYPE_UPPER STREQUAL "DEBUG")
    list(APPEND _qml_extra_args NO_CACHEGEN)
endif()

qt6_add_qml_module(${PROJECT_NAME}
    URI ${MAIN_MODULE_URI}
    VERSION 1.0
    RESOURCE_PREFIX "/"
    QML_FILES   ${QML_FILES}
    SOURCES     ${SOURCES}
    RESOURCES   ${RESOURCES}
    ${_qml_extra_args}
)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)

# Copy DLLs post-build (only when exe rebuilds) 
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BINS} "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMENT "Copying runtime DLLs"
)

# Copy mpv config (always, even if exe didn't rebuild) 
add_custom_target(copy_mpv_config ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/third-parties/mpv"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/mpv"
    COMMENT "Syncing mpv config"
)
add_dependencies(copy_mpv_config ${PROJECT_NAME})

# Deploy 
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/deploy" CACHE PATH "" FORCE)

install(TARGETS ${PROJECT_NAME}
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
install(FILES ${BINS} DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY "${PROJECT_SOURCE_DIR}/third-parties/mpv/"
    DESTINATION "${CMAKE_INSTALL_BINDIR}/mpv"
)

message(STATUS "** Build type : ${CMAKE_BUILD_TYPE} (${BUILD_TYPE_UPPER})")
message(STATUS "** Deploy dir : ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")