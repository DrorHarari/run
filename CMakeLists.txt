cmake_minimum_required(VERSION 3.10)


project(Run VERSION 1.0)

set(SOURCES src/run.c src/Everything.c include/Everything.h ipc/Everything_IPC.h)

add_executable(Run ${SOURCES})

target_include_directories(Run
 PRIVATE
  ./
  )
