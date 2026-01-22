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
    find_package(Threads REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED glfw3)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    pkg_check_modules(CAIRO REQUIRED cairo)
    pkg_check_modules(PANGO REQUIRED pango)
    pkg_check_modules(X11 REQUIRED x11)
    pkg_check_modules(BZIP2 REQUIRED bzip2)
    pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
    pkg_check_modules(FREETYPE REQUIRED freetype2)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
    link_directories(../x64/Debug/lib)
    link_directories(../ThirdParty/vulkan/Lib)
    link_directories(../ThirdParty/GLFW/lib-vc2019)
    link_directories(../ThirdParty/UltraLight/windows/lib)
endif()

add_executable(Compute_Pass ${SRC})

target_compile_definitions(
        Compute_Pass
        PRIVATE
        $<$<BOOL:RESOURCE_TRACKING>:RESOURCE_TRACKING>
        $<$<BOOL:RESOURCE_DEBUG>:RESOURCE_DEBUG>
)

target_compile_definitions(Compute_Pass PUBLIC WOLF_VULKAN)

if(WIN32)
    target_link_libraries(Compute_Pass vulkan-1.lib)
    target_link_libraries(Compute_Pass glfw3.lib)
    target_link_libraries(Compute_Pass Ultralight.lib)
    target_link_libraries(Compute_Pass UltralightCore.lib)
    target_link_libraries(Compute_Pass WebCore.lib)
    target_link_libraries(Compute_Pass AppCore.lib)
    target_link_libraries(Compute_Pass Common.lib)
    target_link_libraries(Compute_Pass GraphicAPIBroker.lib)
    target_link_libraries(Compute_Pass WolfEngine.lib)
elseif(UNIX AND NOT APPLE)
    set(WOLF_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../x64/Debug/lib")
    set(ULTRALIGHT_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/UltraLight/linux/bin")

    target_link_libraries(Compute_Pass PRIVATE
            ${WOLF_LIB_PATH}/libWolfEngine.a
            ${WOLF_LIB_PATH}/libGraphicAPIBroker.a
            ${WOLF_LIB_PATH}/libCommon.a

            ${ULTRALIGHT_LIB_PATH}/libAppCore.so
            ${ULTRALIGHT_LIB_PATH}/libUltralight.so
            ${ULTRALIGHT_LIB_PATH}/libUltralightCore.so
            ${ULTRALIGHT_LIB_PATH}/libWebCore.so

            # The system dependencies found by PkgConfig
            ${GTK3_LIBRARIES}
            ${CAIRO_LIBRARIES}
            ${PANGO_LIBRARIES}
            ${X11_LIBRARIES}
            ${BZIP2_LIBRARIES}
            ${FONTCONFIG_LIBRARIES}
            z

            Vulkan::Vulkan
            ${GLFW_LIBRARIES}
            Threads::Threads
            dl
    )
endif()

set_target_properties(Compute_Pass
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../x64/${CMAKE_BUILD_TYPE}/exe"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/../x64/Debug/exe"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../x64/Release/exe")
