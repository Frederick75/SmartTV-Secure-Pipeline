cmake_minimum_required(VERSION 3.16)
project(tee_svp_client C)

add_library(tee_svp_client STATIC tee_svp_client.c)
target_include_directories(tee_svp_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tee_svp_client PUBLIC teec)
