cmake_minimum_required(VERSION 3.31)
project(Hello_Triangle)

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
include_directories(../ThirdParty/tiny_obj)
include_directories(../ThirdParty/FrustumCull)
include_directories(../ThirdParty/stb_image)
if(WIN32)
    include_directories(../ThirdParty/UltraLight/windows/include)
    include_directories(../ThirdParty/vulkan/Include)
    include_directories(../ThirdParty/GLFW/include)
elseif(UNIX AND NOT APPLE)
    include_directories(../ThirdParty/UltraLight/linux/include)
    find_package(Vulkan REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED glfw3)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    link_directories(../x64/Debug/lib)
    link_directories(../ThirdParty/vulkan/Lib)
    link_directories(../ThirdParty/GLFW/lib-vc2019)
    link_directories(../ThirdParty/UltraLight/windows/lib)
endif()

add_executable(Hello_Triangle ${SRC})

target_compile_definitions(
        Hello_Triangle
        PRIVATE
        $<$<BOOL:RESOURCE_TRACKING>:RESOURCE_TRACKING>
        $<$<BOOL:RESOURCE_DEBUG>:RESOURCE_DEBUG>
)

target_compile_definitions(Hello_Triangle PUBLIC WOLF_VULKAN)

if(WIN32)
    target_link_libraries(Hello_Triangle vulkan-1.lib)
    target_link_libraries(Hello_Triangle glfw3.lib)
    target_link_libraries(Hello_Triangle Ultralight.lib)
    target_link_libraries(Hello_Triangle UltralightCore.lib)
    target_link_libraries(Hello_Triangle WebCore.lib)
    target_link_libraries(Hello_Triangle AppCore.lib)
    target_link_libraries(Hello_Triangle Common.lib)
    target_link_libraries(Hello_Triangle GraphicAPIBroker.lib)
    target_link_libraries(Hello_Triangle WolfEngine.lib)
elseif(UNIX AND NOT APPLE)
    set(WOLF_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../x64/Debug/lib")
    set(ULTRALIGHT_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/UltraLight/linux/bin")

    target_link_libraries(Hello_Triangle PRIVATE
            Vulkan::Vulkan
            ${GLFW_LIBRARIES}
            Threads::Threads
            dl

            ${ULTRALIGHT_LIB_PATH}/libAppCore.so
            ${ULTRALIGHT_LIB_PATH}/libUltralight.so
            ${ULTRALIGHT_LIB_PATH}/libUltralightCore.so
            ${ULTRALIGHT_LIB_PATH}/libWebCore.so

            ${WOLF_LIB_PATH}/libWolfEngine.a
            ${WOLF_LIB_PATH}/libGraphicAPIBroker.a
            ${WOLF_LIB_PATH}/libCommon.a
    )
endif()

set_target_properties(Hello_Triangle
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../x64/${CMAKE_BUILD_TYPE}/exe"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/../x64/Debug/exe"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../x64/Release/exe")
