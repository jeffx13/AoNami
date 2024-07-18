cmake_minimum_required(VERSION 3.16)

project(kyokou VERSION 0.1 LANGUAGES CXX)

# Qt packages
find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Qml Concurrent Core5Compat WebEngineQuick WebEngineCore)
qt_policy(SET QTP0001 NEW)

# Automatically handle the Qt MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set toolchain file for vcpkg
set(CMAKE_TOOLCHAIN_FILE "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake")
include("C:/Program Files/Microsoft Visual Studio/2022/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake")

# Set compiler flags for debug configuration
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /RTC1")

# Source and resource files
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*.h ${PROJECT_SOURCE_DIR}/src/*.hpp)
file(GLOB_RECURSE QML_FILES ${PROJECT_SOURCE_DIR}/src/qml/*.qml)
set(APP_ICON_RESOURCE_WINDOWS kyokou.rc)

# Define executable
qt_add_executable(kyokou
    ${SOURCES}
    ${QML_FILES}
    ${APP_ICON_RESOURCE_WINDOWS}
    kyokou.qrc
)

find_package(CURL REQUIRED)
find_package(CryptoPP REQUIRED)
find_package(LibXml2 REQUIRED)



# Include directories
target_include_directories(kyokou PRIVATE
    ${PROJECT_SOURCE_DIR}/dependencies/includes
    ${PROJECT_SOURCE_DIR}/src
    ${CURL_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIR}
)

# Find and link dependencies
target_link_libraries(kyokou PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
    Qt6::Concurrent
    Qt6::Core5Compat
    Qt6::WebEngineQuick
    Qt6::WebEngineCore
    ${PROJECT_SOURCE_DIR}/dependencies/lib/libmpv.dll.a
    CURL::libcurl
    cryptopp::cryptopp
    LibXml2::LibXml2
)

# Ensure libmpv.dll is copied to the output directory
add_custom_command(TARGET kyokou POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/dependencies/lib/libmpv-2.dll"
    "$<TARGET_FILE_DIR:kyokou>/libmpv-2.dll")

# Debug and Release specific settings
target_compile_options(kyokou PRIVATE
    "$<$<CONFIG:DEBUG>:-Od>"
)
if (Qt6_FOUND)
    message("** Found Qt6")
    qt6_add_qml_module(${PROJECT_NAME}
        URI kyokou_qml
        VERSION 1.0
        NO_GENERATE_QMLDIR
        NO_GENERATE_QMLTYPES
        SOURCES ${QML}
    )
endif()

# Executable properties
set_target_properties(kyokou PROPERTIES
    WIN32_EXECUTABLE TRUE
)

# Installation settings
install(TARGETS kyokou
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
