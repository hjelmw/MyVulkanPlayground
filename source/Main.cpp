#include <VulkanGraphicsEngine.hpp>
#include "SceneSetup.hpp" // Has to be included last

int main()
{
	using namespace NVulkanEngine;

	CVulkanGraphicsEngine graphicsEngine;

	graphicsEngine.Initialize();
	SetupScene(graphicsEngine);
	graphicsEngine.CreateScene();

	while (graphicsEngine.IsRunning())
	{
		graphicsEngine.DrawFrame();
	}

	graphicsEngine.Cleanup();

}