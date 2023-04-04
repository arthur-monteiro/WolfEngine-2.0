LOCAL_PATH := $(call my-dir)
ANDROID_NDK := 'F:\Android_studio_SDK\ndk\25.1.8937393'
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cc .cpp .cxx

LOCAL_MODULE := wolf-engine

LOCAL_SRC_FILES := Buffer.cpp \
	CommandBuffer.cpp \
	CommandPool.cpp \
	Configuration.cpp \
	DescriptorPool.cpp \
	Image.cpp \
	Fence.cpp \
	FrameBuffer.cpp \
	Pipeline.cpp \
	RayTracingSymbols.cpp \
	RenderPass.cpp \
	Semaphore.cpp \
	ShaderParser.cpp \
	SwapChain.cpp \
	Vulkan.cpp \
	VulkanHelper.cpp \
	WolfEngine.cpp

LOCAL_CXXFLAGS:=-std=c++20 -fno-exceptions -fno-rtti -DENABLE_HLSL=1 -D_LIBCPP_NO_NATIVE_SEMAPHORES=1 -DVK_USE_PLATFORM_ANDROID_KHR=1
LOCAL_EXPORT_CPPFLAGS:=-std=c++20
LOCAL_EXPORT_LDFLAGS:=-latomic
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../ThirdParty/glm $(LOCAL_PATH)/../ThirdParty/stb_image \
	${ANDROID_NDK}/sources/third_party/shaderc/libshaderc/include \
	${ANDROID_NDK}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/vulkan
include $(BUILD_STATIC_LIBRARY)