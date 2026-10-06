#pragma once

void SetupModels(NVulkanEngine::CVulkanGraphicsEngine& graphicsEngine)
{
	graphicsEngine.AddModelFromFilepath("models/Box.obj");
	graphicsEngine.SetModelTexture("textures/statue.jpg");
	graphicsEngine.SetModelPosition(0.0f, 800.0f, -200.0f);
	graphicsEngine.SetModelRotation(45.0f, 0.0f, 45.0f);
	graphicsEngine.SetModelScaling(40.0f, 40.0f, 40.0f);
	graphicsEngine.PushModel();
	//
	graphicsEngine.AddModelFromFilepath("models/Box.obj");
	graphicsEngine.SetModelTexture("textures/statue.jpg");
	graphicsEngine.SetModelPosition(150.0f, 600.0f, 500.0f);
	graphicsEngine.SetModelRotation(45.0f, 0.0f, 45.0f);
	graphicsEngine.SetModelScaling(30.0f, 30.0f, 30.0f);
	graphicsEngine.PushModel();
	//
	//graphicsEngine.AddModelFromFilepath("models/Floor.obj");
	//graphicsEngine.SetModelTexture("textures/box.png");
	//graphicsEngine.SetModelPosition(0.0f, -10.0f, 0.0f);
	//graphicsEngine.SetModelRotation(0.0f, 0.0f, 0.0f);
	//graphicsEngine.SetModelScaling(90.0f, 1.0f, 90.0f);
	//graphicsEngine.PushModel();
	//
	graphicsEngine.AddModelFromFilepath("models/BigSphere.obj");
	graphicsEngine.SetModelPosition(0.0f, 1500.0f, 80.0f);
	graphicsEngine.SetModelScaling(15.0f, 15.0f, 15.0f);
	graphicsEngine.PushModel();
	//
	graphicsEngine.AddModelFromFilepath("models/NewShip.obj");
	graphicsEngine.SetModelPosition(1800.0, 1000.0f, -1000.0f);
	graphicsEngine.SetModelScaling(15.0f, 15.0f, 15.0f);
	graphicsEngine.PushModel();

	graphicsEngine.AddModelFromFilepath("models/NewShip.obj");
	graphicsEngine.SetModelPosition(-2800.0, 1200.0f, -1000.0f);
	graphicsEngine.SetModelScaling(15.0f, 15.0f, 15.0f);
	graphicsEngine.PushModel();

}

void SetupLights(NVulkanEngine::CVulkanGraphicsEngine& graphicsEngine)
{
	graphicsEngine.AddLightSource(NVulkanEngine::ELightType::SUN);
	graphicsEngine.SetLightDirection(0.0f, -1.0f, 0.0f);
	graphicsEngine.SetLightIntensity(10.0f);
	graphicsEngine.PushLight();
}

void SetupScene(NVulkanEngine::CVulkanGraphicsEngine& graphicsEngine)
{
	SetupModels(graphicsEngine);
	SetupLights(graphicsEngine);
}
