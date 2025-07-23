cmake_minimum_required(VERSION 3.16)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(kyokou VERSION 0.1 LANGUAGES CXX)


# message(STATUS "Using msys2 to manage dependencies")
list(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")
set(CRYPTOPP_TARGET cryptopp-static)
set(APP_ICON_RESOURCE_WINDOWS kyokou.rc)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Qml Concurrent Core5Compat)
qt_standard_project_setup()
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

find_package(CryptoPP REQUIRED)
find_package(LibXml2 REQUIRED)


# Source and resource files
file(GLOB_RECURSE SOURCES   RELATIVE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*.h ${PROJECT_SOURCE_DIR}/src/*.hpp)
file(GLOB_RECURSE QML_FILES RELATIVE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src/qml/*.qml)
file(GLOB LIBS ${PROJECT_SOURCE_DIR}/third-parties/lib/*)
file(GLOB BINS ${PROJECT_SOURCE_DIR}/third-parties/bin/*)
list(APPEND BINS
    "C:/msys64/mingw64/bin/libxml2-2.dll"
    "C:/msys64/mingw64/bin/libcryptopp.dll"
    "C:/msys64/mingw64/bin/libiconv-2.dll"
    "C:/msys64/mingw64/bin/liblzma-5.dll"
    "C:/msys64/mingw64/bin/zlib1.dll"
)

# Define executable
qt_add_executable(kyokou
    src/main.cpp
    ${APP_ICON_RESOURCE_WINDOWS}
    kyokou.qrc
)

# Include directories
target_include_directories(kyokou PRIVATE
    ${PROJECT_SOURCE_DIR}/third-parties/includes
    ${PROJECT_SOURCE_DIR}/src
)

# Find and link dependencies
target_link_libraries(kyokou PUBLIC
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
    Qt6::Concurrent
    Qt6::Core5Compat
    LibXml2::LibXml2
    ${CRYPTOPP_TARGET}
    ${LIBS}
)

set(MY_MAIN_MODULE_URI "Kyokou.App.Main")
message(STATUS "MY_MAIN_MODULE_URI: ${MY_MAIN_MODULE_URI}")

if (Qt6_FOUND)
    message("** Found Qt6")
    qt6_add_qml_module(${PROJECT_NAME}
        URI ${MY_MAIN_MODULE_URI}
        VERSION 1.0
        QML_FILES
        ${QML_FILES}
        SOURCES
        ${SOURCES}
    )
endif()

# Executable properties
set_target_properties(kyokou PROPERTIES
    WIN32_EXECUTABLE TRUE
)

set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}" CACHE PATH "Install path" FORCE)
# Installation settings
install(TARGETS kyokou
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Post build
qt_generate_deploy_qml_app_script(
    TARGET kyokou
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

add_custom_command(TARGET kyokou POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_INSTALL_PREFIX}/bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BINS}  "${CMAKE_INSTALL_PREFIX}/bin"
)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE} COMPLETE!")
