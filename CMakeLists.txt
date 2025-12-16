cmake_minimum_required(VERSION 4.0)
project(duck_hunt C)

set(CMAKE_C_STANDARD 11)

add_executable(duck_hunt main.c
        graficos.c
        serial.c
        simplecontroller.c)
