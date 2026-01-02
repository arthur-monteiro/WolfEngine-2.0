LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cc .cpp .cxx

LOCAL_MODULE := wolf-engine_vulkan

LOCAL_SRC_FILES := AndroidCacheHelper.cpp \
	BoundingSphere.cpp \
	CameraList.cpp \
	CommandRecordBase.cpp \
	ConfigurationHelper.cpp \
	DescriptorSetGenerator.cpp \
	DescriptorSetLayoutGenerator.cpp \
	ImageCompression.cpp \
	ImageFileLoader.cpp \
	JSONReader.cpp \
	LightManager.cpp \
	MaterialsGPUManager.cpp \
	Mesh.cpp \
	MipMapGenerator.cpp \
	MultiThreadTaskManager.cpp \
	PipelineSet.cpp \
	PushDataToGPU.cpp \
	ReadbackDataFromGPU.cpp \
	RenderMeshList.cpp \
	ShaderList.cpp \
	ShaderParser.cpp \
	SubMesh.cpp \
	Timer.cpp \
	VirtualTextureManager.cpp \
	WolfEngine.cpp

LOCAL_CXXFLAGS:=-std=c++20 -fexceptions -fno-rtti -DENABLE_HLSL=1 -D_LIBCPP_NO_NATIVE_SEMAPHORES=1 -DVK_USE_PLATFORM_ANDROID_KHR=1 -DGLM_FORCE_CXX14=1 -DWOLF_VULKAN=1
LOCAL_EXPORT_CPPFLAGS:=-std=c++20
LOCAL_EXPORT_LDFLAGS:=-latomic
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../Common $(LOCAL_PATH)/../GraphicAPIBroker/Public $(LOCAL_PATH)/../ThirdParty/glm $(LOCAL_PATH)/../ThirdParty/stb_image $(LOCAL_PATH)/../ThirdParty/tiny_obj $(LOCAL_PATH)/../ThirdParty/xxh64 \
	${NDK_PROJECT_PATH}/sources/third_party/shaderc/libshaderc/include \
	${NDK_PROJECT_PATH}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/vulkan
include $(BUILD_STATIC_LIBRARY)