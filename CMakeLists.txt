cmake_minimum_required(VERSION 3.10)
project(bintest)

set(PROJECT_NAME "bintest")

set(DEBUG_LEVEL "-g3")
set(OPT_LEVEL "-O0")
set(GCC_FLAGS "${DEBUG_LEVEL} ${OPT_LEVEL} -Wall")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${GCC_FLAGS}")

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/binacpp.cpp
        src/binacpp_utils.cpp
        )

target_link_libraries(${PROJECT_NAME} curl jsoncpp websockets ssl crypto)
