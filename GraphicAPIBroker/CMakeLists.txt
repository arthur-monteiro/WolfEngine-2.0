cmake_minimum_required(VERSION 3.31)
project(GraphicAPIBroker)

set(CMAKE_CXX_STANDARD 23)

include(GraphicAPIBroker_Vulkan.cmake)

add_library(GraphicAPIBroker STATIC ${SRC})

# Defines
target_compile_definitions(GraphicAPIBroker PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_compile_definitions(GraphicAPIBroker PUBLIC GLM_FORCE_RADIANS)
target_compile_definitions(GraphicAPIBroker PUBLIC GLM_ENABLE_EXPERIMENTAL)
target_compile_definitions(GraphicAPIBroker PUBLIC WOLF_VULKAN)

set_target_properties(GraphicAPIBroker
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../x64/${CMAKE_BUILD_TYPE}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../x64/${CMAKE_BUILD_TYPE}/lib"
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_LIST_DIR}/../x64/Debug/lib"
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_LIST_DIR}/../x64/Release/lib")