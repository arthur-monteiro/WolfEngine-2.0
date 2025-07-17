file(GLOB SRC
        "Public/*.cpp"
        "Private/Vulkan/*.cpp"
)

# Includes Wolf libs
include_directories(../Common)

# Includes third parties
include_directories(../ThirdParty/xxh64)
include_directories(../ThirdParty/glm)
include_directories(../ThirdParty/GLFW/include)
include_directories(../ThirdParty/vulkan/Include)