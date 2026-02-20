cmake_minimum_required(VERSION 3.22)
project(GraphicAPIBroker)

set(CMAKE_CXX_STANDARD 23)

include(GraphicAPIBroker_Vulkan.cmake)

add_library(GraphicAPIBroker STATIC ${SRC})

# Defines
target_compile_definitions(GraphicAPIBroker PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_compile_definitions(GraphicAPIBroker PUBLIC GLM_FORCE_RADIANS)
target_compile_definitions(GraphicAPIBroker PUBLIC GLM_ENABLE_EXPERIMENTAL)
target_compile_definitions(GraphicAPIBroker PUBLIC WOLF_VULKAN)

if(ANDROID)
    set(ABI_DIR "/${ANDROID_ABI}")
else()
    set(ABI_DIR "")
endif()

set_target_properties(GraphicAPIBroker
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../x64/${CMAKE_BUILD_TYPE}/lib${ABI_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../x64/${CMAKE_BUILD_TYPE}/lib${ABI_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_LIST_DIR}/../x64/Debug/lib${ABI_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_LIST_DIR}/../x64/Release/lib${ABI_DIR}")