LOCAL_PATH := $(call my-dir)
ANDROID_NDK := 'F:\Android_studio_SDK\ndk\25.1.8937393'
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cc .cpp .cxx

LOCAL_MODULE := common

LOCAL_SRC_FILES := Configuration.cpp \
	Debug.cpp \
	ResourceNonOwner.cpp \
	ResourceUniqueOwner.cpp \
	RuntimeContext.cpp

LOCAL_CXXFLAGS:=-std=c++20 -fno-exceptions -fno-rtti -DENABLE_HLSL=1 -D_LIBCPP_NO_NATIVE_SEMAPHORES=1 -DVK_USE_PLATFORM_ANDROID_KHR=1 -DGLM_FORCE_CXX14=1 -DWOLF_USE_VULKAN=1
LOCAL_EXPORT_CPPFLAGS:=-std=c++20
LOCAL_EXPORT_LDFLAGS:=-latomic
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../ThirdParty/glm $(LOCAL_PATH)/../ThirdParty/xxh64
include $(BUILD_STATIC_LIBRARY)