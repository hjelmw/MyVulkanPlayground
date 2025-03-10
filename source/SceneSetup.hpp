#pragma once

void SetupModels(NVulkanEngine::CVulkanGraphicsEngine& graphicsEngine)
{
	graphicsEngine.AddModelFilepath("models/Box.obj");
	graphicsEngine.SetModelTexture("textures/statue.jpg");
	graphicsEngine.SetModelPosition(0.0f, 60.0f, -200.0f);
	graphicsEngine.SetModelRotation(45.0f, 0.0f, 45.0f);
	graphicsEngine.SetModelScaling(40.0f, 40.0f, 40.0f);
	graphicsEngine.PushModel();
	//
	graphicsEngine.AddModelFilepath("models/Box.obj");
	graphicsEngine.SetModelTexture("textures/statue.jpg");
	graphicsEngine.SetModelPosition(150.0f, 120.0f, 500.0f);
	graphicsEngine.SetModelRotation(45.0f, 0.0f, 45.0f);
	graphicsEngine.SetModelScaling(30.0f, 30.0f, 30.0f);
	graphicsEngine.PushModel();
	//
	graphicsEngine.AddModelFilepath("models/Floor.obj");
	graphicsEngine.SetModelTexture("textures/box.png");
	graphicsEngine.SetModelPosition(0.0f, -10.0f, 0.0f);
	graphicsEngine.SetModelRotation(0.0f, 0.0f, 0.0f);
	graphicsEngine.SetModelScaling(90.0f, 1.0f, 90.0f);
	graphicsEngine.PushModel();
	//
	graphicsEngine.AddModelFilepath("models/BigSphere.obj");
	graphicsEngine.SetModelPosition(0.0f, 150.0f, 80.0f);
	graphicsEngine.SetModelScaling(15.0f, 15.0f, 15.0f);
	graphicsEngine.PushModel();
	//
	graphicsEngine.AddModelFilepath("models/Buddha.obj");
	graphicsEngine.SetModelTexture("textures/statue.jpg");
	graphicsEngine.SetModelPosition(0.0f, 150.0f, 200.0f);
	graphicsEngine.SetModelScaling(15.0f, 15.0f, 15.0f);
	graphicsEngine.PushModel();

	graphicsEngine.AddModelFilepath("models/NewShip.obj");
	graphicsEngine.SetModelPosition(1800.0, 150.0f, -1000.0f);
	graphicsEngine.SetModelScaling(15.0f, 15.0f, 15.0f);
	graphicsEngine.PushModel();

	graphicsEngine.AddModelFilepath("models/NewShip.obj");
	graphicsEngine.SetModelPosition(-2800.0, 800.0f, -1000.0f);
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
