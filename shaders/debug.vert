#version 450

#include "ShaderConstants.hpp"

layout (binding = 0) uniform DebugLines { SDebugVertexUniforms SDebugVertexConstants; };

layout (location = 0) in vec3 inWorldPosition;
layout (location = 1) in vec3 inColor;

layout (location = 1) out vec4 outColor;

void main()
{
	gl_Position = SDebugVertexConstants.m_ViewProjectionMatrix * vec4(inWorldPosition, 1.0);
	outColor = vec4(inColor, 1.0f);
}