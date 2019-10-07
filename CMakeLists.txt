cmake_minimum_required(VERSION 3.12.0)

project(uc2 LANGUAGES CXX)

#
# Put Visual Studio resulting binaries out of their config directory
#
if(MSVC)
  set(PKG_POSSIBLE_CONFIGS "Debug;Release;MinSizeRel;RelWithDebInfo")
  foreach(CUR_CONFIG ${PKG_POSSIBLE_CONFIGS})
    string(TOUPPER ${CUR_CONFIG} CUR_CONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CUR_CONFIG} "${OUTPUT_DIRECTORY}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CUR_CONFIG} "${OUTPUT_DIRECTORY}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CUR_CONFIG} "${OUTPUT_DIRECTORY}")
  endforeach()
endif()

option(UNCSO2_USE_LTO "Use 'Link Time Optimizations'" ON)
option(UNCSO2_USE_CLANG_FSAPI "Use libc++fs when available" OFF)

set(UNCSO2_ROOT_DIR "${PROJECT_SOURCE_DIR}")
set(UNCSO2_LIBS_DIR "${UNCSO2_ROOT_DIR}/external")
set(UNCSO2_RESOURCES_DIR "${UNCSO2_ROOT_DIR}/resources")
set(UNCSO2_LIB_GSL_DIR "${UNCSO2_LIBS_DIR}/gsl-lite")
set(UNCSO2_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")

set(UNCSO2_ICON_SVG "${UNCSO2_RESOURCES_DIR}/uncso2.svg")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake_modules")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(UNCSO2_VERSION_FILE "${UNCSO2_ROOT_DIR}/version.txt")
set(UNCSO2_VERSION_TEMPLATE "${CMAKE_SOURCE_DIR}/headers/uc2_version.hpp.in")
set(UNCSO2_VERSION_OUT "${UNCSO2_GENERATED_DIR}/uc2_version.hpp")
set(UNCSO2_GIT_DIR "${CMAKE_SOURCE_DIR}/.git")

include(uc2_version)
generate_uc2_version(VERSION_FILE
                     ${UNCSO2_VERSION_FILE}
                     HEADER_TEMPLATE
                     ${UNCSO2_VERSION_TEMPLATE}
                     HEADER_OUT
                     ${UNCSO2_VERSION_OUT}
                     GIT_DIR
                     ${UNCSO2_GIT_DIR})

if(UNCSO2_USE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT
                      UNCSO2_IPO_SUPPORTED
                      OUTPUT
                      UNCSO2_IPO_ERR)

  if(UNCSO2_IPO_SUPPORTED)
    message(STATUS "UnCSO2: Using Link Time Optimizations")
  else()
    message(
      FATAL_ERROR
        "Link time optimization is not supported: <${CMAKE_INTERPROCEDURAL_OPTIMIZATION}>"
      )
  endif()
endif()

#
# find dependencies
#
if(UNCSO2_USE_CLANG_FSAPI)
  set(PKG_USE_CLANG_FSAPI TRUE CACHE BOOL "" FORCE)
endif()

add_subdirectory("libuncso2")

find_package(Qt5 COMPONENTS Widgets REQUIRED)
if(WIN32)
  find_package(Qt5 COMPONENTS WinExtras REQUIRED)
endif()

# Auto generate Qt files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Set the .ui files path
set(CMAKE_AUTOUIC_SEARCH_PATHS "${UNCSO2_ROOT_DIR}/resources/layouts")

#
# add source files to the project
#
set(UNCSO2_SOURCES_BASE
    "sources/archivebasenode.cpp"
    "sources/archivedirectorynode.cpp"
    "sources/archivefilenode.cpp"
    "sources/busywinwrapper.cpp"
    "sources/dynindexfilefactory.cpp"
    "sources/dynpkgfilefactory.cpp"
    "sources/fileproperties.cpp"
    "sources/fsutils.cpp"
    "sources/gamedatainfo.cpp"
    "sources/main.cpp"
    "sources/nodeextractionmgr.cpp"
    "sources/pkgfilemodel.cpp"
    "sources/pkgfilemodelsorter.cpp"
    "sources/pkgfileview.cpp"
    "sources/specialfilehandler.cpp"
    "sources/uncso2app.cpp")

set(UNCSO2_SOURCES_LAYOUTS
    "sources/layouts/aboutdialog.cpp"
    "sources/layouts/indexpropertiesdialog.cpp"
    "sources/layouts/loadindexdialog.cpp"
    "sources/layouts/mainwindow.cpp"
    "sources/layouts/pkgpropertiesdialog.cpp")

set(UNCSO2_SOURCES_WIDGETS "sources/widgets/errorbox.cpp"
                           "sources/widgets/statuswidget.cpp")

set(UNCSO2_HEADERS_BASE
    "headers/archivebasenode.hpp"
    "headers/archivedirectorynode.hpp"
    "headers/archivefilenode.hpp"
    "headers/busywinwrapper.hpp"
    "headers/dynindexfilefactory.hpp"
    "headers/dynpkgfilefactory.hpp"
    "headers/fileproperties.hpp"
    "headers/fsutils.hpp"
    "headers/gamedatainfo.hpp"
    "headers/indexkeycollections.hpp"
    "headers/miscutils.hpp"
    "headers/nodeextractionmgr.hpp"
    "headers/pkgfilemodel.hpp"
    "headers/pkgfilemodelsorter.hpp"
    "headers/pkgfilesystemshared.hpp"
    "headers/pkgfileview.hpp"
    "headers/specialfilehandler.hpp"
    "headers/uncso2app.hpp"
    ${UNCSO2_VERSION_OUT})

set(UNCSO2_HEADERS_LAYOUTS
    "headers/layouts/aboutdialog.hpp"
    "headers/layouts/indexpropertiesdialog.hpp"
    "headers/layouts/loadindexdialog.hpp"
    "headers/layouts/mainwindow.hpp"
    "headers/layouts/pkgpropertiesdialog.hpp")

set(UNCSO2_HEADERS_WIDGETS "headers/widgets/errorbox.hpp"
                           "headers/widgets/statuswidget.hpp")

set(UNCSO2_RESOURCES "resources/icons-uncso2.qrc")

set(UNCSO2_RESOURCES_BREEZE "resources/icons-breeze.qrc")

set(UNCSO2_RESOURCES_LAYOUTS
    "resources/layouts/aboutdialog.ui"
    "resources/layouts/loadindexdialog.ui"
    "resources/layouts/mainwindow.ui"
    "resources/layouts/pkgpropertiesdialog.ui")

if(WIN32)
  set(UNCSO2_RESOURCES_WIN32 "resources/win32/uncso2.ico"
                             "resources/win32/uncso2.rc")
else()
  set(UNCSO2_RESOURCES_WIN32)
endif()

source_group("Source Files" FILES ${UNCSO2_SOURCES_BASE})
source_group("Source Files\\Layouts" FILES ${UNCSO2_SOURCES_LAYOUTS})
source_group("Source Files\\Widgets" FILES ${UNCSO2_SOURCES_WIDGETS})
source_group("Header Files" FILES ${UNCSO2_HEADERS_BASE})
source_group("Header Files\\Layouts" FILES ${UNCSO2_HEADERS_LAYOUTS})
source_group("Header Files\\Widgets" FILES ${UNCSO2_HEADERS_WIDGETS})
source_group("Resources Files" FILES ${UNCSO2_RESOURCES})
source_group("Resources Files\\Breeze" FILES ${UNCSO2_RESOURCES_BREEZE})
source_group("Resources Files\\Layouts" FILES ${UNCSO2_RESOURCES_LAYOUTS})

qt5_add_binary_resources(icons-breeze ${UNCSO2_RESOURCES_BREEZE})

file(GLOB
     UNCSO2_ALL_SOURCES
     ${UNCSO2_SOURCES_BASE}
     ${UNCSO2_SOURCES_LAYOUTS}
     ${UNCSO2_SOURCES_WIDGETS}
     ${UNCSO2_HEADERS_BASE}
     ${UNCSO2_HEADERS_LAYOUTS}
     ${UNCSO2_HEADERS_WIDGETS}
     ${UNCSO2_RESOURCES_LAYOUTS}
     ${UNCSO2_RESOURCES_WIN32})

qt5_add_resources(UNCSO2_ALL_SOURCES ${UNCSO2_RESOURCES})

# force c++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Add executable to build.
#
if(WIN32)
  add_executable(uc2 WIN32 ${UNCSO2_ALL_SOURCES})
else()
  add_executable(uc2 ${UNCSO2_ALL_SOURCES})
endif()

# add fallback menu icons
add_dependencies(uc2 icons-breeze)

#
# Enable all warnings
#
if(MSVC)
  target_compile_options(uc2 PRIVATE /W4 /WX)
else()
  target_compile_options(uc2
                         PRIVATE -Wall
                                 -Wextra
                                 -Wconversion
                                 -pedantic)
  target_compile_options(uc2 PRIVATE -g)
endif()

if(UNCSO2_USE_LTO)
  set_property(TARGET uc2 PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  set_property(TARGET uncso2 PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(PKG_BUILD_SHARED)
  message("uncso2: Using libuncso2 as a shared library")
  target_compile_definitions(uc2 PRIVATE UNCSO2_SHARED)
else()
  message("uncso2: Using libuncso2 as a static library")
  target_compile_definitions(uc2 PRIVATE UNCSO2_STATIC)
endif()

#
# Setup include directories
#
target_include_directories(uc2 PRIVATE "headers")
target_include_directories(uc2 PRIVATE "headers/layouts")

# the generated version header's directory
target_include_directories(uc2 PRIVATE ${UNCSO2_GENERATED_DIR})

#
# link libraries
#
target_include_directories(uc2 PRIVATE "${UNCSO2_LIB_GSL_DIR}/include")

if(UNCSO2_USE_CLANG_FSAPI)
  message(STATUS "uc2: Using libc++fs")
  target_link_libraries(uc2 c++fs)
elseif(NOT MSVC)
  message(STATUS "uc2: Using stdlibc++fs")
  target_link_libraries(uc2 stdc++fs)
endif()

# link libuncso2
target_include_directories(uc2 PRIVATE ${PKG_INCLUDE_DIR})
target_link_libraries(uc2 uncso2)

# link Qt5
target_include_directories(uc2 PRIVATE ${Qt5Widgets_INCLUDE_DIRS})
target_link_libraries(uc2 Qt5::Widgets)
if(WIN32)
  target_link_libraries(uc2 Qt5::WinExtras)
endif()
