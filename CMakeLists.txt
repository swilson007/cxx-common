# This cmake is tested so far only on:
#  * MacOS using CLion. (Not xcode. Never xcode!)
cmake_minimum_required(VERSION 3.5)
project(sw-cxx-common)

# Conan support for packages
include(${CMAKE_BINARY_DIR}/conan_paths.cmake OPTIONAL)
# Ensure conan modules are found earlier than others
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOS TRUE)
    message("OSX_SYSROOT=${CMAKE_OSX_SYSROOT}")
endif()

################################################################################
# Determine OS/Release/Debug build dir name
if (MSVC)
    set(OS_BUILD_DIR "win64_v141")    # TODO
    message("Found MSVC")
elseif (MACOS)
    set(OS_BUILD_DIR "macosx64")
    message("Found MacOS")
elseif (LINUX)
    set(OS_BUILD_DIR "linux64")
    message("Found Linux64")
endif()

# Project level variables here
set(CxxCommonDir ${CMAKE_CURRENT_SOURCE_DIR}) # Useful if we're a sub-module
set(ScwSrcDir ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Setup the shared compiler flags/warnings for all subprojects here
# Note that in general we're trying to use per-project settings
set(CMAKE_CXX_EXTENSIONS Off)
list(APPEND StandardCxxCompilerFeatures cxx_std_14)

# These defines are common across OS's
list(APPEND StandardCxxDefines) # TODO

# OS Specifics
if (MSVC)
    # TODO: Try this. Test this. Fix this

    list(APPEND StandardCxxDefines
      -DSW_OS=WINDOWS -DSW_WINDOWS=1 -DNOMINMAX)

    set(CMAKE_CXX_FLAGS_RELEASE "/MP /O2 /DSW_RELEASE=1")
    set(CMAKE_CXX_FLAGS_DEBUG "/MP /DSW_DEBUG=1")

    # Force to always compile with W1
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W1" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W1")
    endif()

elseif (MACOS)
    list(APPEND StandardCxxDefines
      -DSW_OS=MACOS -DSW_UNIX=1 -DSW_POSIX=1 -DSW_MACOS=1)

    # Not sure the "right" way to do this
    set(CMAKE_CXX_FLAGS_DEBUG "-g -DSW_DEBUG=1")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DSW_RELEASE=1")

    # Clang/gcc flags
    list(APPEND StandardCxxWarnings
        -pedantic
        -Werror -Wall -Wextra -Wbind-to-temporary-copy -Wswitch -Warray-bounds -Wconversion -Wformat
        -Wimplicit -Wmove -Wpedantic -Wshadow -Wmissing-braces
        -Wno-mismatched-tags -Wno-format-security)
    # Disable unused warning during dev
    #list(APPEND StandardCxxWarnings -Wno-unused-parameter -Wno-unused-private-field -Wno-unused)
    list(APPEND StandardCxxFlags
        -funsigned-char)

elseif (LINUX)
    list(APPEND StandardCxxDefines
      -DSW_OS=LINUX -DSW_UNIX=1 -DSW_POSIX=1 -DSW_LINUX=1)

    set(CMAKE_CXX_FLAGS_DEBUG "-g -DSW_DEBUG=1")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DSW_RELEASE=1")

    # Clang/gcc flags
    list(APPEND StandardCxxWarnings
        -pedantic
        -Werror -Wall -Wextra -Wbind-to-temporary-copy -Wswitch -Warray-bounds -Wconversion -Wformat
        -Wimplicit -Wmove -Wpedantic -Wshadow
        -Wno-unused-parameter -Wno-unused-private-field -Wno-unused -Wno-mismatched-tags -Wno-format-security)
    list(APPEND StandardCxxFlags
        -funsigned-char)
endif ()

find_package(fmt REQUIRED)

# Setup the library
file(GLOB HeaderFiles sw/*.h)
add_library(sw-cxx-common INTERFACE)

target_sources(${PROJECT_NAME} INTERFACE ${HeaderFiles})
target_include_directories(${PROJECT_NAME} INTERFACE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME} PUBLIC INTERFACE fmt::fmt)

# Our code subdirs. Header only... no src!
add_subdirectory(test)

