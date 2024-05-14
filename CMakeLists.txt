cmake_minimum_required(VERSION 3.20)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build configuration specified, defaulting to Release")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build configuration" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/submodules/erebus-server/cmake)

include(version)
get_version_from_file("version.txt")

project(erebus-gui VERSION ${ER_VERSION})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/client-version.h.in client-version.h)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/a)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)


if(WIN32)
    set(ER_WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
    set(ER_LINUX TRUE)
    set(ER_POSIX TRUE)
else()
    message(FATAL_ERROR "Unsupported OS")
endif()


# require erebus-server libs
set(ER_BUILD_CLIENT_LIBS_ONLY TRUE)
add_subdirectory(submodules/erebus-server)

# require Boost
find_package(Boost REQUIRED COMPONENTS filesystem iostreams system program_options)


include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

if(ER_WINDOWS)
    # disable autolinking in boost
    add_definitions(-DBOOST_ALL_NO_LIB)
    # force all boost libraries to dynamic link
    add_definitions(-DBOOST_ALL_DYN_LINK)
endif()

# force shared CRT
if(MSVC)
    cmake_policy(SET CMP0091 NEW)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()


# Windows-specific settings
if(ER_WINDOWS)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()


# require Qt6
find_package(Qt6Core REQUIRED)
find_package(Qt6Gui REQUIRED)
find_package(Qt6Widgets REQUIRED)
find_package(Qt6Network REQUIRED)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt6Widgets_INCLUDES also includes the include directories for QtCore and QtGui
include_directories(${Qt6Widgets_INCLUDES} ${Qt6Network_INCLUDES})
# We need to add -DQT_WIDGETS_LIB when using QtWidgets in Qt 6
add_definitions(${Qt6Widgets_DEFINITIONS} ${Qt6Network_DEFINITIONS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(
    ${PROJECT_BINARY_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/submodules/erebus-server/include 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


set(EREBUS_RTLLIB erebus-rtl)
set(EREBUS_CLTLIB erebus-clt)


# component names
set(EREBUS_CLIENT erebus-gui)
set(EREBUS_GUILIB erebus)
set(EREBUS_ICONCACHE erebus-iconcache)

add_subdirectory(src/erebus-gui)
add_subdirectory(src/erebus-processmgr)
add_subdirectory(src/client)
if(ER_LINUX)
    add_subdirectory(src/erebus-iconcache)
endif()
