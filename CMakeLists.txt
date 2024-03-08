cmake_minimum_required(VERSION 3.20)

function(get_version_from_file VER_FILE_NAME)
    file(READ ${VER_FILE_NAME} ER_VERSION_RAW)
    # Remove trailing whitespaces and/or newline
    string(STRIP ${ER_VERSION_RAW} ER_VERSION_)
    set(ER_VERSION ${ER_VERSION_} CACHE STRING
        "Project version determined from version.txt" FORCE
    )
    message(STATUS "Determined project version ${ER_VERSION}")
endfunction()


get_version_from_file("version.txt")

project(erebus-gui VERSION ${ER_VERSION})

if(NOT CMAKE_BUILD_TYPE)
    message(WARNING "CMAKE_BUILD_TYPE isn't defined. Debug value will be used by default.")
    set(CMAKE_BUILD_TYPE Debug)
else()
    if(${CMAKE_BUILD_TYPE} MATCHES "^[Rr][Ee][Ll][Ee][Aa][Ss][Ee]$")
        set(CMAKE_BUILD_TYPE Release)
    elseif(${CMAKE_BUILD_TYPE} MATCHES "^[Dd][Ee][Bb][Uu][Gg]$")
        set(CMAKE_BUILD_TYPE Debug)
    else()
        message(WARNING "CMAKE_BUILD_TYPE isn't Release or Debug. Debug value will be used by default.")
        set(CMAKE_BUILD_TYPE Debug)
    endif()
endif()

# -fPIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# -fvisibility=hidden
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if(NOT MSVC)
    add_link_options(-rdynamic) # enable stack capture
endif()

set(ER_LINUX 0)
set(ER_WINDOWS 0)
set(ER_POSIX 0)

set(CMAKE_CXX_STANDARD 20)
set(EREBUS_CXX_FEATURES cxx_std_20)

if(WIN32)
    set(ER_WINDOWS 1)
elseif(UNIX AND NOT APPLE)
    set(ER_LINUX 1)
    set(ER_POSIX 1)
else()
    message(FATAL_ERROR "Unsupported OS")
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

# require Boost
if(MSVC)
    find_package(Boost REQUIRED COMPONENTS system program_options)
else()
    find_package(Boost REQUIRED COMPONENTS 
        stacktrace_basic
        stacktrace_backtrace
        stacktrace_addr2line
        stacktrace_noop 
        system
        program_options
    )
endif()


include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

if(WIN32)
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
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/client-version.h.in client-version.h)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${PROJECT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/submodules/erebus-server/include ${CMAKE_CURRENT_SOURCE_DIR}/include)


# erebus-server dependencies
set(EREBUS_RTLLIB erebus-rtl)
set(EREBUS_CLTLIB erebus-clt)

# component names
set(EREBUS_CLIENT erebus-gui)
set(EREBUS_GUILIB erebus)
set(EREBUS_ICONCACHE erebus-iconcache)

add_subdirectory(submodules/erebus-server)
add_subdirectory(src/erebus-gui)
add_subdirectory(src/erebus-processmgr)
add_subdirectory(src/client)
if(ER_LINUX)
    add_subdirectory(src/erebus-iconcache)
endif()
