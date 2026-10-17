#version 450

layout(location = 0) in  vec3  inPosition;
layout(location = 1) in  vec3  inNormal;

layout(location = 0) out vec3  outWorldPosition;
layout(location = 1) out vec3  outNormal;
layout(location = 2) out float outHeight;

layout( push_constant ) uniform constants
{
    mat4 m_ViewProjectionMatrix;
    mat4 m_ModelMatrix;

} STerrainVertexPushConstants;


void main()
{
    mat4 modelMatrix   = STerrainVertexPushConstants.m_ModelMatrix;
    vec4 worldPosition = modelMatrix * vec4(inPosition, 1.0);

    gl_Position      = STerrainVertexPushConstants.m_ViewProjectionMatrix * worldPosition;
    outWorldPosition = worldPosition.xyz;
    outNormal        = transpose(inverse(mat3(modelMatrix))) * inNormal;
    outHeight        = inPosition.y; // Unscaled heightmap height for the albedo ramp
}