#pragma once

#include "GraphicsContext.hpp"
#include "VulkanGraphicsEngineUtils.hpp"
#include "Texture.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <vector>

struct SVertex
{
	glm::vec3 m_Position = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 m_Color    = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec2 m_TexCoord = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 m_Normal   = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 m_Tangent  = glm::vec3(0.0f, 0.0f, 0.0f);

	// Vertex bindings
	static VkVertexInputBindingDescription GetVertexBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};

		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(SVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Move to the next data entry after each vertex

		return bindingDescription;
	};

	// Vertex attributes
	static std::vector<VkVertexInputAttributeDescription> GetVertexInputAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);

		// inPosition
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; //vec2
		attributeDescriptions[0].offset = offsetof(SVertex, m_Position);

		// inColor
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; //vec3
		attributeDescriptions[1].offset = offsetof(SVertex, m_Color);

		// texCoord
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(SVertex, m_TexCoord);

		// normal
		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(SVertex, m_Normal);

		// tangent
		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[4].offset = offsetof(SVertex, m_Tangent);

		return attributeDescriptions;
	}

	bool operator==(const SVertex& other) const {
		return m_Position == other.m_Position && m_Color == other.m_Color && m_TexCoord == other.m_TexCoord;
	}
};

// Hash function for storing unique vertices in unordered_map
namespace std {
	template<> struct hash<SVertex> {
		size_t operator()(SVertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.m_Position)));
		}
	};
};

// A mesh is a subset of polygons inside the model. Model is split up this way to handle multiple materials per mdel
struct SMesh
{
	int      m_MaterialId = -1;
	uint32_t m_StartIndex = 0;
	uint32_t m_NumVertices = 0;
};

struct SMaterial
{
	glm::vec3    m_Diffuse      = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::float32 m_Reflectivity = glm::float32(0.0f);
	glm::float32 m_Shininess    = glm::float32(0.0f);
	glm::float32 m_Metallness   = glm::float32(0.0f);
	glm::float32 m_Fresnel      = glm::float32(0.0f);
	glm::float32 m_Emission     = glm::float32(0.0f);
	glm::float32 m_Transparency = glm::float32(0.0f);
	glm::vec3    m_Pad0         = glm::vec3(0xDEADBEEF, 0xDEADBEEF, 0xDEADBEEF);
	glm::vec3    m_Pad1         = glm::vec3(0xDEADBEEF, 0xDEADBEEF, 0xDEADBEEF);
};

namespace NVulkanEngine
{
	class CModel
	{
	public:
		CModel() = default;
		~CModel() = default;

		// Loads the model and creates meshes. Initializes vertex and index buffers and assigns materials for them
		void               CreateModelMeshes(CGraphicsContext* context);
		void               SetModelFilepath(const std::string& modelFilepath, const std::string materialSearchPath);

		void               AllocateDescriptors(CGraphicsContext* context, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, VkSampler sampler, const uint32_t numDescriptors);

		glm::mat4          GetTransform();
		void               SetTransform(glm::mat4 transform);

		CTexture*          GetModelTexture();
		void               SetModelTexture(CTexture* texture);


		std::vector<SMesh> GetMeshes();
		
		uint32_t           GetNumIndices();

		// Get number of indices of model
		SMaterial          GetMaterial(uint32_t materialId);

		// Bind vertex and index buffers for a mesh
		void               BindMesh(VkCommandBuffer commandBuffer);
		void               BindMesh(VkCommandBuffer commandBuffer, SMesh mesh);

		// Cleanup model and meshes
		void               CleanupModel(CGraphicsContext* context);
	private:
		std::string            m_ModelFilepath      = {};
		std::string            m_MaterialFilepath   = {};

		std::vector<SMesh>     m_Meshes             = {};
		std::vector<SMaterial> m_Materials          = {};
			
		std::vector<SVertex>   m_Vertices           = {};
		std::vector<uint32_t>  m_Indices            = {};

		glm::mat4              m_Transform          = glm::identity<glm::mat4>();

		VkBuffer               m_VertexBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory         m_VertexBufferMemory = VK_NULL_HANDLE;

		VkBuffer               m_IndexBuffer        = VK_NULL_HANDLE;
		VkDeviceMemory         m_IndexBufferMemory  = VK_NULL_HANDLE;

		VkBuffer               m_GeometryBuffer     = VK_NULL_HANDLE;
		VkDeviceMemory         m_GeometryBufferMemory = VK_NULL_HANDLE;

		std::vector<VkDescriptorSet> m_DescriptorSets = { VK_NULL_HANDLE };

		CTexture*              m_ModelTexture       = nullptr;

		// Load a model .obj file using relative path
		bool LoadModel(const std::string modelFilepath, const std::string materialSearchPath);
		void CreateVertexBuffer(CGraphicsContext* context);
		void CreateIndexBuffer(CGraphicsContext* context);

		// Generate a normal vector given 3 points (vertices)
		glm::vec3 GenerateNormal(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2);
	};
}
