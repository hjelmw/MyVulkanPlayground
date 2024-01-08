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
	float  m_Intensity;
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
	vec3  position  = texture(GBufferPositions, inUV).rgb;
	float metalness = texture(GBufferPositions, inUV).a;
	vec3  albedo    = texture(GBufferAlbedo,    inUV).rgb;
	float fresnel   = texture(GBufferAlbedo,    inUV).a;
	vec3  normal    = texture(GBufferNormals,   inUV).rgb;
	float roughness = texture(GBufferNormals,   inUV).a;
	float depth     = texture(GBufferDepth,     inUV).r;

	if(depth == 1.0f)
		discard;
	
	vec3 viewPos   = SDeferredLightingConstants.m_ViewPos;
	vec3 fragColor = vec3(0.0f, 0.0f ,0.0f);

	for(int i = 0; i < NUM_LIGHTS; i++)
	{
		vec3  lightPos       = SDeferredLightingConstants.m_Lights[i].m_Position;
		vec3  lightColor     = SDeferredLightingConstants.m_Lights[i].m_Color; 
		float lightRadius    = SDeferredLightingConstants.m_Lights[i].m_Radius;
		float lightIntensity = SDeferredLightingConstants.m_Lights[i].m_Intensity;

		// Vector from fragment towards light
		vec3 lightDir = lightPos - position;
		float distToLight = length(lightDir);

		float diffuse  = 0.0f;
		float specular = 0.0f;
		float ambient  = 0.0f; // 0.2f;
		{
			// Diffuse part
			diffuse = max(dot(normalize(normal), lightDir), 0.0f);

			// Specular part
			specular = 0.0;
			if(diffuse != 0.0f)
			{
				// Vector from fragment to camera
				vec3 viewDir = normalize(viewPos - position);

				vec3 halfwayDir  = normalize(viewDir + lightDir);
				specular = pow(max(dot(normalize(normal), halfwayDir), 0.0f), 16.0f);
			}

			// Inverse square law
			float attenuation = lightRadius / (distToLight * distToLight + 1.0);
			attenuation *= lightIntensity;

			fragColor += albedo * (diffuse * attenuation);
			fragColor += specular;
			fragColor *= lightColor;
		}


	}



	outFragcolor = vec4(fragColor, 1.0f);

}