

#project(kyokou_launcher VERSION 0.1 LANGUAGES CXX)

#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake;C:/msys64/mingw64/share/cmake/Modules")
#set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#FILE(GLOB_RECURSE FILES ${PROJECT_SOURCE_DIR}/launcher/*.cpp  ${PROJECT_SOURCE_DIR}/launcher/*.h ${PROJECT_SOURCE_DIR}/launcher/*.hpp)
#set(app_icon_resource_windows kyokou.rc)

#message ("${FILES}")
#include_directories(${PROJECT_SOURCE_DIR}/dependencies/includes)
#target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/dependencies/includes)
#include_directories(${PROJECT_SOURCE_DIR}/src)
message ("${PROJECT_SOURCE_DIR}")

set_target_properties(launcher PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
#    WIN32_EXECUTABLE TRUE
)

install(TARGETS launcher
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})


