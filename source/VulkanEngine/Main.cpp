#include "VulkanGraphicsEngine.hpp"

int main()
{
	using namespace NVulkanEngine;

	CVulkanGraphicsEngine graphicsEngine;

	/*int constexpr num_boxes = 16;
	for(int i = 0; i < num_boxes; i++)
	{
	}*/

	graphicsEngine.AddModel("assets/Box.obj");
	graphicsEngine.SetModelPosition(30.0f, 10.0f, 0.0f);
	graphicsEngine.SetModelRotation(0.0f, 0.0f, 0.0f);
	graphicsEngine.SetModelScaling(15.0f, 15.0f, 15.0f);
	graphicsEngine.AddModel("assets/Box.obj");
	graphicsEngine.SetModelPosition(150.0f, 10.0f, 150.0f);
	graphicsEngine.SetModelRotation(0.0f, 0.0f, 0.0f);
	graphicsEngine.SetModelScaling(15.0f, 15.0f, 15.0f);
	graphicsEngine.AddModel("assets/Floor.obj");
	graphicsEngine.SetModelPosition(0.0f, -10.0f, 0.0f);
	graphicsEngine.SetModelRotation(0.0f, 0.0f, 0.0f);
	graphicsEngine.SetModelScaling(30.0f, 1.0f, 30.0f);
	graphicsEngine.AddModel("assets/BigSphere.obj");
	graphicsEngine.SetModelPosition(0.0f, 40.0f, 80.0f);
	graphicsEngine.SetModelScaling(3.0f, 3.0f, 3.0f);

	graphicsEngine.Initialize();

	while (true)
	{
		graphicsEngine.DrawFrame();
	}

	graphicsEngine.Cleanup();

}