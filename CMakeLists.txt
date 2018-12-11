# This cmake is tested so far only on:
#  * MacOS using CLion. (Not xcode. Never xcode!)
cmake_minimum_required(VERSION 3.5)
project(cxx-common)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
endif()

################################################################################
# Determine OS/Release/Debug build dir name
if (MSVC)
    set(OS_BUILD_DIR "win64_v141")    # TODO
elseif (MACOSX)
    set(OS_BUILD_DIR "macosx64")
elseif (LINUX)
    set(OS_BUILD_DIR "linux64")
endif()

# Project level variables here
set(ScwSrcRoot ${CMAKE_SOURCE_DIR}/src)

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
            -DSCW_OS=WINDOWS -DSCW_WINDOWS=1 -DNOMINMAX)

    set(CMAKE_CXX_FLAGS_RELEASE "/MP /O2 /DSCW_RELEASE=1")
    set(CMAKE_CXX_FLAGS_DEBUG "/MP /DSCW_DEBUG=1")

    # Force to always compile with W1
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W1" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W1")
    endif()

elseif (MACOSX)
    list(APPEND StandardCxxDefines
        -DSCW_OS=MACOS -DSCW_UNIX=1 -DSCW_MACOS=1)

    # Not sure the "right" way to do this
    set(CMAKE_CXX_FLAGS_DEBUG "-g -DSCW_DEBUG=1")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DSCW_RELEASE=1")

    # Clang/gcc flags
    list(APPEND StandardCxxWarnings
        -pedantic
        -Werror -Wall -Wextra -Wbind-to-temporary-copy -Wswitch -Warray-bounds -Wconversion -Wformat
        -Wimplicit -Wmove -Wpedantic -Wshadow
        -Wno-unused-parameter -Wno-unused-private-field -Wno-unused -Wno-mismatched-tags -Wno-format-security)
    list(APPEND StandardCxxFlags
        -funsigned-char)

elseif (LINUX)
    list(APPEND StandardCxxDefines
        -DSCW_OS=LINUX -DSCW_UNIX=1 -DSCW_LINUX=1)

    set(CMAKE_CXX_FLAGS_DEBUG "-g -DSCW_DEBUG=1")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DSCW_RELEASE=1")

    # Clang/gcc flags
    list(APPEND StandardCxxWarnings
        -pedantic
        -Werror -Wall -Wextra -Wbind-to-temporary-copy -Wswitch -Warray-bounds -Wconversion -Wformat
        -Wimplicit -Wmove -Wpedantic -Wshadow
        -Wno-unused-parameter -Wno-unused-private-field -Wno-unused -Wno-mismatched-tags -Wno-format-security)
    list(APPEND StandardCxxFlags
        -funsigned-char)
endif ()

# Our code subdirs. Header only... no src!
add_subdirectory(test)
