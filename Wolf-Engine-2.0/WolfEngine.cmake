file(GLOB SRC
        "*.cpp"
        "../ThirdParty/Tracy/TracyClient.cpp"
)

# Includes Wolf libs
include_directories(../Common)
include_directories(../GraphicAPIBroker/Public)

# Includes third parties
include_directories(../ThirdParty/xxh64)
include_directories(../ThirdParty/glm)
include_directories(../ThirdParty/GLFW/include)
include_directories(../ThirdParty/tiny_obj)
include_directories(../ThirdParty/FrustumCull)
include_directories(../ThirdParty/stb_image)
include_directories(../ThirdParty/UltraLight/include)
include_directories(../ThirdParty/vulkan/Include)
include_directories(../ThirdParty/Tracy/tracy)

add_library(WolfEngine STATIC ${SRC})