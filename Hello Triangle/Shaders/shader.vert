#version 450

layout(location = 0) in vec2 inPosition;
 
out gl_PerVertex
{
    vec4 gl_Position;
};

void main() 
{
    gl_Position = vec4(inPosition.x, inPosition.y, 0.0, 1.0);
} 
