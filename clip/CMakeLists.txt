cmake_minimum_required(VERSION 3.1.2)

project (clip)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories (.)

if (WIN32)
  add_definitions (-D_SCL_SECURE_NO_WARNINGS)
  set (CLIP_SOURCES clip.cpp image.cpp clip_win.cpp)
else()
  set (CLIP_SOURCES clip.cpp image.cpp clip_none.cpp)
endif()

add_library (clip ${CLIP_SOURCES})

if (WIN32)
  target_link_libraries (clip shlwapi)
endif()
