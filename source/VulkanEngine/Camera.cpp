#include "Camera.hpp"
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>


namespace NVulkanEngine
{
	void CCamera::SetPosition(glm::vec3 position)
	{
		m_CameraPosition = position;
	}

	void CCamera::SetDirection(glm::vec3 direction)
	{
		m_CameraDirection = direction;
	}

	void CCamera::SetUp(glm::vec3 up)
	{
		m_CameraUp = up;
	}

	glm::vec3 CCamera::GetPosition()
	{
		return m_CameraPosition;
	}

	glm::vec3 CCamera::GetDirection()
	{
		return m_CameraDirection;
	}

	glm::vec3 CCamera::GetUp()
	{
		return m_CameraUp;
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
			m_CameraPosition,            // Camera world pos
			m_CameraPosition + m_CameraDirection, // Center of world
			m_CameraUp
		);

		m_ProjectionMatrix = glm::perspective(
			glm::radians(90.0f),
			(float)context->GetRenderResolution().width / (float)context->GetRenderResolution().height,
			0.1f,
			1000.0f
		);
		m_ProjectionMatrix[1][1] *= -1; // Stupid vulkan requirement
	}
};