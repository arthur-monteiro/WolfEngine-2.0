cmake_minimum_required(VERSION 3.31)
project(Variable_Rate_Shading)

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

add_executable(Variable_Rate_Shading ${SRC})

target_compile_definitions(Variable_Rate_Shading PUBLIC WOLF_VULKAN)
target_link_libraries(Variable_Rate_Shading vulkan-1.lib)
target_link_libraries(Variable_Rate_Shading glfw3.lib)
target_link_libraries(Variable_Rate_Shading Ultralight.lib)
target_link_libraries(Variable_Rate_Shading UltralightCore.lib)
target_link_libraries(Variable_Rate_Shading WebCore.lib)
target_link_libraries(Variable_Rate_Shading AppCore.lib)
target_link_libraries(Variable_Rate_Shading Common.lib)
target_link_libraries(Variable_Rate_Shading GraphicAPIBroker.lib)
target_link_libraries(Variable_Rate_Shading WolfEngine.lib)

set_target_properties(Variable_Rate_Shading
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../x64/${CMAKE_BUILD_TYPE}/exe"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/../x64/Debug/exe"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../x64/Release/exe")
