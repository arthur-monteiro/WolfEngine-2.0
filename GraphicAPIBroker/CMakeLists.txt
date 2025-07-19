cmake_minimum_required(VERSION 3.31)
project(GraphicAPIBroker)

set(CMAKE_CXX_STANDARD 23)

include(GraphicAPIBroker_Vulkan.cmake)

add_library(GraphicAPIBroker STATIC ${SRC})

# Defines
target_compile_definitions(GraphicAPIBroker PUBLIC WOLF_VULKAN)

set_target_properties(GraphicAPIBroker
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "../../x64/Debug/lib"
        LIBRARY_OUTPUT_DIRECTORY "../../x64/Debug/lib")