#version 450
#include "ShaderConstants.hpp"

layout (location = 0) in vec3  inPosition;

layout (binding = 0) uniform Shadow { SGeometryVertexUniforms SShadowVertexConstants; };

void main()
{
	const mat4 shadowModelViewProjectionMatrix = SShadowVertexConstants.m_ProjectionMatrix * SShadowVertexConstants.m_ViewMatrix * SShadowVertexConstants.m_ModelMatrix;
	const vec4 shadowPos = shadowModelViewProjectionMatrix * vec4(inPosition, 1.0);
	gl_Position = shadowPos;
}