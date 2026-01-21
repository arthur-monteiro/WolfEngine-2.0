cmake_minimum_required(VERSION 3.31)
project(Compute_Pass)

set(CMAKE_CXX_STANDARD 23)

file(GLOB SRC
        "*.cpp"
)

# Includes Wolf libs
include_directories(../Common)
include_directories(../GraphicAPIBroker/Public)
include_directories("../Wolf-Engine-2.0")

# Includes third parties
include_directories(../ThirdParty/xxh64)
include_directories(../ThirdParty/glm)
include_directories(../ThirdParty/GLFW/include)
include_directories(../ThirdParty/tiny_obj)
include_directories(../ThirdParty/FrustumCull)
include_directories(../ThirdParty/stb_image)
include_directories(../ThirdParty/UltraLight/include)
include_directories(../ThirdParty/vulkan/Include)

link_directories(../x64/Debug/lib)
link_directories(../ThirdParty/vulkan/Lib)
link_directories(../ThirdParty/GLFW/lib-vc2019)
link_directories(../ThirdParty/UltraLight/lib)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(Compute_Pass ${SRC})

target_compile_definitions(
        Compute_Pass
        PRIVATE
        $<$<BOOL:RESOURCE_TRACKING>:RESOURCE_TRACKING>
        $<$<BOOL:RESOURCE_DEBUG>:RESOURCE_DEBUG>
)

target_compile_definitions(Compute_Pass PUBLIC WOLF_VULKAN)
target_link_libraries(Compute_Pass vulkan-1.lib)
target_link_libraries(Compute_Pass glfw3.lib)
target_link_libraries(Compute_Pass Ultralight.lib)
target_link_libraries(Compute_Pass UltralightCore.lib)
target_link_libraries(Compute_Pass WebCore.lib)
target_link_libraries(Compute_Pass AppCore.lib)
target_link_libraries(Compute_Pass Common.lib)
target_link_libraries(Compute_Pass GraphicAPIBroker.lib)
target_link_libraries(Compute_Pass WolfEngine.lib)

set_target_properties(Compute_Pass
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../x64/${CMAKE_BUILD_TYPE}/exe"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/../x64/Debug/exe"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../x64/Release/exe")
