cmake_minimum_required(VERSION 2.6)
PROJECT(tcpserver C CXX)

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
 -std=c++14
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

 include_directories(${PROJECT_SOURCE_DIR}/src)

 string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
 message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

 get_property(dirs DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
 message(">>> include_dirs=${dirs}")

 add_subdirectory(src)
 add_subdirectory(examples)