LOCAL_PATH := $(call my-dir)
ANDROID_NDK := 'F:\Android_studio_SDK\ndk\25.1.8937393'
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cc .cpp .cxx

LOCAL_MODULE := wolf-engine_vulkan

LOCAL_SRC_FILES := AndroidCacheHelper.cpp \
	CameraList.cpp \
	CommandRecordBase.cpp \
	DAEImporter.cpp \
	DescriptorSetGenerator.cpp \
	DescriptorSetLayoutGenerator.cpp \
	ImageCompression.cpp \
	ImageFileLoader.cpp \
	MaterialsGPUManager.cpp \
	Mesh.cpp \
	MipMapGenerator.cpp \
	ModelLoader.cpp \
	PipelineSet.cpp \
	RenderMeshList.cpp \
	ShaderList.cpp \
	ShaderParser.cpp \
	SubMesh.cpp \
	Timer.cpp \
	WolfEngine.cpp

LOCAL_CXXFLAGS:=-std=c++20 -fno-exceptions -fno-rtti -DENABLE_HLSL=1 -D_LIBCPP_NO_NATIVE_SEMAPHORES=1 -DVK_USE_PLATFORM_ANDROID_KHR=1 -DGLM_FORCE_CXX14=1 -DWOLF_VULKAN=1
LOCAL_EXPORT_CPPFLAGS:=-std=c++20
LOCAL_EXPORT_LDFLAGS:=-latomic
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../Common $(LOCAL_PATH)/../GraphicAPIBroker/Public $(LOCAL_PATH)/../ThirdParty/glm $(LOCAL_PATH)/../ThirdParty/stb_image $(LOCAL_PATH)/../ThirdParty/tiny_obj $(LOCAL_PATH)/../ThirdParty/xxh64 \
	${ANDROID_NDK}/sources/third_party/shaderc/libshaderc/include \
	${ANDROID_NDK}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/vulkan
include $(BUILD_STATIC_LIBRARY)