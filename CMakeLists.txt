cmake_minimum_required(VERSION 3.24)
project(lmtc)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED True)

set(SOURCE_FILES
        src/common/argsparse.cpp
        src/common/utils.cpp
        src/compiler/main.cpp
        src/compiler/tokenstream.cpp
        src/compiler/tokenmatching.cpp
        src/compiler/parser.cpp
        src/compiler/typeinfo.cpp
        src/compiler/intermediate.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
# include relative to src
target_include_directories(${PROJECT_NAME} PRIVATE src)
# required for backtrace
target_link_libraries(${PROJECT_NAME} stdc++exp)

