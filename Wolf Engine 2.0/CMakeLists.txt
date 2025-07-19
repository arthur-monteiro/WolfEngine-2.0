cmake_minimum_required(VERSION 3.31)
project(WolfEngine)

set(CMAKE_CXX_STANDARD 23)

include(WolfEngine.cmake)

# Defines
target_compile_definitions(WolfEngine PUBLIC WOLF_VULKAN)
target_compile_definitions(WolfEngine PUBLIC MATERIAL_DEBUG)
target_compile_definitions(WolfEngine PUBLIC TRACY_ENABLE)
target_compile_definitions(WolfEngine PUBLIC _CRT_SECURE_NO_WARNINGS)
target_compile_definitions(WolfEngine PUBLIC CUSTOM_PUSH_DATA_TO_GPU)
target_compile_definitions(WolfEngine PUBLIC CUSTOM_READBACK_DATA_FROM_GPU)

set_target_properties(WolfEngine
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "../../x64/Debug/lib"
        LIBRARY_OUTPUT_DIRECTORY "../../x64/Debug/lib")

find_package("../Common/Common.cmake" NO_DEFAULT_PATH)
find_package("../GraphicAPIBroker/GraphicAPIBroker_Vulkan.cmake" NO_DEFAULT_PATH)