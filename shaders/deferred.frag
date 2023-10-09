#version 450
#define NUM_LIGHTS 1

// Texture sampler G-Buffer
layout (binding = 0) uniform sampler2D GBufferPositions;
layout (binding = 1) uniform sampler2D GBufferNormals;
layout (binding = 2) uniform sampler2D GBufferAlbedo;
layout (binding = 3) uniform sampler2D GBufferDepth;

// Per Light data
struct SLight
{
	vec3   m_Position;
	float  m_Radius;
	vec3   m_Color;
	float  m_Pad0;
};

// Deferred lighting uniform buffer constants
layout (binding = 4) uniform UBO
{
	SLight m_Lights[NUM_LIGHTS];
	vec3   m_ViewPos;
	float  m_Pad1;
} SDeferredLightingConstants;

layout (location = 0) in  vec2 inUV;
layout (location = 0) out vec4 outFragcolor;

void main()
{
	// Get G-Buffer values
	vec3 position = texture(GBufferPositions, inUV).rgb;
	vec3 albedo   = texture(GBufferAlbedo,    inUV).rgb;
	vec3 normal   = texture(GBufferNormals,   inUV).rgb;
	float depth   = texture(GBufferDepth,     inUV).r;
	
	vec3 viewPos   = SDeferredLightingConstants.m_ViewPos;
	vec3 fragColor = vec3(0.0f, 0.0f ,0.0f);
	
	for(int i = 0; i < NUM_LIGHTS; i++)
	{
		vec3  lightPos    = SDeferredLightingConstants.m_Lights[i].m_Position;
		vec3  lightColor  = SDeferredLightingConstants.m_Lights[i].m_Color; 
		float lightRadius = SDeferredLightingConstants.m_Lights[i].m_Radius;

		// Vector from fragment towards light
		vec3 L = lightPos - position;

		// Vector from fragment to camera
		vec3 V = normalize(viewPos - position);

		float distToLight = length(L);

		if(abs(distToLight) <= lightRadius)
		{
			fragColor = albedo * dot(V,normal);
		}
		else
		{
			// Inverse square law
			float attenuation = lightRadius / (pow(distToLight, 2.0) + 1.0);

			// Diffuse part
			vec3 N = normalize(normal);
			float NdotL = max(0.0f, dot(N, L));
			vec3 diffuse = lightColor * albedo * NdotL * attenuation;

			// Specular part
			vec3 R = reflect(-L, N);
			float NdotR = max(0.0f, dot(R, V));
			vec3 specular = lightColor * pow(NdotR, 16.0f) * attenuation;

			fragColor += diffuse;// + specular;
		}


	}

	outFragcolor = vec4(fragColor, 1.0f);

}