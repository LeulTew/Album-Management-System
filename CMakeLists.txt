cmake_minimum_required(VERSION 3.10)
project(AlbumManagementSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(album_management main.cpp manager.cpp)

# Include current directory for headers
target_include_directories(album_management PRIVATE .)

# For Windows, link any needed libraries if necessary
if(WIN32)
    target_link_libraries(album_management)
endif()

# Optional: Add compile options
target_compile_options(album_management PRIVATE -Wall -Wextra)