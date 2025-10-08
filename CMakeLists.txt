cmake_minimum_required(VERSION 3.31)
project(WolfEngine)

set(CMAKE_CXX_STANDARD 23)

file(GLOB SRC
        "Wolf-Engine-2.0/*.cpp"
        "ThirdParty/Tracy/TracyClient.cpp"
)

# Includes Wolf libs
include_directories(Common)
include_directories(GraphicAPIBroker/Public)

# Includes third parties
include_directories(ThirdParty/xxh64)
include_directories(ThirdParty/glm)
include_directories(ThirdParty/GLFW/include)
include_directories(ThirdParty/tiny_obj)
include_directories(ThirdParty/FrustumCull)
include_directories(ThirdParty/stb_image)
include_directories(ThirdParty/UltraLight/include)
include_directories(ThirdParty/vulkan/Include)
include_directories(ThirdParty/Tracy/tracy)

add_library(WolfEngine STATIC ${SRC})

# Defines
target_compile_definitions(WolfEngine PUBLIC WOLF_VULKAN)
target_compile_definitions(WolfEngine PUBLIC MATERIAL_DEBUG)
target_compile_definitions(WolfEngine PUBLIC TRACY_ENABLE)
target_compile_definitions(WolfEngine PUBLIC TRACY_ON_DEMAND)
target_compile_definitions(WolfEngine PUBLIC _CRT_SECURE_NO_WARNINGS)
target_compile_definitions(WolfEngine PUBLIC CUSTOM_PUSH_DATA_TO_GPU)
target_compile_definitions(WolfEngine PUBLIC CUSTOM_READBACK_DATA_FROM_GPU)

option(RESOURCE_TRACKING "Enable resource tracking features" OFF)
option(RESOURCE_DEBUG "Enable resource debug features" OFF)

if (RESOURCE_TRACKING OR RESOURCE_DEBUG)
    target_compile_definitions(
            WolfEngine
            PRIVATE
            $<$<BOOL:RESOURCE_TRACKING>:RESOURCE_TRACKING>
            $<$<BOOL:RESOURCE_DEBUG>:RESOURCE_DEBUG>
    )
endif()

add_subdirectory("GenerateCodeFileHashes")
add_subdirectory("Common")
add_subdirectory("GraphicAPIBroker")

add_custom_command(COMMAND "$<TARGET_FILE:GenerateCodeFileHashes>"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Wolf-Engine-2.0"
        OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/Wolf-Engine-2.0/CodeFileHashes.h"
        DEPENDS GenerateCodeFileHashes)

add_custom_target(
        GenerateHashesTarget
        ALL
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Wolf-Engine-2.0/CodeFileHashes.h"
)

add_dependencies(WolfEngine GenerateHashesTarget)

set_target_properties(WolfEngine
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/x64/${CMAKE_BUILD_TYPE}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/x64/${CMAKE_BUILD_TYPE}/lib"
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_LIST_DIR}/x64/Debug/lib"
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_LIST_DIR}/x64/Release/lib")