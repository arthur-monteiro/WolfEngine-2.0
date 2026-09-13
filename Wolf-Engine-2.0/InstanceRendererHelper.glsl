R"(

struct InstanceInfo
{
    mat4 transform;
    uint materialIdx;
    uint customData;
    uint lod;
    uint instanceIdx;
};

layout (std430, binding = 0, set = @DESCRIPTOR_SLOT) restrict buffer InstanceBufferLayout
{
    InstanceInfo instancesInfo[];
};

#ifdef USE_MESHLET_HIERARCHY
struct CullingInstanceInfo
{
	mat4 transform;
	uint meshIdx;
	uint materialIdx;
	uint customData;
	uint batchesMask;
};
layout(std430, binding = 1, set = @DESCRIPTOR_SLOT) restrict buffer CullingInstancesBufferLayout
{
    CullingInstanceInfo cullingInstancesInfo[];
};

struct MeshInfo
{
    uint meshletBaseIndex;
    uint meshletCount;
    uint pad0;
    uint pad1;

    vec4 boundingSphere;
    vec4 aabbMin;
    vec4 aabbMax;
};
layout(std430, binding = 2, set = @DESCRIPTOR_SLOT) restrict buffer MeshesInfoBufferLayout
{
    MeshInfo meshesInfo[];
};

struct MeshletInfo
{
    uint vertexOffset;
    uint indexOffset;
    uint indexCount;
    float lodError;

    float parentLodError;
    uint coneAxisAndCutoff;
    uint vertexCount;
    uint pad2;

    vec4 boundingSphere;
    vec4 groupBoundingSphere;
    vec4 parentBoundingSphere;
};
layout(std430, binding = 3, set = @DESCRIPTOR_SLOT) restrict buffer MeshletsInfoBufferLayout
{
    MeshletInfo meshletsInfo[];
};

layout(binding = 4, set = @DESCRIPTOR_SLOT) uniform sampler2D HZBSampler;

// TODO: this should be provided by project / buffer
struct Vertex
{
    float px, py, pz;
    float nx, ny, nz;
    float tx, ty, tz;
    float u, v;
};
layout(std430, binding = 5, set = @DESCRIPTOR_SLOT) restrict buffer VerticesBufferLayout
{
    Vertex vertices[];
};

layout(std430, binding = 6, set = @DESCRIPTOR_SLOT) restrict buffer IndicesBufferLayout
{
    uint indices[];
};

uint getInstanceIdx(uint workgroupIdx)
{
    return instancesInfo[workgroupIdx].instanceIdx;
}
#else
mat4 getInstanceTransform()
{
	return instancesInfo[gl_DrawID].transform;
}

uint getMaterialIdx()
{
	return instancesInfo[gl_DrawID].materialIdx;
}

uint getCustomData()
{
	return instancesInfo[gl_DrawID].customData;
}

uint getLOD()
{
    return instancesInfo[gl_DrawID].lod;
}
#endif
)"