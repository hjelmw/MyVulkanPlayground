#version 450
#define NUM_LIGHTS 1

#include "ShaderConstants.hpp"

// Texture sampler G-Buffer
layout (binding = 0) uniform sampler2D GBufferPositions;
layout (binding = 1) uniform sampler2D GBufferNormals;
layout (binding = 2) uniform sampler2D GBufferAlbedo;
layout (binding = 3) uniform sampler2D GBufferDepth;
layout (binding = 4) uniform sampler2D ShadowMapBuffer;
layout (binding = 5) uniform sampler2D AtmosphericsBuffer;

layout (binding = 6) uniform Lighting{ SDeferredLightingFragmentUniforms SDeferredLightingConstants; };

layout (location = 0) in  vec2 inUV;
layout (location = 0) out vec4 outFragColor;

float CalculateShadow(vec4 fragPosLightSpace)
{
	// Perspective divide to get normalized device coordinates [-1,1]
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	
	// NDC [-1,1] to UV space [0, 1] 
	vec2 shadowMapUV = projCoords.xy * vec2(0.5f, 0.5f) + vec2(0.5f, 0.5f);
	
	float currentDepth   = clamp(projCoords.z, 0.0f, 1.0f);
	float shadowMapDepth = texture(ShadowMapBuffer, shadowMapUV).r;

	const float bias = SDeferredLightingConstants.m_ShadowDepthBias;
	float shadow = ( currentDepth - bias ) < shadowMapDepth ? 1.0f : 0.0f;
	
	return shadow;
}

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

	// Early out for skybox
	if(depth == 1.0f)
	{
		vec4 atmosphericsColor = texture(AtmosphericsBuffer, inUV);
		outFragColor = vec4(atmosphericsColor.rgb, 1.0f);
		return;
	}

	vec3 viewPos   = SDeferredLightingConstants.m_ViewPos;
	vec3 fragColor = vec3(0.0f, 0.0f ,0.0f);

	// The shadow map is rendered from the sun, see CShadowNode
	float shadow = 0.0f;
	{
		mat4 lightMatrix = SDeferredLightingConstants.m_Lights[0].m_LightMatrix;

		// Move world space fragment to light view space
		vec4 fragPosLightSpace = lightMatrix * vec4(position, 1.0f);

		shadow = CalculateShadow(fragPosLightSpace);
	}

	// Sun is a directional light so there is no distance falloff
	vec3 directLight = vec3(0.0f, 0.0f, 0.0f);
	{
		vec3  sunDir   = normalize(SDeferredLightingConstants.m_SunDirection);
		float sunNdotL = max(dot(normalize(normal), sunDir), 0.0f);

		directLight += SDeferredLightingConstants.m_SunColor * SDeferredLightingConstants.m_SunIntensity * albedo * sunNdotL;
	}

	for(int i = 0; i < NUM_LIGHTS; i++)
	{
		vec3  lightPos       = SDeferredLightingConstants.m_Lights[i].m_LightPosition;
		vec3  lightColor     = SDeferredLightingConstants.m_Lights[i].m_LightColor; 
		float lightRadius    = SDeferredLightingConstants.m_Lights[i].m_LightRadius;
		float lightIntensity = SDeferredLightingConstants.m_Lights[i].m_LightIntensity;

		// Vector from fragment towards light
		vec3 lightDir = lightPos - position;
		float distToLight = length(lightDir);

		float specular = 0.0f;
		vec3  diffuse  = vec3(0.0f, 0.0f, 0.0f);
		{
			// Inverse square law
			float attenuation = lightRadius / (distToLight * distToLight + 1.0f);

			// Diffuse part. Clamped so surfaces facing away don't subtract light
			{
				float NdotL = max(dot(normalize(normal), lightDir), 0.0f);
				diffuse = lightColor * albedo * NdotL * attenuation;
			}

			// Specular part
			{
				// Vector from fragment to camera
				vec3 viewDir = normalize(viewPos - position);

				vec3 halfwayDir  = normalize(viewDir + lightDir);
				specular = pow(max(dot(normalize(normal), halfwayDir), 0.0f), 32.0f) * attenuation;
			}

			directLight += diffuse + specular;
		}


	}

	// Ambient keeps shadowed areas from going completely black
	vec3 ambient = SDeferredLightingConstants.m_AmbientIntensity * albedo;

	fragColor = ambient + directLight * shadow;

	outFragColor = vec4(fragColor, 1.0f);

}