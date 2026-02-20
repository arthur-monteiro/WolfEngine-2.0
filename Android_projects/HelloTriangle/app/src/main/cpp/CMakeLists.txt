cmake_minimum_required(VERSION 3.18.1)
project(hellotriangle)

# Include the GameActivity static lib to the project.
find_package(game-activity REQUIRED CONFIG)
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u \
    Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -Wall")

add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR=1)

add_library(${PROJECT_NAME} SHARED
        main.cpp
        UniquePass.cpp)

set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")

get_filename_component(SHADERC_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../ThirdParty/android/libshaderc
	ABSOLUTE)

add_library(shaderc_lib STATIC IMPORTED)
set_target_properties(shaderc_lib PROPERTIES IMPORTED_LOCATION
        ${SHADERC_SRC}/libs/${ANDROID_ABI}/libshaderc.a)

add_library(shaderc_util_lib STATIC IMPORTED)
set_target_properties(shaderc_util_lib PROPERTIES IMPORTED_LOCATION
        ${SHADERC_SRC}/libs/${ANDROID_ABI}/libshaderc_util.a)

add_library(glslang_lib STATIC IMPORTED)
set_target_properties(glslang_lib PROPERTIES IMPORTED_LOCATION
        ${SHADERC_SRC}/libs/${ANDROID_ABI}/libglslang.a)

add_library(spv_lib STATIC IMPORTED)
set_target_properties(spv_lib PROPERTIES IMPORTED_LOCATION
        ${SHADERC_SRC}/libs/${ANDROID_ABI}/libSPIRV.a)

add_library(spv_tools_lib STATIC IMPORTED)
set_target_properties(spv_tools_lib PROPERTIES IMPORTED_LOCATION
        ${SHADERC_SRC}/libs/${ANDROID_ABI}/libSPIRV-Tools.a)

add_library(spv_tools_opt_lib STATIC IMPORTED)
set_target_properties(spv_tools_opt_lib PROPERTIES IMPORTED_LOCATION
        ${SHADERC_SRC}/libs/${ANDROID_ABI}/libSPIRV-Tools-opt.a)

add_library(os_dependent_lib STATIC IMPORTED)
set_target_properties(os_dependent_lib PROPERTIES IMPORTED_LOCATION
        ${SHADERC_SRC}/libs/${ANDROID_ABI}/libOSDependent.a)

add_library(ogl_compiler_lib STATIC IMPORTED)
set_target_properties(ogl_compiler_lib PROPERTIES IMPORTED_LOCATION
        ${SHADERC_SRC}/libs/${ANDROID_ABI}/libOGLCompiler.a)

add_library(wolf_engine_lib STATIC IMPORTED)
set_target_properties(wolf_engine_lib PROPERTIES IMPORTED_LOCATION
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../x64/Debug/${ANDROID_ABI}/libWolfEngine.a)

add_library(common_lib STATIC IMPORTED)
set_target_properties(common_lib PROPERTIES IMPORTED_LOCATION
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../x64/Debug/${ANDROID_ABI}/libCommon.a)

add_library(graphic_api_broker_lib STATIC IMPORTED)
set_target_properties(graphic_api_broker_lib PROPERTIES IMPORTED_LOCATION
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../x64/Debug/${ANDROID_ABI}/libGraphicAPIBroker.a)

# add lib dependencies
target_link_libraries(${PROJECT_NAME} PUBLIC
        # The game activity
        game-activity::game-activity_static

        vulkan
        shaderc_lib
        shaderc_util_lib
        glslang_lib
        spv_lib
        spv_tools_lib
        spv_tools_opt_lib
        os_dependent_lib
        ogl_compiler_lib
        wolf_engine_lib
        common_lib
        graphic_api_broker_lib
        android
        log)

target_compile_definitions(${PROJECT_NAME} PUBLIC WOLF_VULKAN)

target_include_directories(${PROJECT_NAME} PRIVATE "../../../../../../ThirdParty/glm")
target_include_directories(${PROJECT_NAME} PRIVATE ${ANDROID_NDK}/sources/third_party/shaderc/libshaderc/include)
target_include_directories(${PROJECT_NAME} PRIVATE "../../../../../../Common")
target_include_directories(${PROJECT_NAME} PRIVATE "../../../../../../GraphicAPIBroker/Public")
target_include_directories(${PROJECT_NAME} PRIVATE "../../../../../../Wolf-Engine-2.0")
