cmake_minimum_required(VERSION 2.6)
PROJECT(serverlite C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(CXX_FLAGS
  -g
  # -DVALGRIND
  -DCHECK_PTHREAD_RETURN_VALUE
  -D_FILE_OFFSET_BITS=64
  -Wall
  -Wextra
  -Werror
  -Wconversion
  -Wno-unused-parameter
  -Wold-style-cast
  -Woverloaded-virtual
  -Wpointer-arith
  -Wshadow
  -Wwrite-strings
  -march=native
  # -MMD
  -std=c++17
  -rdynamic
)
if(CMAKE_BUILD_BITS EQUAL 32)
  list(APPEND CXX_FLAGS "-m32")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  list(APPEND CXX_FLAGS "-Wno-null-dereference")
  list(APPEND CXX_FLAGS "-Wno-sign-conversion")
  list(APPEND CXX_FLAGS "-Wno-unused-local-typedef")
  list(APPEND CXX_FLAGS "-Wthread-safety")
  list(REMOVE_ITEM CXX_FLAGS "-rdynamic")
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

find_package(Protobuf)
find_package(ZLIB)
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h)
find_library(HIREDIS_LIBRARY NAMES hiredis)

if(PROTOBUF_FOUND)
  message(STATUS "found protobuf")
endif()
if(ZLIB_FOUND)
  message(STATUS "found zlib")
endif()
if(HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARY)
  message(STATUS "found hiredis")
endif()

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/build/debug/src)

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

get_property(dirs DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
message(">>> include_dirs=${dirs}")

add_subdirectory(src/slite)
add_subdirectory(examples)