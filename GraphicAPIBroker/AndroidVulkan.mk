LOCAL_PATH := $(call my-dir)
ANDROID_NDK := 'F:\Android_studio_SDK\ndk\25.1.8937393'
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cc .cpp .cxx

LOCAL_MODULE := graphic_api_broker

LOCAL_SRC_FILES := Private/Vulkan/AccelerationStructureVulkan.cpp \
	Private/Vulkan/BottomLevelAccelerationStructureVulkan.cpp \
	Private/Vulkan/BufferVulkan.cpp \
	Private/Vulkan/CommandBufferVulkan.cpp \
	Private/Vulkan/CommandPool.cpp \
	Private/Vulkan/DescriptorPool.cpp \
	Private/Vulkan/DescriptorSetLayoutVulkan.cpp

LOCAL_CXXFLAGS:=-std=c++20 -fno-exceptions -fno-rtti -DENABLE_HLSL=1 -D_LIBCPP_NO_NATIVE_SEMAPHORES=1 -DVK_USE_PLATFORM_ANDROID_KHR=1 -DGLM_FORCE_CXX14=1 -DWOLF_USE_VULKAN=1
LOCAL_EXPORT_CPPFLAGS:=-std=c++20
LOCAL_EXPORT_LDFLAGS:=-latomic
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../Common $(LOCAL_PATH)/../ThirdParty/glm $(LOCAL_PATH)/../ThirdParty/xxh64 \
	${ANDROID_NDK}/sources/third_party/shaderc/libshaderc/include \
	${ANDROID_NDK}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/vulkan
include $(BUILD_STATIC_LIBRARY)