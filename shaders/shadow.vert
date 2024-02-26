#version 450

layout (location = 0) in vec3  inPosition;

layout (binding = 0) uniform DepthUniformBuffer 
{
	mat4 m_ModelMatrix;
	mat4 m_ViewMatrix;
	mat4 m_ProjectionMatrix;
} SShadowUBO;

void main()
{
	vec4 shadowPos =  SShadowUBO.m_ProjectionMatrix * SShadowUBO.m_ViewMatrix * SShadowUBO.m_ModelMatrix * vec4(inPosition, 1.0);
	gl_Position = shadowPos;
}