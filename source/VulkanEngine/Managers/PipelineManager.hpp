#pragma once

#include <vector>
#include <DrawNodes/Utils/Pipeline.hpp>
#include "GraphicsContext.hpp"

enum class EPipelineVariant
{
	Geometry         = 0,
	Shadows          = 1,
	Terrain          = 2,
	Atmospherics     = 3,
	DeferredLighting = 4,
	Count            = 5
};

namespace NVulkanEngine
{
	class CPipelineManager
	{
	public:
		CPipelineManager() = default;
		~CPipelineManager() = default;

		void RegisterPipeline(CPipeline* pipeline);
		void CreatePipelines(CGraphicsContext* context, VkDescriptorSetLayout descriptorLayout);

	private:
		std::vector<CPipeline*> m_Pipelines = {};
	};
};