cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(MainProject VERSION 1.0)
configure_file(Version.h.in Version.h)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add the executable
add_executable(main main.cpp Parser.cpp Lexer.cpp)

target_include_directories(main PUBLIC "${PROJECT_BINARY_DIR}")
