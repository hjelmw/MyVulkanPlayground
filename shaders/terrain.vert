#version 450

layout(location = 0) in  vec3  inPosition;

layout(location = 0) out float fragHeight;
layout(location = 1) out vec3  fragColor;

layout( push_constant ) uniform constants
{
    mat4 m_ModelViewProjectionMatrix;

} STerrainVertexPushConstants;


void main() 
{
    vec4 position = STerrainVertexPushConstants.m_ModelViewProjectionMatrix * vec4(inPosition, 1.0);
    gl_Position  = vec4(position.x, -position.y, position.z, position.w);
    fragHeight   = inPosition.y;
    fragColor    = vec3(1.0f, 0.0f, 0.0f);
}