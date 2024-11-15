R"(
layout(binding = 0, set = �CAMERA_DESCRIPTOR_SLOT) uniform UniformBufferCamera
{
	mat4 view;

	mat4 projection;

	mat4 invView;

	mat4 invProjection;

	mat4 previousViewMatrix;

	vec2 jitter;
	vec2 projectionParams;

	float near;
	float far;
	
} ubCamera;

mat4 getViewMatrix()
{
	return ubCamera.view;
}

mat4 getProjectionMatrix()
{
	return ubCamera.projection;
}

mat4 getInvViewMatrix()
{
	return ubCamera.invView;
}

mat4 getInvProjectionMatrix()
{
	return ubCamera.invProjection;
}

vec2 getProjectionParams()
{
	return ubCamera.projectionParams;
}

vec2 getCameraJitter()
{
	return ubCamera.jitter;
}

float linearizeDepth(float d)
{
    return ubCamera.near * ubCamera.far / (ubCamera.far - d * (ubCamera.far - ubCamera.near));
}

mat4 getPreviousViewMatrix()
{
	return ubCamera.previousViewMatrix;
}

vec3 computeWorldPosFromViewPos(in const vec3 viewPos)
{
	return (getInvViewMatrix() * vec4(viewPos, 1.0)).xyz;
}

vec3 getCameraPos()
{
	return ubCamera.invView[3].xyz;
}
)"