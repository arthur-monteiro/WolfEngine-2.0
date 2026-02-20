cmake_minimum_required(VERSION 3.22)
project(Common)

set(CMAKE_CXX_STANDARD 23)

include(Common.cmake)

add_library(Common STATIC ${SRC})

if(ANDROID)
    set(ABI_DIR "/${ANDROID_ABI}")
else()
    set(ABI_DIR "")
endif()

set_target_properties(Common
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../x64/${CMAKE_BUILD_TYPE}/lib${ABI_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../x64/${CMAKE_BUILD_TYPE}/lib${ABI_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_LIST_DIR}/../x64/Debug/lib${ABI_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_LIST_DIR}/../x64/Release/lib${ABI_DIR}")