cmake_minimum_required(VERSION 3.22)

project(
  MemoryManagement
  VERSION 1.0
  DESCRIPTION "This is the memory management assignment for CMPT
276."
  LANGUAGES C)

add_executable(main src/main.c src/alloc.c)

include_directories(include)
