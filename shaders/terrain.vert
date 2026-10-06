#version 450

layout(location = 0) in  vec3  inPosition;
layout(location = 1) in  vec3  inNormal;

layout(location = 0) out vec3  outWorldPosition;
layout(location = 1) out vec3  outNormal;

layout( push_constant ) uniform constants
{
    mat4 m_ModelViewProjectionMatrix;

} STerrainVertexPushConstants;


void main()
{
    gl_Position      = STerrainVertexPushConstants.m_ModelViewProjectionMatrix * vec4(inPosition, 1.0);
    outWorldPosition = inPosition;
    outNormal        = inNormal;
}