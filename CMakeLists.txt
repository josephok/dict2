cmake_minimum_required(VERSION 3.1)

project(dict VERSION 1.0
             DESCRIPTION "query dict in terminal"
             LANGUAGES C CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -g3")

include(FindPkgConfig)
pkg_check_modules(CURL libcurl REQUIRED)
include_directories(
  SYSTEM ${CURL_INCLUDE_DIRS}
)

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
    message("Debug Build")
    add_definitions(-DDEBUG)
elseif(${CMAKE_BUILD_TYPE} MATCHES Release)
    message("Release Build")
endif()

FILE(GLOB SOURCES ${CMAKE_SOURCE_DIR}/src/*)
include_directories(${CMAKE_SOURCE_DIR}/lib)

add_library(sqlite3 STATIC ${CMAKE_SOURCE_DIR}/lib/sqlite3.c)
target_link_libraries(sqlite3 pthread dl m)
set_target_properties(sqlite3 PROPERTIES LINKER_LANGUAGE C)

add_library(pugixml STATIC ${CMAKE_SOURCE_DIR}/lib/pugixml.cpp)

add_executable(dict ${SOURCES})
target_link_libraries(dict sqlite3)
target_link_libraries(dict ${CURL_LIBRARIES})
target_link_libraries(dict pugixml)

install(TARGETS dict DESTINATION bin)
