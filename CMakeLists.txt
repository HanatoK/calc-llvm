cmake_minimum_required(VERSION 3.10)
SET_PROPERTY(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)

message(STATUS "Using CMAKE prefix path ${CMAKE_PREFIX_PATH}")
list(APPEND CMAKE_PREFIX_PATH "/usr/lib64/cmake")
# require LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# set the project name and version
project(MainProject VERSION 1.0)
configure_file(Version.h.in Version.h)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add the executable
add_executable(main main.cpp Parser.cpp Lexer.cpp AbstractSyntaxTree.cpp Driver.cpp)

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_config(main USE_SHARED core irreader support)

# Link against LLVM libraries
#target_link_libraries(main ${llvm_libs})

target_include_directories(main PUBLIC "${PROJECT_BINARY_DIR}")
