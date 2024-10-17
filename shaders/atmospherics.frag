#version 450

layout (binding = 0) uniform sampler2D SceneColor;
layout (binding = 1) uniform sampler2D GBufferDepth;
layout (binding = 2) uniform UBO
{
	vec3  m_PlanetCameraPosition;
	float m_CameraNear;
	// 
	vec3  m_PlanetCenter;
	float m_CameraFar;
	//
	vec3 m_PlanetToSunDir;
	uint m_NumInScatteringPoints;
	//
	uint  m_NumOpticalDepthPoints;
	float m_PlanetRadius;
	float m_AtmosphereRadius;
	float m_DensityFallof;
	//
	vec3  m_ScatteringCoefficients;
	float m_Pad0;
} SAtmosphericsConstants;

layout (location = 0) in  vec2 inUV;
layout (location = 1) in  vec3 inCameraRayDir;
layout (location = 0) out vec4 outFragColor;

#define FLT_MAX 3.402823466e+38

vec2 raySphereIntersect(vec3 rayOrigin, vec3 rayDirection, vec3 sphereCenter, float sphereRadius)
{ 
	vec3 offset = rayOrigin - sphereCenter;
	float a = 1.0f;
	float b = 2.0f * dot(offset, rayDirection);
	float c = dot(offset, offset) - sphereRadius * sphereRadius;

	// Discriminant of quadratic formula
	float discriminant = b * b - 4.0f * a * c;

	if(discriminant > 0.0f)
	{
		float discriminant_sq = sqrt(discriminant);

		float distToSphereNear = max((-b - discriminant_sq) / (2.0f * a), 0.0f);
		float distToSphereFar  =     (-b + discriminant_sq) / (2.0f * a);

		if(distToSphereFar >= 0.0f)
			return vec2(distToSphereNear, distToSphereFar - distToSphereNear);
	}

	return vec2(FLT_MAX, 0.0f);
}

float CalculateDensityAtPoint(vec3 samplePoint)
{
	float heightAboveSurface = max(length(samplePoint - SAtmosphericsConstants.m_PlanetCenter) - SAtmosphericsConstants.m_PlanetRadius, 0.0f);

	// Scale to [0,1]
	float heightAboveSurface01 = heightAboveSurface / (SAtmosphericsConstants.m_AtmosphereRadius - SAtmosphericsConstants.m_PlanetRadius);

	// Last part is to make sure height = 1 gives density = 0
	float density = exp(-(heightAboveSurface01) * SAtmosphericsConstants.m_DensityFallof) * (1.0f - heightAboveSurface01);
	
	return density;
}

float CalculateOpticalDepth(vec3 rayOrigin, vec3 rayDirection, float rayLength)
{
	vec3 densitySamplePoint = rayOrigin;
	float stepSize = rayLength / (SAtmosphericsConstants.m_NumOpticalDepthPoints - 1);

	float opticalDepth = 0.0f;
	for(uint i = 0; i < SAtmosphericsConstants.m_NumOpticalDepthPoints; i++)
	{
		float density = CalculateDensityAtPoint(densitySamplePoint);

		opticalDepth += density * stepSize;
		densitySamplePoint += rayDirection * stepSize;
	}

	return opticalDepth; 
}

vec3 mie(float dist, vec3 sunL)
{
    return max(exp(-pow(dist, 0.25)) * sunL - 0.4, 0.0);
}

vec3 CalculateScatteredLight(vec3 rayOrigin, vec3 rayDirection, float rayLength, vec3 originalColor)
{
    vec3 inScatterPoint = rayOrigin;
	float stepSize = rayLength / (SAtmosphericsConstants.m_NumInScatteringPoints - 1);
	
	float viewRayOpticalDepth = 0.0f;

	vec3 inScatteredLight = vec3(0.0f, 0.0f, 0.0f);
	for(uint i = 0; i < SAtmosphericsConstants.m_NumInScatteringPoints; i++)
	{
		float sunRayLength = raySphereIntersect(inScatterPoint, -SAtmosphericsConstants.m_PlanetToSunDir, SAtmosphericsConstants.m_PlanetCenter, SAtmosphericsConstants.m_AtmosphereRadius).y;
		float sunRayOpticalDepth = CalculateOpticalDepth(inScatterPoint, -SAtmosphericsConstants.m_PlanetToSunDir, sunRayLength);
		viewRayOpticalDepth = CalculateOpticalDepth(inScatterPoint, -rayDirection, stepSize * i);

		vec3 transmittance = exp(-(sunRayOpticalDepth + viewRayOpticalDepth) * SAtmosphericsConstants.m_ScatteringCoefficients);

		float density = CalculateDensityAtPoint(inScatterPoint);

		inScatteredLight += density * transmittance * SAtmosphericsConstants.m_ScatteringCoefficients * stepSize;
		inScatterPoint += rayDirection * stepSize;
	}

	float originalColorTransmittance = exp(-viewRayOpticalDepth);
	return originalColor * originalColorTransmittance + inScatteredLight;
}


float LinearizeDepth(float d,float zNear,float zFar)
{
    float z_n = 2.0 * d - 1.0;
    return 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
}

void main()
{
	float depth = LinearizeDepth(texture(GBufferDepth, inUV).r, SAtmosphericsConstants.m_CameraNear, SAtmosphericsConstants.m_CameraFar);
	vec4  color = texture(SceneColor,   inUV);

	vec3  rayOrigin        = SAtmosphericsConstants.m_PlanetCameraPosition;
	vec3  rayDirection     = normalize(inCameraRayDir);
	vec3  sunDirection     = SAtmosphericsConstants.m_PlanetToSunDir;
	float atmosphereRadius = SAtmosphericsConstants.m_AtmosphereRadius;
	float planetRadius     = SAtmosphericsConstants.m_PlanetRadius;
	vec3  planetCenter     = SAtmosphericsConstants.m_PlanetCenter;

	vec2 hitInfo = raySphereIntersect(rayOrigin, rayDirection, planetCenter, atmosphereRadius);

	float distToAtmosphere      = hitInfo.x;
	float distThroughAtmosphere = min(hitInfo.y, depth - distToAtmosphere);

//	if(distThroughAtmosphere > 0.0f)
//	{
//		vec3 pointInAtmosphere = rayOrigin + rayDirection * (distToAtmosphere + 0.002f);
//		vec3 light = CalculateScatteredLight(pointInAtmosphere, rayDirection, (distThroughAtmosphere - 0.002f), color.rgb);
//		outFragColor = vec4(light, 0.0f);
//		return;
//	}
//
//	outFragColor = color;

	outFragColor = getSky(inUv);
	
	//outFragColor = vec4(distThroughAtmosphere / (atmosphereRadius * 2.0f));
}