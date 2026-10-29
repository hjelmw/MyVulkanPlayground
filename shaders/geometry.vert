#version 450

#include "ShaderConstants.hpp"

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec3 inTangent;

layout (binding = 0) uniform Geometry{ SGeometryVertexUniforms SGeometryConstants; };

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outColor;
layout (location = 3) out vec3 outWorldPosition;
layout (location = 4) out vec3 outTangent;

void main()
{
    gl_Position  = SGeometryConstants.m_ProjectionMatrix * SGeometryConstants.m_ViewMatrix * SGeometryConstants.m_ModelMatrix * vec4(inPosition, 1.0);
	
	outUV = inTexCoord;

	// Vertex position in world space
	outWorldPosition = (SGeometryConstants.m_ModelMatrix * vec4(inPosition, 1.0)).xyz;
	
	// Normal in world space
	mat4 normalMatrix = transpose(inverse(SGeometryConstants.m_ModelMatrix));
	
	outNormal = (normalMatrix * vec4(inNormal, 1.0f)).xyz;
	

	// Currently just vertex color
	outColor = inColor;
}