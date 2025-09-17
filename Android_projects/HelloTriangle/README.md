# Android Hello Triangle 

This is a simple test for the Wolf Engine on Android. It simply draws a triangle.

## Installation

- Open the project in Android Studio and download the NDK 25.1.8937393.
- Go to `$(ANDROID_NDK)/sources/third_party/shaderc` and run `ndk-build APP_PLATFORM=33 TARGET_OUT=libs/$(TARGET_ARCH_ABI) APP_ABI=all APP_BUILD_SCRIPT=Android.mk -B`
- In the Wolf Engine, run "build_android_all.bat"