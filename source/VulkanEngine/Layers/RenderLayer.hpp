#include <DrawNode.hpp>
#include <vector>

namespace NVulkanEngine
{
	  class CRenderLayer
	  {
			CRenderLayer() = default;
			~CRenderLayer() = default;

public:
			void Init();
			void AddNode();
			void Draw();
			void Cleanup();
private:
			std::vector<CDrawNode*> m_RenderNodes = {};			
	  };

};
