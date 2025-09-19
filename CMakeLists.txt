# Minimum CMake version required
cmake_minimum_required(VERSION 3.20)

set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel.")

# Project information
project(TestBigNum CXX)

# Set the required C++ standard.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Add doctest
include(FetchContent)
FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG master
)
FetchContent_MakeAvailable(doctest)

# Add compiler-specific flags
if(MSVC)
    set(CMAKE_CXX_FLAGS "/W4 /WX /EHsc /utf-8")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
elseif(UNIX)
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -march=native")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
endif()

# Define the source files
set(SOURCE_FILES
    BigNumTest.cpp
)

# Add executable target
add_executable(testbignum ${SOURCE_FILES})
target_link_libraries(testbignum PRIVATE doctest)
target_include_directories(testbignum PRIVATE ${doctest_SOURCE_DIR}/doctest)

# Enable testing with CTest
enable_testing()

# Add the test executable to CTest
add_test(NAME testbignum COMMAND testbignum)
