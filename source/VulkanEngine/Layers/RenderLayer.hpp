#include <DrawNode.hpp>
#include <vector>

namespace NVulkanEngine
{
	  class CDrawLayer
	  {
			CDrawLayer() = default;
			~CDrawLayer() = default;

public:
			void Init();
			void AddDrawNode(CDrawNode* drawNode);
			void AddDrawInstance(uint32_t drawNodeSlot);
			void Draw();
			void Cleanup();
private:
			std::vector<CDrawNode*> m_RenderNodes = {};
			std__vectoruint32_t
	  };

};
