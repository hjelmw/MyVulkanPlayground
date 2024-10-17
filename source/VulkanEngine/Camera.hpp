#pragma once

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

#include "GraphicsContext.hpp"

namespace NVulkanEngine
{
	class CCamera
	{
	public:
		CCamera() = default;
		~CCamera() = default;

		void SetPosition(glm::vec3 position);
		void SetDirection(glm::vec3 direction);
		void SetUp(glm::vec3 up);
		void SetNear(float near);
		void SetFar(float far);

		glm::vec3 GetPosition();
		glm::vec3 GetDirection();
		glm::vec3 GetUp();

		float GetNear();
		float GetFar();

		void UpdateCamera(CGraphicsContext* context);

		glm::mat4 GetLookAtMatrix();
		glm::mat4 GetProjectionMatrix();

	private:
		glm::vec3       m_PlanetCameraPosition  = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3       m_CameraDirection = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3       m_CameraUp        = glm::vec3(0.0f, 0.0f, 0.0f);

		glm::mat4 m_LookAtMatrix     = glm::identity<glm::mat4>();
		glm::mat4 m_ProjectionMatrix = glm::identity<glm::mat4>();

		float m_Near = 0.0f;
		float m_Far  = 0.0f;
	};

};