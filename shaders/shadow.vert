#version 450

layout (location = 0) in vec3 inPosition;
layout (binding = 0) uniform DepthUniformBuffer 
{
    vec3 m_LightDirection;
    float m_Pad0;
    mat4 m_DepthModelViewProjectionMatrix;
} SShadowUBO;

void main()
{
	vec4 shadowPos = SShadowUBO.m_DepthModelViewProjectionMatrix * vec4(inPosition, 1.0);
    
	gl_Position = shadowPos;
}