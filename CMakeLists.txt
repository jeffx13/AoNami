cmake_minimum_required(VERSION 3.16)

project(kyokou VERSION 0.1 LANGUAGES CXX)

# Qt packages
find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Qml Concurrent Core5Compat)
qt_policy(SET QTP0001 NEW)

# Automatically handle the Qt MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(USE_VCPKG "Use vcpkg to manage dependencies" OFF)
option(USE_MSYS2 "Use msys2 to manage dependencies" ON)

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


if(USE_VCPKG)
    message(STATUS "Using vcpkg to manage dependencies")
    include("C:/Program Files/Microsoft Visual Studio/2022/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CRYPTOPP_TARGET cryptopp::cryptopp)
endif()
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")


if(USE_MSYS2)
    message(STATUS "Using msys2 to manage dependencies")
    list(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")
    set(CRYPTOPP_TARGET cryptopp-static)
endif()


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
target_link_libraries(kyokou PUBLIC
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
    Qt6::Concurrent
    Qt6::Core5Compat
    ${PROJECT_SOURCE_DIR}/dependencies/lib/libmpv.dll.a
    CURL::libcurl
    LibXml2::LibXml2
    ${CRYPTOPP_TARGET}

)

# Ensure libmpv.dll is copied to the output directory
add_custom_command(TARGET kyokou POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/dependencies/lib/libmpv-2.dll"
    "$<TARGET_FILE_DIR:kyokou>/libmpv-2.dll")
add_custom_command(TARGET kyokou POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/dependencies/ffmpeg.exe"
    "$<TARGET_FILE_DIR:kyokou>/ffmpeg.exe")
add_custom_command(TARGET kyokou POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/dependencies/N_m3u8DL-CLI_v3.0.2.exe"
    "$<TARGET_FILE_DIR:kyokou>/N_m3u8DL-CLI_v3.0.2.exe")

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
