#include "LightManager.hpp"


namespace NVulkanEngine
{
	void CLightManager::AddLightSource(ELightType type)
	{
		SLightSource lightSource{};
		lightSource.m_Type = type;

		m_Lights.push_back(lightSource);
	}

	void CLightManager::AddPosition(glm::vec3 position)
	{
		m_Lights[m_CurrentLightSourceIndex].m_Position = position;
	}

	void CLightManager::AddDirection(glm::vec3 direction)
	{
		m_Lights[m_CurrentLightSourceIndex].m_Direction = direction;
	}

	void CLightManager::AddIntensity(float intensity)
	{
		m_Lights[m_CurrentLightSourceIndex].m_Intensity = intensity;
	}

	void CLightManager::PushLight()
	{
		m_CurrentLightSourceIndex++;
	}

	void CLightManager::Cleanup(CGraphicsContext* context)
	{
		m_Lights.clear();
	}
};