cmake_minimum_required(VERSION 3.31)
project(GenerateCodeFileHashes)

set(CMAKE_CXX_STANDARD 23)

file(GLOB SRC
        "*.cpp"
)

include_directories(../ThirdParty/xxh64)

add_executable(GenerateCodeFileHashes ${SRC})

set_target_properties(GenerateCodeFileHashes
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../x64/${CMAKE_BUILD_TYPE}/exe"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/../x64/Debug/exe"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../x64/Release/exe")