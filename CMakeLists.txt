cmake_minimum_required(VERSION 3.5)

project(textedit VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Network REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Network REQUIRED)

set(PROJECT_SOURCES
        src/main.cpp
        src/textedit.cpp
        src/textedit.h
        src/localserver.cpp
        src/localserver.h
        src/serialization.cpp
        src/serialization.h
        src/textedit.qrc
)


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(textedit
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET textedit APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(textedit SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(textedit
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(textedit PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
                               PRIVATE Qt${QT_VERSION_MAJOR}::Network)

set_target_properties(textedit PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(textedit)
endif()


install(TARGETS textedit
    RUNTIME DESTINATION "/usr/lib/"
    LIBRARY DESTINATION "/usr/lib/"
    DESTINATION "/usr/bin/"
)

set(CPACK_GENERATOR "DEB")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "/usr/bin/")
set(CPACK_PACKAGE_VENDOR "Vendor")
set(CPACK_VERBATIM_VARIABLES true)
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Maintainer")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
include(CPack)
