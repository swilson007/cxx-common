project(test-all)

# BEGIN: Quick hack for googletest support. For now, using ExternalProject to download it.
set(DownloadsRoot ${CMAKE_BINARY_DIR}/downloads)
configure_file(${CMAKE_HOME_DIRECTORY}/cmake/downloads-template.cmake ${DownloadsRoot}/CMakeLists.txt COPYONLY)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . WORKING_DIRECTORY ${DownloadsRoot})
execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${DownloadsRoot} )
set(GoogletestRoot ${DownloadsRoot}/googletest)
# END: Quick hack for googletest support. For now, using ExternalProject to download it.

# Auto-find test sources
file(GLOB TestSources "*.cpp" )

# Taking the simplistic "compile gtest-all.cc" directly into the project approach
# The main reason is I'm not sure how to compile it properly using my compilers
# options via the ExternalProject call. #someday
set(GoogletestDir ${GoogletestRoot}/googletest)

# Test Sources
set(AllSources ${TestSources}
        ${GoogletestDir}/src/gtest_main.cc
        ${GoogletestDir}/src/gtest-all.cc)

add_executable(${PROJECT_NAME} ${AllSources})

target_compile_features(${PROJECT_NAME} PUBLIC ${StandardCxxCompilerFeatures})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${StandardCxxDefines})
target_compile_options(${PROJECT_NAME} PUBLIC ${StandardCxxWarnings})
target_compile_options(${PROJECT_NAME} PUBLIC ${StandardCxxFlags})

target_include_directories(${PROJECT_NAME} PUBLIC ${ScwSrcRoot})
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${GoogletestDir}/include ${GoogletestDir})


