#include "VulkanGraphicsEngine.hpp"

int main()
{
	using namespace NVulkanEngine;

	CVulkanGraphicsEngine graphicsEngine;

	graphicsEngine.AddModel("assets/NewShip.obj");
	graphicsEngine.SetModelPosition(0.0f, 10.0f, 0.0f);
	graphicsEngine.SetModelRotation(0.0f, 0.0f, 0.0f);
	graphicsEngine.SetModelScaling(2.0f, 2.0f, 2.0f);
	graphicsEngine.AddModel("assets/LandingPad.obj");
	graphicsEngine.SetModelPosition(0.0f, -10.0f, 0.0f);
	graphicsEngine.SetModelRotation(0.0f, 90.0f, 0.0f);
	graphicsEngine.SetModelScaling(2.5f, 2.5f, 2.5f);
	graphicsEngine.AddModel("assets/BigSphere.obj");
	graphicsEngine.SetModelPosition(0.0f, 30.0f, 30.0f);
	graphicsEngine.SetModelScaling(0.3f, 0.3f, 0.3f);

	graphicsEngine.Initialize();

	while (true)
	{
		graphicsEngine.DrawFrame();
	}

	graphicsEngine.Cleanup();

}