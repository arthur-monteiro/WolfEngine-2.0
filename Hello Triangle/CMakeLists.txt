cmake_minimum_required(VERSION 3.31)
project(Hello_Triangle)

set(CMAKE_CXX_STANDARD 23)

file(GLOB SRC
        "*.cpp"
)

# Includes Wolf libs
include_directories(../Common)
include_directories(../GraphicAPIBroker/Public)
include_directories("../Wolf Engine 2.0")

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

add_executable(Hello_Triangle ${SRC})

target_compile_definitions(Hello_Triangle PUBLIC WOLF_VULKAN)
target_link_libraries(Hello_Triangle vulkan-1.lib)
target_link_libraries(Hello_Triangle glfw3.lib)
target_link_libraries(Hello_Triangle Ultralight.lib)
target_link_libraries(Hello_Triangle UltralightCore.lib)
target_link_libraries(Hello_Triangle WebCore.lib)
target_link_libraries(Hello_Triangle AppCore.lib)
target_link_libraries(Hello_Triangle Common.lib)
target_link_libraries(Hello_Triangle GraphicAPIBroker.lib)
target_link_libraries(Hello_Triangle WolfEngine.lib)

find_package("../Common/Common.cmake" NO_DEFAULT_PATH)
find_package("../GraphicAPIBroker/GraphicAPIBroker_Vulkan.cmake" NO_DEFAULT_PATH)
find_package("../Wolf Engine 2.0/WolfEngine.cmake" NO_DEFAULT_PATH)

set_target_properties(Hello_Triangle
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "../../x64/Debug/exe")
