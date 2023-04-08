cmake_minimum_required(VERSION 3.16)

project(Bingime VERSION 0.1 LANGUAGES CXX)

find_package(Qt6 6.2 COMPONENTS Quick REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS QuickControls2 )
find_package(Qt6 REQUIRED COMPONENTS Qml)
find_package(Qt6 REQUIRED COMPONENTS Concurrent)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake;C:/msys64/mingw64/share/cmake/Modules")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

FILE(GLOB_RECURSE FILES ${PROJECT_SOURCE_DIR}/*.cpp  ${PROJECT_SOURCE_DIR}/*.h ${PROJECT_SOURCE_DIR}/*.hpp)

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/bingime.rc")

qt_add_executable(Bingime
    qml.qrc
    main.cpp
    ${FILES}
    ${app_icon_resource_windows}
)

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
include_directories ("C:/msys64/mingw64/include/curl")
target_link_libraries(${PROJECT_NAME} PRIVATE ${CURL_LIBRARIES})
message("** Found libcurl ${CURL_INCLUDE_DIRS} ${CURL_LIBRARIES}")
ENDIF()

find_package(OpenSSL REQUIRED)
IF(OPENSSL_FOUND)
include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENSSL_LIBRARIES})
message("** Found OpenSSL ${OPENSSL_INCLUDE_DIR} ${OPENSSL_LIBRARIES}")
ENDIF()

find_package(CryptoPP REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE cryptopp-static)
message("** Found CryptoPP ${cryptopp-static}")

set_target_properties(Bingime PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)


target_link_libraries(Bingime
    PRIVATE Qt6::Quick
    PRIVATE Qt6::QuickControls2
    PRIVATE Qt6::Qml
    PRIVATE Qt6::Concurrent
)
if (Qt6_FOUND)
    message("** Found Qt6")
    qt6_add_qml_module(${PROJECT_NAME}
        URI bingime_qml
        VERSION 1.0
        AUTO_RESOURCE_PREFIX
#        NO_GENERATE_QMLDIR
#        NO_GENERATE_QMLTYPES
        QML_FILES
        qml/main.qml
        RESOURCES
        icon.jpg
        images/net_grey.png
        images/net_grey_on.png
        images/net_lightgrey.png
        images/net_lightgrey_on.png
        images/pause_grey.png
        images/pause_grey_on.png
        images/pause_lightgrey.png
        images/pause_lightgrey_on.png
        images/play_grey.png
        images/play_grey_on.png
        images/play_lightgrey.png
        images/play_lightgrey_on.png
        images/playlist_grey.png
        images/playlist_grey_on.png
        images/playlist_lightgrey.png
        images/playlist_lightgrey_on.png
        images/settings_grey.png
        images/settings_grey_on.png
        images/settings_lightgrey.png
        images/settings_lightgrey_on.png
        images/stop_grey.png
        images/stop_grey_on.png
        images/stop_lightgrey.png
        images/stop_lightgrey_on.png
        images/volume_grey.png
        images/volume_grey_on.png
        images/volume_lightgrey.png
        images/volume_lightgrey_on.png
    )
endif()

install(TARGETS Bingime
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})


