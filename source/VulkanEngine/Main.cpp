#include "VulkanGraphicsEngine.hpp"

int main()
{
	using namespace NVulkanEngine;

	CVulkanGraphicsEngine graphicsEngine;

	graphicsEngine.AddModel("assets/box.obj");
	graphicsEngine.SetModelPosition(30.0f, 30.0f, 0.0f);
	graphicsEngine.SetModelRotation(45.0f, 0.0f, 45.0f);
	graphicsEngine.SetModelScaling(15.0f, 15.0f, 15.0f);
	graphicsEngine.AddModel("assets/Floor.obj");
	graphicsEngine.SetModelTexture("assets/box.png");
	graphicsEngine.SetModelPosition(0.0f, -10.0f, 0.0f);
	graphicsEngine.SetModelRotation(0.0f, 0.0f, 0.0f);
	graphicsEngine.SetModelScaling(90.0f, 1.0f, 90.0f);
	graphicsEngine.AddModel("assets/BigSphere.obj");
	graphicsEngine.SetModelPosition(0.0f, 40.0f, 80.0f);
	graphicsEngine.SetModelScaling(3.0f, 3.0f, 3.0f);

	//graphicsEngine.AddModel("assets/NewShip.obj");
	//graphicsEngine.SetModelPosition(200.0f, 0, 0.0f);
	//graphicsEngine.SetModelRotation(0.0f, 45.0f, 0.0f);
	//graphicsEngine.SetModelScaling(15.0f, 15.0f, 15.0f);
	//graphicsEngine.AddModel("assets/LandingPad.obj");
	//graphicsEngine.SetModelPosition(200.0f, -150.0f, 0.0f);
	//graphicsEngine.SetModelRotation(0.0f, 180.0f, 0.0f);
	//graphicsEngine.SetModelScaling(15.0f, 15.0f, 15.0f);


	graphicsEngine.Initialize();

	while (true)
	{
		graphicsEngine.DrawFrame();
	}

	graphicsEngine.Cleanup();

}