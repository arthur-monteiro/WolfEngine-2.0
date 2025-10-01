R"(
struct PointLightInfo
{
    vec4 lightPos;
    vec4 lightColor;
};
const uint MAX_POINT_LIGHTS = 16;

struct SunLightInfo
{
	vec4 sunDirection;
	vec4 sunColor;
};
const uint MAX_SUN_LIGHTS = 1;

layout(binding = 0, set = @LIGHT_INFO_DESCRIPTOR_SLOT, std140) uniform readonly UniformBufferLights
{
    PointLightInfo pointLights[MAX_POINT_LIGHTS];
    uint pointLightsCount;

    SunLightInfo sunLights[MAX_SUN_LIGHTS];
    uint sunLightsCount;
} ubLights;

layout (binding = 1, set = @LIGHT_INFO_DESCRIPTOR_SLOT) uniform samplerCube CubeMap;
)"