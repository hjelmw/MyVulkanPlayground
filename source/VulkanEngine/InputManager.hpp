#pragma once

#include <glm/glm.hpp>

#include "Camera.hpp"
#include "GraphicsContext.hpp"

namespace NVulkanEngine
{
	class CInputManager
	{
	public:
		/* The input manager is a singleton class! */
		static CInputManager* GetInstance();
		~CInputManager() = default;

		// Should be called once per frame to update camera based on user input
		void UpdateCameraTransforms(CGraphicsContext* context, float deltaTime);

		CCamera* GetCamera();

		// Used as callback for GLFW keyboard input handling
		void ProcessKeyboardInputsImpl(GLFWwindow* window, int key, int scancode, int action, int mods);

		// Used as callback for GLFW mouse input handling
		void ProcessMouseInputImpl(GLFWwindow* window, int button, int action, int mods);
	private:
		CInputManager();
		static CInputManager* s_InputManagerInstance;

		struct SKeyStatus
		{
			// Moves camera
			bool m_Left;
			bool m_Right;
			bool m_Forward;
			bool m_Backward;
			bool m_Up;
			bool m_Down;

			// Exits app
			bool m_Esc;
		} m_KeyStatus;

		struct SMouseStatus
		{
			// Hold down to move rotate camera
			bool   m_RightButton;
			bool   m_FirstTimePressed;

			// Mouse position in screenspace
			double m_MouseX;
			double m_MouseY;
		} m_MouseStatus;

		struct SCameraTransforms
		{
			// Angles of rotation
			float     m_Pitch;
			float     m_Yaw;

			glm::vec3 m_CameraDirection;
			glm::vec3 m_CameraPosition;
		} m_CameraTransforms;

		CCamera m_Camera;

		void ResetInputs();
	};

	static void ProcessKeyboardInputs(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		CInputManager::GetInstance()->ProcessKeyboardInputsImpl(window, key, scancode, action, mods);
	}

	static void ProcessMouseInputs(GLFWwindow* window, int button, int action, int mods)
	{
		CInputManager::GetInstance()->ProcessMouseInputImpl(window, button, action, mods);
	}
};