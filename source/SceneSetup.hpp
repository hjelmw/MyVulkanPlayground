#pragma once

#define ADD_MODEL(modelpath, texturepath, posX, posY, posZ, rotX, rotY, rotZ, scaleX, scaleY, scaleZ) \
	graphicsEngine.AddModelFromFilepath(modelpath); \
	graphicsEngine.SetModelTexture(texturepath); \
	graphicsEngine.SetModelPosition(posX, posY, posZ); \
	graphicsEngine.SetModelRotation(rotX, rotY, rotZ); \
	graphicsEngine.SetModelScaling(scaleX, scaleY, scaleZ); \
	graphicsEngine.PushModel();

#define ADD_LIGHT(lightType, posX, posY, posZ, dirX, dirY, dirZ, intensity) \
	graphicsEngine.AddLightSource(lightType); \
	graphicsEngine.SetLightPosition(posX, posY, posZ); \
	graphicsEngine.SetLightDirection(dirX, dirY, dirZ); \
	graphicsEngine.SetLightIntensity(intensity); \
	graphicsEngine.PushLight();

using namespace NVulkanEngine;

void SetupModels(CVulkanGraphicsEngine& graphicsEngine)
{
	ADD_MODEL("models/Box.obj", "textures/statue.jpg", 0.0f, 800.0f, -200.0f, 45.0f, 0.0f, 45.0f, 40.0f, 40.0f, 40.0f);
	ADD_MODEL("models/Box.obj", "textures/statue.jpg", 150.0f, 600.0f, 500.0f, 45.0f, 0.0f, 45.0f, 30.0f, 30.0f, 30.0f);
	ADD_MODEL("models/Floor.obj", "textures/box.png", 0.0f, -10.0f, 0.0f, 0.0f, 0.0f, 0.0f, 90.0f, 1.0f, 90.0f);
	ADD_MODEL("models/BigSphere.obj", "", 0.0f, 1500.0f, 80.0f, 0.0f, 0.0f, 0.0f, 15.0f, 15.0f, 15.0f);
	ADD_MODEL("models/NewShip.obj", "", 1800.0f, 1000.0f, -1000.0f, 0.0f, 0.0f, 0.0f, 15.0f, 15.0f, 15.0f);
}

void SetupLights(CVulkanGraphicsEngine& graphicsEngine)
{
	ADD_LIGHT(ELightType::POINT, 0.0f, 1000.0f, 30.0f, 0.0f, -1.0f, 0.0f, 100.0f);
}

void SetupScene(CVulkanGraphicsEngine& graphicsEngine)
{
	SetupModels(graphicsEngine);
	SetupLights(graphicsEngine);
}
