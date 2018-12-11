cmake_minimum_required(VERSION 3.0)
project(googletest-download NONE)

set(DownloadsRoot ${CMAKE_HOME_DIRECTORY})
message(gtestdl: ${DownloadsRoot})
include(ExternalProject)
ExternalProject_Add(
        googletest
        GIT_REPOSITORY https://github.com/abseil/googletest.git
        GIT_TAG 2fe3bd994b3189899d93f1d5a881e725e046fdc2 # release-1.8.1
        SOURCE_DIR "${DownloadsRoot}/googletest"
        BUILD_IN_SOURCE true
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        TEST_COMMAND ""
)

