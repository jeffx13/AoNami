cmake_minimum_required(VERSION 3.16)

project(kyokou VERSION 0.1 LANGUAGES CXX)

find_package(Qt6 6.2 COMPONENTS Quick REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS QuickControls2)
find_package(Qt6 REQUIRED COMPONENTS Qml)
find_package(Qt6 REQUIRED COMPONENTS Concurrent)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake;C:/msys64/mingw64/share/cmake/Modules")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FILE(GLOB_RECURSE FILES ${PROJECT_SOURCE_DIR}/src/*.cpp  ${PROJECT_SOURCE_DIR}/src/*.h ${PROJECT_SOURCE_DIR}/src/*.hpp)
set(app_icon_resource_windows kyokou.rc)
qt_add_executable(kyokou
    qml.qrc
    src/main.cpp
    ${FILES}
    ${PROJECT_SOURCE_DIR}/dependencies/includes/pugixml/pugixml.cpp
    ${app_icon_resource_windows}
)
include_directories(${PROJECT_SOURCE_DIR}/dependencies/includes)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/dependencies/includes)
include_directories(${PROJECT_SOURCE_DIR}/src)

find_package(Libmpv REQUIRED)
IF(Libmpv_FOUND)
include_directories(${Libmpv_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Libmpv_LIBRARIES})
message("** Using libmpv " ${Libmpv_VERSION})
message(STATUS "Found libmpv ${MPV_INCLUDE_DIR}")
ENDIF()

find_package(TIDY REQUIRED)
IF(TIDY_FOUND)
include_directories(${TIDY_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${TIDY_LIBRARIES})
message("** Found Tidy ${TIDY_INCLUDE_DIRS} ${TIDY_LIBRARIES}")
ENDIF()

find_package(CURL REQUIRED)
IF(CURL_FOUND)
include_directories(${CURL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${CURL_LIBRARIES})
message("** Found libcurl ${CURL_INCLUDE_DIRS} ${CURL_LIBRARIES}")
ENDIF()

find_package(CryptoPP REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE cryptopp-static)
message("** Found CryptoPP ${cryptopp-static}")

set_target_properties(kyokou PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)


target_link_libraries(kyokou
    PRIVATE Qt6::Quick
    PRIVATE Qt6::QuickControls2
    PRIVATE Qt6::Qml
    PRIVATE Qt6::Concurrent
)
if (Qt6_FOUND)
    message("** Found Qt6")
    qt6_add_qml_module(${PROJECT_NAME}
        URI kyokou_qml
        VERSION 1.0
        AUTO_RESOURCE_PREFIX
#        NO_GENERATE_QMLDIR
#        NO_GENERATE_QMLTYPES
    )
endif()

install(TARGETS kyokou
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})


