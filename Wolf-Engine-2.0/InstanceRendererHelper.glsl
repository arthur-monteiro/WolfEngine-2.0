R"(

struct InstanceInfo
{
    mat4 transform;
    uint materialIdx;
    uint customData;
};

layout (std430, binding = 0, set = @DESCRIPTOR_SLOT) restrict buffer InstanceBufferLayout
{
    InstanceInfo instancesInfo[];
};

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

)"