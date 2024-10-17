#include "Camera.hpp"


namespace NVulkanEngine
{
	void CCamera::SetPosition(glm::vec3 position)
	{
		m_PlanetCameraPosition = position;
	}

	void CCamera::SetDirection(glm::vec3 direction)
	{
		m_CameraDirection = direction;
	}

	void CCamera::SetUp(glm::vec3 up)
	{
		m_CameraUp = up;
	}

	void CCamera::SetNear(float near)
	{
		m_Near = near;
	}

	void CCamera::SetFar(float far)
	{
		m_Far = far;
	}

	glm::vec3 CCamera::GetPosition()
	{
		return m_PlanetCameraPosition;
	}

	glm::vec3 CCamera::GetDirection()
	{
		return m_CameraDirection;
	}

	glm::vec3 CCamera::GetUp()
	{
		return m_CameraUp;
	}

	float CCamera::GetNear()
	{
		return m_Near;
	}

	float CCamera::GetFar()
	{
		return m_Far;
	}

	glm::mat4 CCamera::GetLookAtMatrix()
	{
		return m_LookAtMatrix;
	}

	glm::mat4 CCamera::GetProjectionMatrix()
	{
		return m_ProjectionMatrix;
	}

	void CCamera::UpdateCamera(CGraphicsContext* context)
	{
		m_LookAtMatrix = glm::lookAt(
			m_PlanetCameraPosition,                     // Camera world pos
			m_PlanetCameraPosition + m_CameraDirection, // Camera look dir
			m_CameraUp
		);

		m_ProjectionMatrix = glm::perspective(
			glm::radians(90.0f),
			(float)context->GetRenderResolution().width / (float)context->GetRenderResolution().height,
			m_Near,
			m_Far
		);
		m_ProjectionMatrix[1][1] *= -1; // Stupid vulkan requirement
	}
};