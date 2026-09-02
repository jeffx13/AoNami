cmake_minimum_required(VERSION 3.16)
if (NOT WIN32)
    message(FATAL_ERROR "This project is Windows-only")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(AoNami VERSION 0.1 LANGUAGES CXX)

set(MAIN_MODULE_URI "AoNami")
string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UPPER)

# MSYS2 root (only used for targeted hints, never added to PREFIX_PATH)
set(MSYS2_ROOT "C:/msys64/mingw64")

# Qt (from Qt installer)
find_package(Qt6 REQUIRED COMPONENTS
    Quick QuickControls2 Qml Concurrent Core5Compat Sql Network Svg WebSockets)

# LibXml2 and zlib from MSYS2 (Miruro's API gzips its replies).
# Hint the Find modules directly - no global path pollution.
set(LIBXML2_INCLUDE_DIR "${MSYS2_ROOT}/include/libxml2" CACHE PATH "" FORCE)
set(LIBXML2_LIBRARY     "${MSYS2_ROOT}/lib/libxml2.dll.a" CACHE FILEPATH "" FORCE)
find_package(LibXml2 REQUIRED)

set(ZLIB_INCLUDE_DIR "${MSYS2_ROOT}/include"        CACHE PATH "" FORCE)
set(ZLIB_LIBRARY     "${MSYS2_ROOT}/lib/libz.dll.a" CACHE FILEPATH "" FORCE)
find_package(ZLIB REQUIRED)

# Project setup
qt_standard_project_setup()
qt_policy(SET QTP0004 NEW)

file(GLOB_RECURSE SOURCES   CONFIGURE_DEPENDS RELATIVE ${PROJECT_SOURCE_DIR} src/*.cpp src/*.h src/*.hpp)
list(REMOVE_ITEM SOURCES src/main.cpp)   # passed to qt_add_executable instead
file(GLOB_RECURSE QML_FILES CONFIGURE_DEPENDS RELATIVE ${PROJECT_SOURCE_DIR} src/ui/qml/*.qml)
file(GLOB_RECURSE RESOURCES CONFIGURE_DEPENDS RELATIVE ${PROJECT_SOURCE_DIR} resources/*)

# Vendored libmpv import lib, plus the runtime binaries fetched by scripts/fetch-deps.ps1.
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
qt_add_executable(${PROJECT_NAME} src/main.cpp aonami.rc)
set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
target_compile_definitions(${PROJECT_NAME} PRIVATE APP_VERSION="${PROJECT_VERSION}")

if(NOT BUILD_TYPE_UPPER STREQUAL "DEBUG")
    target_link_options(${PROJECT_NAME} PRIVATE -s)
endif()

# Our own sources always include the full path from src/. The rest are here only for
# the QML type registration Qt generates, which emits <basename.h> for every
# registered type and finds nothing unless its folder is reachable that way.
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/third-parties/includes
    src
    src/app
    src/net
    src/media
    src/providers
    src/library
    src/ui
)

# MSYS2 includes as SYSTEM to avoid header shadowing
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    ${LIBXML2_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt6::Quick Qt6::QuickControls2 Qt6::Qml
    Qt6::Concurrent Qt6::Core5Compat Qt6::Sql Qt6::Network Qt6::Svg Qt6::WebSockets
    LibXml2::LibXml2
    ZLIB::ZLIB
    ${LIBS}
)
target_link_libraries(${PROJECT_NAME} PRIVATE bcrypt dbghelp psapi)  # CNG + crash minidumps

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

# Copy mpv config (always, even if exe didn't rebuild).
set(_mpv_config_dir "$<TARGET_FILE_DIR:${PROJECT_NAME}>/mpv")

add_custom_target(copy_mpv_config ALL
    # copy_directory only adds, so clear the shaders first - one dropped from the
    # source tree would otherwise live on in the build output forever.
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${_mpv_config_dir}/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/third-parties/mpv-config"
        "${_mpv_config_dir}"
    COMMENT "Syncing mpv config"
)
add_dependencies(copy_mpv_config ${PROJECT_NAME})

# Deploy
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/deploy" CACHE PATH "" FORCE)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

qt_generate_deploy_qml_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
# install(DIRECTORY) only adds too, so a shader dropped from the source tree would
# otherwise keep shipping in every package built over an existing deploy dir.
install(CODE "file(REMOVE_RECURSE \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/mpv/shaders\")")
install(FILES ${BINS} DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY "${PROJECT_SOURCE_DIR}/third-parties/mpv-config/"
    DESTINATION "${CMAKE_INSTALL_BINDIR}/mpv"
)

message(STATUS "** Build type : ${CMAKE_BUILD_TYPE} (${BUILD_TYPE_UPPER})")
message(STATUS "** Deploy dir : ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
