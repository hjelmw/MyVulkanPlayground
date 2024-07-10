#version 450

layout (binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inWorldPosition;
layout (location = 4) in vec3 inTangent;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;

//material push constants block
layout( push_constant ) uniform constants
{
	vec4  m_Diffuse;
	//
	float m_Shininess;
	float m_Metalness;
	float m_Fresnel;
	float m_Emission;
	//
	float m_Transparency;
	float m_Reflectivity;
	bool  m_UseAlbedoTexture;
} SMaterialConstants;


void main() 
{
	float metalness = SMaterialConstants.m_Metalness;
	float fresnel   = SMaterialConstants.m_Fresnel;
	float roughness = SMaterialConstants.m_Shininess;

	outPosition = vec4(inWorldPosition, metalness);
	
	outNormal = vec4(inNormal, fresnel);

	outAlbedo = vec4(SMaterialConstants.m_UseAlbedoTexture ? 
		texture(samplerColor, inUV).rgb : 
		SMaterialConstants.m_Diffuse.rgb, roughness);

	gl_FragDepth = gl_FragCoord.z;
}