#include "VulkanGraphicsEngine.hpp"

int main()
{
	using namespace NVulkanEngine;

	CVulkanGraphicsEngine graphicsEngine;

	graphicsEngine.AddModel("assets/NewShip.obj");
	graphicsEngine.SetModelPosition(0.0f, 10.0f, 0.0f);
	graphicsEngine.SetModelRotation(0.0f, 90.0f, 0.0f);
	graphicsEngine.AddModel("assets/BigSphere");
	graphicsEngine.SetModelPosition(0.0f, -10.0f, 0.0f);

	graphicsEngine.Initialize();

	while (true)
	{
		graphicsEngine.DrawFrame();
	}

	graphicsEngine.Cleanup();

}