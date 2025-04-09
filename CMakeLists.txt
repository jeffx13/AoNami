cmake_minimum_required(VERSION 3.16)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(kyokou VERSION 0.1 LANGUAGES CXX)

# Qt packages
find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Qml Concurrent Core5Compat)
qt_policy(SET QTP0001 NEW)

option(USE_MSYS2 "Use msys2 to manage dependencies" ON)

# Source and resource files
file(GLOB_RECURSE SOURCES   RELATIVE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*.h ${PROJECT_SOURCE_DIR}/src/*.hpp)
file(GLOB_RECURSE QML_FILES RELATIVE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src/qml/*.qml )
# message(STATUS "QML_FILES: ${QML_FILES}")
set(APP_ICON_RESOURCE_WINDOWS kyokou.rc)

# Define executable
qt_add_executable(kyokou
    src/main.cpp
    ${APP_ICON_RESOURCE_WINDOWS}
    kyokou.qrc
)

message(STATUS "Using msys2 to manage dependencies")
list(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")
set(CRYPTOPP_TARGET cryptopp-static)
set(LIBMPV_INCLUDE_DIR "C:/msys64/mingw64/include")
set(LIBMPV_LIBRARY "C:/msys64/mingw64/lib/libmpv.dll.a")
file(GLOB DLL_FILES "${PROJECT_SOURCE_DIR}/dependencies/dll/*.dll")
add_custom_command(TARGET kyokou POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:kyokou>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL_FILES} "$<TARGET_FILE_DIR:kyokou>"
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
    ${LIBMPV_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/dependencies/includes/ffmpeg
)



# Find and link dependencies
target_link_libraries(kyokou PUBLIC
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
    Qt6::Concurrent
    Qt6::Core5Compat
    CURL::libcurl
    LibXml2::LibXml2
    ${CRYPTOPP_TARGET}
    ${LIBMPV_LIBRARY}
    ${PROJECT_SOURCE_DIR}/dependencies/lib/libavutil.dll.a
    ${PROJECT_SOURCE_DIR}/dependencies/lib/libavformat.dll.a  # Ensure this is correct
    ${PROJECT_SOURCE_DIR}/dependencies/lib/libavcodec.dll.a  # Add this for codec support

)

add_custom_command(TARGET kyokou POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/dependencies/N_m3u8DL-RE.exe"
    "$<TARGET_FILE_DIR:kyokou>/N_m3u8DL-RE.exe")
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

# Installation settings
install(TARGETS kyokou
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
