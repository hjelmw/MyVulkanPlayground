
#include "InputManager.hpp"
#include <GLFW/glfw3.h>

namespace NVulkanEngine
{
	void CInputManager::ProcessKeyboardInputs(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (action == GLFW_PRESS)
		{
			switch (key)
			{
			case GLFW_KEY_ESCAPE:
				m_KeyStatus.m_Esc = true;
				break;
			case GLFW_KEY_LEFT_SHIFT:
				m_KeyStatus.m_Shift = true;
				break;
			case GLFW_KEY_W:
				m_KeyStatus.m_Forward = true;
				break;
			case GLFW_KEY_A:
				m_KeyStatus.m_Left = true;
				break;
			case GLFW_KEY_S:
				m_KeyStatus.m_Backward = true;
				break;
			case GLFW_KEY_D:
				m_KeyStatus.m_Right = true;
				break;
			case GLFW_KEY_E:
				m_KeyStatus.m_Up = true;
				break;
			case GLFW_KEY_Q:
				m_KeyStatus.m_Down = true;
				break;
			}
		}
		else if (action == GLFW_RELEASE)
		{
			switch (key)
			{
			case GLFW_KEY_ESCAPE:
				m_KeyStatus.m_Esc = false;
				break;
			case GLFW_KEY_LEFT_SHIFT:
				m_KeyStatus.m_Shift = false;
				break;
			case GLFW_KEY_W:
				m_KeyStatus.m_Forward = false;
				break;
			case GLFW_KEY_A:
				m_KeyStatus.m_Left = false;
				break;
			case GLFW_KEY_S:
				m_KeyStatus.m_Backward = false;
				break;
			case GLFW_KEY_D:
				m_KeyStatus.m_Right = false;
				break;
			case GLFW_KEY_E:
				m_KeyStatus.m_Up = false;
				break;
			case GLFW_KEY_Q:
				m_KeyStatus.m_Down = false;
				break;
			}
		}
	}

	void CInputManager::ProcessMouseInput(GLFWwindow* window, int button, int action, int mods)
	{
		if (action == GLFW_PRESS)
		{
			switch (button)
			{
			case GLFW_MOUSE_BUTTON_RIGHT:
				m_MouseStatus.m_RightButton = true;
				m_MouseStatus.m_FirstTimePressed = true;
				break;
			}
		}
		else if (action == GLFW_RELEASE)
		{
			switch (button)
			{
			case GLFW_MOUSE_BUTTON_RIGHT:
				m_MouseStatus.m_RightButton = false;
				break;
			}
		}
	}

	void CInputManager::UpdateCameraTransforms(CGraphicsContext* context, float deltaTime)
	{
		const float sensitivity_mouse = 1.0f;
		const float sensitivity_keys  = m_KeyStatus.m_Shift ? 8.0f : 2.0f;

		glm::vec3 cameraPosition  = m_Camera.GetPosition();
		glm::vec3 cameraDirection = m_Camera.GetDirection();
		glm::vec3 cameraUp        = m_Camera.GetUp();


		if (m_MouseStatus.m_RightButton)
		{
			if (m_MouseStatus.m_FirstTimePressed)
			{
				glfwGetCursorPos(context->GetGLFWWindow(), &m_MouseStatus.m_MouseX, &m_MouseStatus.m_MouseY);
				m_MouseStatus.m_FirstTimePressed = false;
			}

			double previousMouseX = m_MouseStatus.m_MouseX;
			double previousMouseY = m_MouseStatus.m_MouseY;

			double currentMouseX, currentMouseY;
			glfwGetCursorPos(context->GetGLFWWindow(), &currentMouseX, &currentMouseY);

			float offsetX = static_cast<float>(previousMouseX - currentMouseX);
			float offsetY = static_cast<float>(currentMouseY  - previousMouseY);

			offsetX *= (deltaTime + sensitivity_mouse);
			offsetY *= (deltaTime + sensitivity_mouse);

			m_CameraTransforms.m_Yaw   += offsetX;
			m_CameraTransforms.m_Pitch += offsetY;

			if (m_CameraTransforms.m_Pitch > 89.0f)
				m_CameraTransforms.m_Pitch = 89.0f;

			if (m_CameraTransforms.m_Pitch < -89.0f)
				m_CameraTransforms.m_Pitch = -89.0f;

			m_MouseStatus.m_MouseX = currentMouseX;
			m_MouseStatus.m_MouseY = currentMouseY;
		}

		constexpr float pi = glm::pi<glm::float32>();
		float yawRadians   = m_CameraTransforms.m_Yaw * (pi / 180.0f);
		float pitchRadians = m_CameraTransforms.m_Pitch * (pi / 180.0f);

		glm::vec3 lookDirection;
		lookDirection.x = -sin(yawRadians) * cos(pitchRadians);
		lookDirection.y = -sin(pitchRadians);
		lookDirection.z = -cos(yawRadians) * cos(pitchRadians);

		glm::vec3 cameraFront = glm::normalize(lookDirection);

		if (m_KeyStatus.m_Forward)
		{
			cameraPosition += cameraFront * (deltaTime + sensitivity_keys);
		}
		if (m_KeyStatus.m_Backward)
		{
			cameraPosition -= cameraFront * (deltaTime + sensitivity_keys);
		}
		if (m_KeyStatus.m_Left)
		{
			cameraPosition -= glm::normalize(glm::cross(cameraDirection, cameraUp)) * (deltaTime + sensitivity_keys);
		}
		if (m_KeyStatus.m_Right)
		{
			cameraPosition += glm::normalize(glm::cross(cameraDirection, cameraUp)) * (deltaTime + sensitivity_keys);
		}
		if (m_KeyStatus.m_Up)
		{
			cameraPosition.y += (deltaTime + sensitivity_keys);
		}
		if (m_KeyStatus.m_Down)
		{
			cameraPosition.y -= (deltaTime + sensitivity_keys);
		}

		m_Camera.SetPosition(cameraPosition);
		m_Camera.SetDirection(cameraFront);
		m_Camera.SetNear(0.1f);
		m_Camera.SetFar(10000.0f);
		m_Camera.UpdateCamera(context);
	}

	CCamera* CInputManager::GetCamera()
	{
		return &m_Camera;
	}
}
