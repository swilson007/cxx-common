project(cxx-common-unit-tests)

find_package(gtest REQUIRED) 

# Auto-find test sources
file(GLOB TestSources "*.cpp" )

# Test Sources
set(AllSources ${TestSources})

add_executable(${PROJECT_NAME} ${AllSources})

target_compile_features(${PROJECT_NAME} PRIVATE ${StandardCxxCompilerFeatures})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${StandardCxxDefines})
target_compile_options(${PROJECT_NAME} PRIVATE ${StandardCxxWarnings})
target_compile_options(${PROJECT_NAME} PRIVATE ${StandardCxxFlags})

target_include_directories(${PROJECT_NAME} PRIVATE ${ScwSrcDir})

target_link_libraries(${PROJECT_NAME} GTest::GTest GTest::Main)

