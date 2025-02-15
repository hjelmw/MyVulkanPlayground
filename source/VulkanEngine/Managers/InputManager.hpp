#pragma once

#include <glm/glm.hpp>

#include "Camera.hpp"
#include "GraphicsContext.hpp"

/*
	Input manager keeps track of the camera and updates based on keyboard/mouse input
*/

namespace NVulkanEngine
{
	class CInputManager
	{
	public:
		CInputManager() = default;
		~CInputManager() = default;

		// Should be called once per frame to update camera based on user input
		void UpdateCameraTransforms(CGraphicsContext* context, float deltaTime);

		CCamera* GetCamera();

		// Used as callback for GLFW keyboard input handling
		void ProcessKeyboardInputs(GLFWwindow* window, int key, int scancode, int action, int mods);

		// Used as callback for GLFW mouse input handling
		void ProcessMouseInput(GLFWwindow* window, int button, int action, int mods);
	private:
		CCamera m_Camera;

		struct SKeyStatus
		{
			// Moves camera
			bool m_Left     = false;
			bool m_Right    = false;
			bool m_Forward  = false;
			bool m_Backward = false;
			bool m_Up       = false;
			bool m_Down     = false;
			bool m_Shift    = false;

			// Exits app
			bool m_Esc = false;
		} m_KeyStatus;

		struct SMouseStatus
		{
			// Hold down to move rotate camera
			bool   m_RightButton      = false;
			bool   m_FirstTimePressed = true;

			// Mouse position in screenspace
			double m_MouseX = 0.0f;
			double m_MouseY = 0.0f;
		} m_MouseStatus;

		struct SCameraTransforms
		{
			// Angles of rotation
			float     m_Pitch =  0.0f;
			float     m_Yaw   = -90.0f;

			glm::vec3 m_CameraDirection      = glm::vec3(0.0f, 0.0f, 0.0f);;
			glm::vec3 m_PlanetCameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);;
		} m_CameraTransforms;

	};
};