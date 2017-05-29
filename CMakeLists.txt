cmake_minimum_required(VERSION 2.6)
project(feedserver)

# set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" CACHE STRING "compile flags" FORCE)

add_definitions(
        -std=gnu99
        -ggdb3
        -pedantic
        -Wall
        -Wextra
        -Wcast-align
        -w)

set(FDSRV_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(FDSRV_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/testing)
set(DEPENDENCIES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies)
set(DEPENDENCIES_FIXED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dependencies_fixed)

file(GLOB_RECURSE FDSRV_SOURCES
    ${FDSRV_SOURCE_DIR}/*.h
    ${FDSRV_SOURCE_DIR}/*.c
    ${FDSRV_SOURCE_DIR}/*.cpp
    ${DEPENDENCIES_FIXED_DIR}/opt_parse/opt.h
    ${DEPENDENCIES_FIXED_DIR}/opt_parse/opt.c
    ${DEPENDENCIES_FIXED_DIR}/opt_parse/parse.c
    ${DEPENDENCIES_DIR}/json-parser/json.h
    ${DEPENDENCIES_DIR}/json-parser/json.c
    ${DEPENDENCIES_FIXED_DIR}/vc_vector/vc_vector.h
    ${DEPENDENCIES_FIXED_DIR}/vc_vector/vc_vector.c)
list(SORT FDSRV_SOURCES)

include_directories(
        ${FDSRV_SOURCE_DIR}
        ${DEPENDENCIES_DIR}/haywire/include
        ${DEPENDENCIES_DIR}/haywire/lib/libuv/include
        ${DEPENDENCIES_FIXED_DIR}/opt_parse
        ${DEPENDENCIES_FIXED_DIR}/vc_vector
        ${DEPENDENCIES_DIR}/json-parser
        ${DEPENDENCIES_DIR}/rocksdb/include)

find_package(Threads REQUIRED)

add_executable(feedserver ${FDSRV_SOURCES})


target_link_libraries(feedserver
        ${DEPENDENCIES_DIR}/haywire/build/libhaywire.a
        ${DEPENDENCIES_DIR}/haywire/lib/libuv/.libs/libuv.a
        m
        ${DEPENDENCIES_DIR}/rocksdb/librocksdb.a
        ${CMAKE_THREAD_LIBS_INIT}
        rt z bz2 numa)

enable_testing()

add_executable(test-userfeed
  ${FDSRV_TEST_DIR}/test-userfeed.c
  ${FDSRV_SOURCE_DIR}/userfeed.h
  ${FDSRV_SOURCE_DIR}/userfeed.c
  ${FDSRV_SOURCE_DIR}/vc_string.h
  ${FDSRV_SOURCE_DIR}/vc_string.c
  ${FDSRV_SOURCE_DIR}/json-traverse.h
  ${FDSRV_SOURCE_DIR}/json-traverse.c
  ${DEPENDENCIES_DIR}/json-parser/json.h
  ${DEPENDENCIES_DIR}/json-parser/json.c
  ${DEPENDENCIES_FIXED_DIR}/vc_vector/vc_vector.h
  ${DEPENDENCIES_FIXED_DIR}/vc_vector/vc_vector.c
)
target_link_libraries(test-userfeed m)
add_test(test-userfeed test-userfeed ../testing/test-userfeed.json)

add_executable(test-feedarticle
  ${FDSRV_TEST_DIR}/test-feedarticle.c
  ${FDSRV_SOURCE_DIR}/feedarticle.h
  ${FDSRV_SOURCE_DIR}/feedarticle.c
  ${FDSRV_SOURCE_DIR}/vc_string.h
  ${FDSRV_SOURCE_DIR}/vc_string.c
  ${FDSRV_SOURCE_DIR}/json-traverse.h
  ${FDSRV_SOURCE_DIR}/json-traverse.c
  ${DEPENDENCIES_DIR}/json-parser/json.h
  ${DEPENDENCIES_DIR}/json-parser/json.c
  ${DEPENDENCIES_FIXED_DIR}/vc_vector/vc_vector.h
  ${DEPENDENCIES_FIXED_DIR}/vc_vector/vc_vector.c
)
target_link_libraries(test-feedarticle m)
add_test(test-feedarticle test-feedarticle ../testing/test-feedarticle.json)
