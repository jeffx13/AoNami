cmake_minimum_required(VERSION 3.16)

project(kyokou VERSION 0.1 LANGUAGES CXX)


# Qt packages
find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Qml Concurrent Core5Compat)

# Automatically handle the Qt MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Additional CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake;C:/msys64/mingw64/share/cmake/Modules")

# Source and resource files
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*.h ${PROJECT_SOURCE_DIR}/src/*.hpp)
file(GLOB_RECURSE QML_FILES ${PROJECT_SOURCE_DIR}/src/qml/*.qml)
set(APP_ICON_RESOURCE_WINDOWS kyokou.rc)

# Define executable
qt_add_executable(kyokou
    ${SOURCES}
    ${QML_FILES}
    ${PROJECT_SOURCE_DIR}/dependencies/includes/pugixml/pugixml.cpp
    ${APP_ICON_RESOURCE_WINDOWS}
    kyokou.qrc

)

# Include directories
target_include_directories(kyokou PRIVATE
    ${PROJECT_SOURCE_DIR}/dependencies/includes
    ${PROJECT_SOURCE_DIR}/src
    ${TIDY_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

# Find and link dependencies
find_package(TIDY REQUIRED)
find_package(CURL REQUIRED)
find_package(CryptoPP REQUIRED)

target_link_libraries(kyokou PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
    Qt6::Concurrent
    Qt6::Core5Compat
    ${PROJECT_SOURCE_DIR}/dependencies/lib/libmpv.dll.a
    ${TIDY_LIBRARIES}
    ${CURL_LIBRARIES}
    cryptopp-static
)

# Debug and Release specific settings
target_compile_options(kyokou PRIVATE
    "$<$<CONFIG:DEBUG>:-Og>"
)

# add_executable(launcher
#     launcher/main.cpp
#     #    ${app_icon_resource_windows}
# )
# add_subdirectory(${PROJECT_SOURCE_DIR}/launcher/)


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


