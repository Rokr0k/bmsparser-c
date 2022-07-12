cmake_minimum_required(VERSION 3.10)

project(bmsparser)

add_library(bmsparser SHARED "src/bmsparser.c" "src/table.h" "src/convert.c" "src/sort.h" "src/sort.c")

target_include_directories(bmsparser PUBLIC "include/")