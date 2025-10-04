cmake_minimum_required(VERSION 3.31)
project(Common)

set(CMAKE_CXX_STANDARD 23)

include(Common.cmake)

add_library(Common STATIC ${SRC})

set_target_properties(Common
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../x64/Debug/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../x64/Debug/lib")