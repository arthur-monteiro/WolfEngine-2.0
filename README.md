# Wolf-Engine 2.0

Yet another Vulkan engine.

This engine helps you to develop Vulkan projects.
You can find simple examples: Hello Triangle, Compute Pass, DescriptorSet, Hello Ray Tracing, HTML UI.

Working on Windows and Android.

Currently in development.

## Installation

### Windows (Visual Studio)

Create a new solution with your project and the Wolf Engine 2.0 project
Add Wolf Engine 2.0 includes and Third Parties
Add Wolf Engine 2.0 lib and Third Parties, add the folowing libs: "vulkan-1.lib; glfw3.lib; LibOVR.lib; UltralightCore.lib; Ultralight.lib; WebCore.lib; AppCore.lib; Engine Core.lib"

### Android

Run Wolf Engine 2.0/compile_android.bat, the compiled libs are under ndk-builds/
Add libs and includes to CMakeList.txt:
```
add_library(wolf_engine_lib STATIC IMPORTED)
set_target_properties(wolf_engine_lib PROPERTIES IMPORTED_LOCATION "PATH_TO_WOLF_ENGINE\\${ANDROID_ABI}\\libwolf-engine.a")

# add lib dependencies
target_link_libraries(${PROJECT_NAME} PUBLIC
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
        game-activity::game-activity_static
        android
        log)
        
target_include_directories(${PROJECT_NAME} PRIVATE "PATH_TO_WOLF_ENGINE\\Wolf Engine 2.0")
```
