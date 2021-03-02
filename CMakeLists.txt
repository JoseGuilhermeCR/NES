cmake_minimum_required(VERSION 3.18)

# set the project name
project(Nes)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# bring the headers into the project
include_directories(include)

# wildcard adding source
file(GLOB SOURCES "src/*.cc")

# add the executable
add_executable(Nes ${SOURCES})

target_compile_options(Nes PUBLIC -Wall -Wextra --pedantic-errors)
target_link_libraries(Nes -lSDL2)
