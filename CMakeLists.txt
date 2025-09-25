cmake_minimum_required(VERSION 3.10)
project(AlbumManagementSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(album_management main.cpp manager.cpp)

# Include current directory for headers
target_include_directories(album_management PRIVATE .)

# Link filesystem library for C++17 filesystem support
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(album_management stdc++fs)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_link_libraries(album_management c++fs)
endif()

# For Windows, link any needed libraries if necessary
if(WIN32)
    target_link_libraries(album_management)
endif()

# Optional: Add compile options
target_compile_options(album_management PRIVATE -Wall -Wextra)

# Google Test setup
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Test executable
add_executable(tests test_artist.cpp test_album.cpp test_validation.cpp test_repository.cpp test_integration.cpp)
target_link_libraries(tests gtest_main)
target_include_directories(tests PRIVATE .)

include(GoogleTest)
gtest_discover_tests(tests)