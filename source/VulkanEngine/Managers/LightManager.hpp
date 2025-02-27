#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "GraphicsContext.hpp"

/*
	Stores the light sources of the scene
*/

namespace NVulkanEngine
{
	enum class ELightType
	{
		SUN   = 0,
		POINT = 1,
		AREA  = 2,
		COUNT = 3
	};

	struct SLightSource
	{
		ELightType m_Type      = ELightType::COUNT;
		glm::vec3  m_Position  = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3  m_Direction = glm::vec3(0.0f, 0.0f, 0.0f);
		float      m_Intensity = 1.0f;
	};

	class CLightManager
	{
	public:
		CLightManager() = default;
		~CLightManager() = default;

		void AddLightSource(ELightType type);
		void AddPosition(glm::vec3 position);
		void AddDirection(glm::vec3 rotation);
		void AddIntensity(float);
		void PushLight();

		void Cleanup(CGraphicsContext* context);

		SLightSource GetSunlight() { return m_Sunlight; };
		SLightSource GetLightSource(uint32_t index) { return m_Lights[index]; };

		const uint32_t GetNumLights() { return (uint32_t)m_Lights.size(); }

	private:
		SLightSource m_Sunlight = {};
		std::vector<SLightSource> m_Lights = {};

		int m_CurrentLightSourceIndex = 0;
	};
};