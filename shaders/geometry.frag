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
	vec3  m_Diffuse;
	float m_Reflectivity;
	//
	float m_Shininess;
	float m_Metalness;
	float m_Fresnel;
	float m_Emission;
	//
	float m_Transparency;
	vec3  m_Pad0;
} SMaterialConstants;


void main() 
{
	float metalness = SMaterialConstants.m_Metalness;
	float fresnel   = SMaterialConstants.m_Fresnel;
	float roughness = SMaterialConstants.m_Shininess; // Same thing

	outPosition = vec4(inWorldPosition, metalness);
	
	outNormal = vec4(normalize(inNormal), fresnel);

	//outAlbedo = texture(samplerColor, inUV); 
	outAlbedo = vec4(vec3(SMaterialConstants.m_Diffuse), roughness);
	
	gl_FragDepth = gl_FragCoord.z;
}