#version 450

#include "ShaderConstants.hpp"

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outCameraRayDir;

layout( push_constant ) uniform constants{ SAtmosphericsVertexPushConstants SAtmosphericsVertexConstants; };

void main() 
{
	outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);

	vec4 position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
	gl_Position = position;

	mat4 invViewProj = SAtmosphericsVertexConstants.m_InvViewProjectionMatrix;
	
	float cameraFar  = SAtmosphericsVertexConstants.m_CameraFar;

	vec4 rayDir = invViewProj * position;
	rayDir.xyz  = (rayDir.xyz / rayDir.w) * (1.0f / cameraFar);
	
	outCameraRayDir = rayDir.xyz;
}