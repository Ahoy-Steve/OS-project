cmake_minimum_required(VERSION 4.2)
project(OS_Project C)

set(CMAKE_C_STANDARD 99)

add_executable(OS_Project city_manager.c
        monitor_reports.c
        scorer.c)
