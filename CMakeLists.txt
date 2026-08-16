cmake_minimum_required(VERSION 3.16)
if (NOT WIN32 AND NOT APPLE)
    message(FATAL_ERROR "This project supports Windows and macOS")
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

# LibXml2
if(WIN32)
    # From MSYS2. Hint the FindLibXml2 module directly - no global path pollution.
    set(LIBXML2_INCLUDE_DIR "${MSYS2_ROOT}/include/libxml2" CACHE PATH "" FORCE)
    set(LIBXML2_LIBRARY     "${MSYS2_ROOT}/lib/libxml2.dll.a" CACHE FILEPATH "" FORCE)
endif()
find_package(LibXml2 REQUIRED)

# libmpv: vendored import lib on Windows, pkg-config (Homebrew) on macOS.
if(APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)
endif()

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

# Third-party vendored libs/bins (mpv, etc.). macOS gets libmpv from Homebrew and
# resolves the rest of its runtime through the system, so both lists stay Windows-only.
if(WIN32)
    file(GLOB LIBS ${PROJECT_SOURCE_DIR}/third-parties/lib/*)
    file(GLOB BINS ${PROJECT_SOURCE_DIR}/third-parties/bin/*)

    # MSYS2 runtime DLLs needed alongside the exe
    list(APPEND BINS
        "${MSYS2_ROOT}/bin/libxml2-16.dll"
        "${MSYS2_ROOT}/bin/libiconv-2.dll"
        "${MSYS2_ROOT}/bin/liblzma-5.dll"
        "${MSYS2_ROOT}/bin/zlib1.dll"
    )
endif()

# Executable
if(WIN32)
    qt_add_executable(${PROJECT_NAME} src/main.cpp ${APP_ICON_RESOURCE_WINDOWS})
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
else()
    qt_add_executable(${PROJECT_NAME} src/main.cpp)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.jeffx13.aonami"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE APP_VERSION="${PROJECT_VERSION}")

# ld64 has no bare -s; macOS strips at install time instead.
if(WIN32 AND NOT BUILD_TYPE_UPPER STREQUAL "DEBUG")
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

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE bcrypt dbghelp psapi)  # CNG + crash minidumps
elseif(APPLE)
    # CommonCrypto rides along in libSystem; IOKit is the display-sleep assertion.
    target_link_libraries(${PROJECT_NAME} PRIVATE
        PkgConfig::MPV
        "-framework IOKit"
        "-framework CoreFoundation"
    )
endif()

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
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BINS} "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copying runtime DLLs"
    )
endif()

# Copy mpv config (always, even if exe didn't rebuild). Inside the bundle on macOS,
# where MpvPlayer looks for it relative to applicationDirPath().
if(APPLE)
    set(_mpv_config_dir "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/MacOS/mpv")
else()
    set(_mpv_config_dir "$<TARGET_FILE_DIR:${PROJECT_NAME}>/mpv")
endif()

add_custom_target(copy_mpv_config ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/third-parties/mpv"
        "${_mpv_config_dir}"
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
if(WIN32)
    install(FILES ${BINS} DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/third-parties/mpv/"
        DESTINATION "${CMAKE_INSTALL_BINDIR}/mpv"
    )
else()
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/third-parties/mpv/"
        DESTINATION "${PROJECT_NAME}.app/Contents/MacOS/mpv"
    )
endif()

message(STATUS "** Build type : ${CMAKE_BUILD_TYPE} (${BUILD_TYPE_UPPER})")
message(STATUS "** Deploy dir : ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")