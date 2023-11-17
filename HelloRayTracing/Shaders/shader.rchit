#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) rayPayloadInEXT vec3 hitValue;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 2, set = 0) buffer Vertices { vec4 v[]; } vertices;
layout(binding = 3, set = 0) buffer Indices { uint i[]; } indices;

void main()
{
  hitValue = vec3(0.0, 0.0, 1.0);
}
