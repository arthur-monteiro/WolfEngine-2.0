cmake_minimum_required(VERSION 3.22)
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
include_directories(ThirdParty/tiny_obj)
include_directories(ThirdParty/FrustumCull)
include_directories(ThirdParty/stb_image)
if(WIN32)
    include_directories(ThirdParty/UltraLight/windows/include)
    include_directories(ThirdParty/vulkan/Include)
    include_directories(ThirdParty/GLFW/include)
elseif(ANDROID)
    include_directories(ThirdParty/UltraLight/linux/include)
    include_directories(${ANDROID_NDK}/sources/third_party/shaderc/libshaderc/include)
    find_package(game-activity REQUIRED CONFIG)
    get_target_property(GAME_ACTIVITY_INCLUDES game-activity::game-activity INTERFACE_INCLUDE_DIRECTORIES)
    include_directories(${GAME_ACTIVITY_INCLUDES})
elseif(UNIX AND NOT APPLE)
    include_directories(ThirdParty/UltraLight/linux/include)
    find_package(Vulkan REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED glfw3)
endif()
include_directories(ThirdParty/Tracy/tracy)
include_directories(ThirdParty/meshoptimizer/src)

add_library(WolfEngine STATIC ${SRC})

# Defines
target_compile_definitions(WolfEngine PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_compile_definitions(WolfEngine PUBLIC GLM_FORCE_RADIANS)
target_compile_definitions(WolfEngine PUBLIC GLM_ENABLE_EXPERIMENTAL)
target_compile_definitions(WolfEngine PUBLIC WOLF_VULKAN)
target_compile_definitions(WolfEngine PUBLIC MATERIAL_DEBUG)
target_compile_definitions(WolfEngine PUBLIC TRACY_ENABLE)
target_compile_definitions(WolfEngine PUBLIC TRACY_ON_DEMAND)
if(ANDROID)
    target_compile_definitions(WolfEngine PUBLIC GLM_FORCE_CXX14)
    target_compile_definitions(WolfEngine PUBLIC TRACY_TIMER_FALLBACK=1)
endif ()
target_compile_definitions(WolfEngine PUBLIC _CRT_SECURE_NO_WARNINGS)

option(RESOURCE_TRACKING "Enable resource tracking features" OFF)
option(RESOURCE_DEBUG "Enable resource debug features" OFF)

if(NOT ANDROID)
    add_subdirectory("GenerateCodeFileHashes")
endif()
add_subdirectory("Common")
add_subdirectory("GraphicAPIBroker")

if (RESOURCE_TRACKING OR RESOURCE_DEBUG)
    target_compile_definitions(
            WolfEngine
            PRIVATE
            $<$<BOOL:RESOURCE_TRACKING>:RESOURCE_TRACKING>
            $<$<BOOL:RESOURCE_DEBUG>:RESOURCE_DEBUG>
    )

    target_compile_definitions(
            Common
            PRIVATE
            $<$<BOOL:RESOURCE_TRACKING>:RESOURCE_TRACKING>
            $<$<BOOL:RESOURCE_DEBUG>:RESOURCE_DEBUG>
    )

    target_compile_definitions(
            GraphicAPIBroker
            PRIVATE
            $<$<BOOL:RESOURCE_TRACKING>:RESOURCE_TRACKING>
            $<$<BOOL:RESOURCE_DEBUG>:RESOURCE_DEBUG>
    )
endif()

if(NOT ANDROID)
add_custom_command(COMMAND "$<TARGET_FILE:GenerateCodeFileHashes>" "../Wolf-Engine-2.0"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Wolf-Engine-2.0"
        OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/Wolf-Engine-2.0/CodeFileHashes.h"
        DEPENDS GenerateCodeFileHashes
        VERBATIM)

    add_custom_target(
            GenerateHashesTarget
            ALL
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Wolf-Engine-2.0/CodeFileHashes.h"
    )
add_dependencies(WolfEngine GenerateHashesTarget)
endif()


if(ANDROID)
    set(ABI_DIR "/${ANDROID_ABI}")
else()
    set(ABI_DIR "")
endif()

set_target_properties(WolfEngine
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/x64/${CMAKE_BUILD_TYPE}/lib${ABI_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/x64/${CMAKE_BUILD_TYPE}/lib${ABI_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_LIST_DIR}/x64/Debug/lib${ABI_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_LIST_DIR}/x64/Release/lib${ABI_DIR}")