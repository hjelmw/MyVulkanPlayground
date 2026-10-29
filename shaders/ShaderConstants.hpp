#ifndef SHADER_CONSTANTS_HPP
#define SHADER_CONSTANTS_HPP

#ifdef __cplusplus
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

using uint    = glm::uint32;
using vec2    = glm::vec2;
using vec3    = glm::vec3;
using vec4    = glm::vec4;
using mat3    = glm::mat3;
using mat4    = glm::mat4;

#define INIT_INTEGER = 0
#define INIT_FLOAT   = 0.0f
#define INIT_VEC2    = glm::vec2(0.0f, 0.0f)
#define INIT_VEC3    = glm::vec3(0.0f, 0.0f, 0.0f)
#define INIT_VEC4    = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
#define INIT_MAT4    = glm::identity<glm::mat4>()
#else
#define INIT_INTEGER
#define INIT_FLOAT
#define INIT_VEC2
#define INIT_VEC3
#define INIT_VEC4
#define INIT_MAT4
#endif

struct SGeometryVertexUniforms
{
	mat4 m_ModelMatrix      INIT_MAT4;
	mat4 m_ViewMatrix       INIT_MAT4;
	mat4 m_ProjectionMatrix INIT_MAT4;
};

struct SMaterialFragmentPushConstants
{
	vec4  m_Diffuse           INIT_VEC4;
	//
	float m_Shininess         INIT_FLOAT;
	float m_Metalness         INIT_FLOAT;
	float m_Fresnel           INIT_FLOAT;
	float m_Emission          INIT_FLOAT;
	//
	float m_Transparency      INIT_FLOAT;
	float m_Reflectivity      INIT_FLOAT;
	int   m_UseAlbedoTexture  INIT_INTEGER; // int and not bool, a C++ bool is 1 byte but a GLSL bool is 4
};

struct SLightData
{
	mat4  m_LightMatrix    INIT_MAT4;
	//
	vec3  m_LightPosition  INIT_VEC3;
	float m_LightRadius    INIT_FLOAT;
	//
	vec3  m_LightColor     INIT_VEC3;
	float m_LightIntensity INIT_FLOAT;
};

struct SDeferredLightingFragmentUniforms
{
	SLightData m_Lights[1];
	//
	vec3  m_ViewPos          INIT_VEC3;
	float m_ShadowDepthBias  INIT_FLOAT;
	//
	vec3  m_SunDirection     INIT_VEC3; // Towards the sun
	float m_SunIntensity     INIT_FLOAT;
	//
	vec3  m_SunColor         INIT_VEC3;
	float m_AmbientIntensity INIT_FLOAT;
};

struct STerrainVertexPushConstants
{
	mat4 m_ViewProjectionMatrix INIT_MAT4;
	mat4 m_ModelMatrix          INIT_MAT4;
};

struct SAtmosphericsVertexPushConstants
{
	mat4  m_InvViewProjectionMatrix INIT_MAT4;
	//
	float m_CameraFar               INIT_FLOAT;
};

struct SAtmosphericsFragmentUniforms
{
	vec3  m_PlanetCameraPosition     INIT_VEC3;
	float m_CameraNear               INIT_FLOAT;
	//
	vec3  m_PlanetCenter             INIT_VEC3;
	float m_CameraFar                INIT_FLOAT;
	//
	vec3     m_PlanetToSunDir        INIT_VEC3;
	int      m_NumInScatteringPoints INIT_INTEGER;
	//
	int      m_NumOpticalDepthPoints INIT_INTEGER;
	float    m_PlanetRadius          INIT_FLOAT;
	float    m_AtmosphereRadius      INIT_FLOAT;
	float    m_AbsorptionFallof      INIT_FLOAT;
	//
	vec3  m_AbsorptionBeta           INIT_VEC3;
	float m_AbsorptionHeight         INIT_FLOAT;
	//
	vec3  m_RayleighBetaScattering   INIT_VEC3;
	float m_RayleighHeight           INIT_FLOAT;
	//
	vec3  m_MieBetaScattering        INIT_VEC3;
	float m_MieHeight                INIT_FLOAT;
	//
	uint   m_AllowMieScattering      INIT_INTEGER;
	float  m_ScatteringIntensity     INIT_FLOAT;
	vec2   m_Pad0                    INIT_VEC2;
};

struct SDebugVertexUniforms
{
	mat4 m_ViewProjectionMatrix INIT_MAT4;
};


#undef INIT_INTEGER
#undef INIT_FLOAT
#undef INIT_VEC2
#undef INIT_VEC3
#undef INIT_VEC4
#undef INIT_MAT4
#endif // SHADER_CONSTANTS_HPP