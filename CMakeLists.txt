cmake_minimum_required(VERSION 3.16)

# Require Windows platform
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
list(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")

set(CRYPTOPP_TARGET cryptopp-static)
set(LibXml2_STATIC TRUE)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Qml Concurrent Core5Compat Sql Network)
find_package(CryptoPP REQUIRED)
find_package(LibXml2 REQUIRED)

qt_standard_project_setup()
qt_policy(SET QTP0004 NEW)

# Source and resource files
file(GLOB_RECURSE SOURCES   RELATIVE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*.h ${PROJECT_SOURCE_DIR}/src/*.hpp)
file(GLOB_RECURSE QML_FILES RELATIVE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src/ui/qml/*.qml)
file(GLOB_RECURSE RESOURCES RELATIVE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/resources/*)
file(GLOB LIBS ${PROJECT_SOURCE_DIR}/third-parties/lib/*)
file(GLOB BINS ${PROJECT_SOURCE_DIR}/third-parties/bin/*)
list(APPEND BINS
    "C:/msys64/mingw64/bin/libxml2-2.dll"
    "C:/msys64/mingw64/bin/libiconv-2.dll"
    "C:/msys64/mingw64/bin/liblzma-5.dll"
    "C:/msys64/mingw64/bin/zlib1.dll"
)

qt_add_executable(${PROJECT_NAME} src/app/main.cpp ${APP_ICON_RESOURCE_WINDOWS})
set_target_properties(${PROJECT_NAME}
    PROPERTIES
    WIN32_EXECUTABLE TRUE
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/third-parties/includes
    ${PROJECT_SOURCE_DIR}/src
    ${LIBXML2_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/src/base/player
    ${PROJECT_SOURCE_DIR}/src/app
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt6::Quick
    Qt6::Sql
    Qt6::QuickControls2
    Qt6::Qml
    Qt6::Concurrent
    Qt6::Core5Compat
    Qt6::Network
    LibXml2::LibXml2
    ${CRYPTOPP_TARGET}
    ${LIBS}
)

set_source_files_properties(src/ui/qml/Globals.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    qt6_add_qml_module(${PROJECT_NAME}
        URI ${MAIN_MODULE_URI}
        VERSION 1.0
        RESOURCE_PREFIX "/"
        QML_FILES
        ${QML_FILES}
        SOURCES
        ${SOURCES}
        RESOURCES
        ${RESOURCES}
        NO_CACHEGEN
    )
else()
    qt6_add_qml_module(${PROJECT_NAME}
        URI ${MAIN_MODULE_URI}
        VERSION 1.0
        RESOURCE_PREFIX "/"
        QML_FILES
        ${QML_FILES}
        SOURCES
        ${SOURCES}
        RESOURCES
        ${RESOURCES}
    )
endif()

# Precompiled headers to speed up compilation
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)

# Copy binary files to executable directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying binary files to executable directory..."
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BINS} "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Copying required DLLs and binaries to executable directory"
)

# Deploy Application for Release only
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}" CACHE PATH "Install path" FORCE)
    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        CONFIGURATIONS Release
    )

    qt_generate_deploy_qml_app_script(
        TARGET ${PROJECT_NAME}
        OUTPUT_SCRIPT deploy_script
    )
    install(SCRIPT ${deploy_script} CONFIGURATIONS Release)
    install(FILES ${BINS} DESTINATION bin CONFIGURATIONS Release)
endif()

message(STATUS "** BUILD COMPLETED: ${CMAKE_BUILD_TYPE}")
message(STATUS "** Executable will be in: ${CMAKE_CURRENT_BINARY_DIR}")
