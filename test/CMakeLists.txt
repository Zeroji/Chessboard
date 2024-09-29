cmake_minimum_required(VERSION 3.13)
project(chessboard)
set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES
    src/test.cpp
    src/sensors.cpp
    ../src/chess.cpp
)

add_executable(chessboard ${SOURCE_FILES})