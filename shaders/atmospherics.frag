#version 450

layout (binding = 0) uniform sampler2D GBufferDepth;
layout (binding = 1) uniform UBO
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
	float m_AbsorptionFallof;
	//
	vec3 m_AbsorptionBeta;
	float m_AbsorptionHeight;
	//
	vec3 m_RayleighBetaScattering;
	float m_RayleighHeight;
	//
	vec3 m_MieBetaScattering;
	float m_MieHeight;
	//
	uint  m_AllowMieScattering;
	float m_ScatteringIntensity;
	vec2  m_Pad0;
} SAtmosphericsConstants;

layout (location = 0) in  vec2 inUV;
layout (location = 1) in  vec3 inCameraRayDir;
layout (location = 0) out vec4 outFragColor;

#define FLT_MAX 3.402823466e+38
#define M_PI 3.1415926535897932384626433832795

vec2 RaySphereIntersect(vec3 rayOrigin, vec3 rayDirection, vec3 sphereCenter, float sphereRadius)
{ 
	vec3 offset = rayOrigin - sphereCenter;
	float a = dot(rayDirection, rayDirection);
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

vec2 CalculateRayleighMiePhase(vec3 rayDirection, vec3 sunDirection)
{
	const float mu = dot(rayDirection, sunDirection);
	const float mumu = mu * mu;
	const float g = 0.7f;
	const float gg = g * g;

	const float phaseRayleigh = 3.0f / (16.0f * M_PI) * (1.0 + mumu); // Rayleigh phase function
	const float phaseMie = 3.0f / (8.0f * M_PI) * ((1.0f - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg)); // Mie phase function

	return vec2(phaseRayleigh, phaseMie);
 }

vec3 CalculateDensityAtPoint(vec3 samplePoint)
{
	const vec2 scaleHeight = vec2(SAtmosphericsConstants.m_RayleighHeight, SAtmosphericsConstants.m_MieHeight);
	const float heightAboveSurface = max(length(samplePoint) - SAtmosphericsConstants.m_PlanetRadius, 0.0f);
	const vec2  particleDensity = vec2(exp(-heightAboveSurface / scaleHeight));

	const float absorptionDenominator = (SAtmosphericsConstants.m_AbsorptionHeight - heightAboveSurface) / SAtmosphericsConstants.m_AbsorptionFallof;
	const float absorptionDensity = (1.0f / (absorptionDenominator * absorptionDenominator + 1.0f)) * particleDensity.x;

	return vec3(particleDensity.xy, absorptionDensity);
}

vec3 CalculateOpticalDepth(vec3 rayOrigin, vec3 rayDirection, float rayLength)
{
	vec3 densitySamplePoint = rayOrigin;
	const float stepSize = rayLength / SAtmosphericsConstants.m_NumOpticalDepthPoints;

	vec3 opticalDepth = vec3(0.0f, 0.0f, 0.0f);
	for(uint i = 0; i < SAtmosphericsConstants.m_NumOpticalDepthPoints; i++)
	{
		vec3 density = CalculateDensityAtPoint(densitySamplePoint);

		opticalDepth += density * stepSize;
		densitySamplePoint += rayDirection * stepSize;
	}

	return opticalDepth; 
}

vec4 CalculateScatteredLight(vec3 rayOrigin, vec3 rayDirection)
{
	vec3 inscatterStartPoint = rayOrigin - SAtmosphericsConstants.m_PlanetCenter;

	// Shoot a ray to figure out if it collides or is inside of the atmosphere
	const vec2 rayAtmosphereHit = RaySphereIntersect(inscatterStartPoint, rayDirection, SAtmosphericsConstants.m_PlanetCenter, SAtmosphericsConstants.m_AtmosphereRadius);
	const vec2 rayPlanetHit     = RaySphereIntersect(inscatterStartPoint, rayDirection, SAtmosphericsConstants.m_PlanetCenter, SAtmosphericsConstants.m_PlanetRadius);

	const float distToAtmosphere = rayAtmosphereHit.x;
	const float distThroughAtmosphere = min(rayAtmosphereHit.y, rayPlanetHit.x);

	// Early out if ray did not hit anything
	if(distThroughAtmosphere == 0.0f || distToAtmosphere > distThroughAtmosphere)
		return vec4(0.0f, 0.0f, 0.0f, 0.0f);

	const float stepSize = (distThroughAtmosphere - distToAtmosphere) / float(SAtmosphericsConstants.m_NumInScatteringPoints);

	// Raymarch start
	const float rayStepDistance = distToAtmosphere + stepSize * 0.5;
	vec3 inScatterPoint = inscatterStartPoint + rayDirection * rayStepDistance;

	const vec3 rayleighBeta = SAtmosphericsConstants.m_RayleighBetaScattering;
	const vec3 mieBeta = SAtmosphericsConstants.m_MieBetaScattering;
	const vec3 absorptionBeta = SAtmosphericsConstants.m_AbsorptionBeta;

	// These will contain the amount of scattered light and optical depth
	vec3 rayleighDensity = vec3(0.0f, 0.0f, 0.0f);
	vec3 mieDensity = vec3(0.0f, 0.0f, 0.0f);
	vec3 totalOpticalDepth = vec3(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < SAtmosphericsConstants.m_NumInScatteringPoints; i++)
	{
		vec3 density = CalculateDensityAtPoint(inScatterPoint);

		// Now shoot a ray from current position toward sun. We will use this to compute the optical depth
		vec2 raySunHit = RaySphereIntersect(inScatterPoint, SAtmosphericsConstants.m_PlanetToSunDir, SAtmosphericsConstants.m_PlanetCenter, SAtmosphericsConstants.m_AtmosphereRadius);

		// Now raymarch toward the sun and gather the optical depth
		vec3 currentOpticalDepth = CalculateOpticalDepth(inScatterPoint, SAtmosphericsConstants.m_PlanetToSunDir, raySunHit.y);

		// The amount of light that reached the sample point
		vec3 rayleighAttenuation   = -rayleighBeta   * (totalOpticalDepth.x + currentOpticalDepth.x);
		vec3 mieAttenuation        =  mieBeta        * (totalOpticalDepth.y + currentOpticalDepth.y);
		vec3 absorptionAttenuation =  absorptionBeta * (totalOpticalDepth.z + currentOpticalDepth.z);

		vec3 attenuation = exp(rayleighAttenuation - mieAttenuation - absorptionAttenuation);

		// Add this density to the optical depth so that we know how many particles intersect it
		totalOpticalDepth += density * stepSize;

		rayleighDensity += density.x * attenuation * stepSize;
		mieDensity += density.y * attenuation * stepSize;

		inScatterPoint += rayDirection * stepSize;
	}

	// calculate how much light can pass through the atmosphere
	vec3 opacity = exp(-(mieBeta * totalOpticalDepth.y + rayleighBeta * totalOpticalDepth.x + absorptionBeta * totalOpticalDepth.z));

	vec2 rayleighMiePhase = CalculateRayleighMiePhase(rayDirection, SAtmosphericsConstants.m_PlanetToSunDir);
	const float rayleighPhase = rayleighMiePhase.x;
	const float miePhase = rayleighMiePhase.y;

	vec3 rayleighTerm = rayleighPhase * rayleighBeta * rayleighDensity;
	vec3 mieTerm = miePhase * mieBeta * mieDensity;

	vec4 finalInscatteredLight = vec4((rayleighTerm + mieTerm) * SAtmosphericsConstants.m_ScatteringIntensity, opacity);

	return 1.0f - exp(-finalInscatteredLight);
}

void main()
{
	float depth = texture(GBufferDepth, inUV).r;

	if(depth != 1.0f)
	{
		outFragColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);
		return;
	}

	vec3  rayOrigin = SAtmosphericsConstants.m_PlanetCameraPosition;
	vec3  rayDirection = normalize(inCameraRayDir);
	vec4 light = CalculateScatteredLight(rayOrigin, rayDirection);

	outFragColor = light;
}