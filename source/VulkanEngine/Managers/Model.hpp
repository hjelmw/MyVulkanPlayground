#pragma once

#include <VulkanGraphicsEngineUtils.hpp>
#include <GraphicsContext.hpp>
#include <Managers/Texture.hpp>
#include <DrawNodes/BindingTable.hpp>

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <vector>

/* 
	A model is a collection of meshes with vertices to be rendered. One material per mesh
*/

struct SModelVertex
{
	glm::vec3 m_Position = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 m_Color    = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec2 m_TexCoord = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 m_Normal   = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 m_Tangent  = glm::vec3(0.0f, 0.0f, 0.0f);

	bool operator==(const SModelVertex& other) const {
		return m_Position == other.m_Position &&
		m_Color == other.m_Color              &&
		m_TexCoord == other.m_TexCoord        &&
		m_Normal == other.m_Normal;
	}
};

// Hash function for storing unique vertices in unordered_map
namespace std {
	template<> struct hash<SModelVertex> {
		size_t operator()(SModelVertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.m_Position + vertex.m_Color + vertex.m_Normal)));
		}
	};
};

struct SDescriptorSets
{
	std::vector<VkDescriptorSet>      m_DescriptorSets   = { VK_NULL_HANDLE };
	std::vector<VkWriteDescriptorSet> m_WriteDescriptors = { };
};

// Unifom buffer object and its device memory
struct SUniformMemoryBuffer
{
	VkBuffer       m_Buffer = VK_NULL_HANDLE;
	VkDeviceMemory m_Memory = VK_NULL_HANDLE;

};

// A mesh is a subset of polygons inside the model. Model is split up this way to handle multiple materials per mdel
struct SMesh
{
	int      m_MaterialId = -1;
	uint32_t m_StartIndex = 0;
	uint32_t m_NumVertices = 0;
};

struct SModelMaterial
{
	glm::vec4    m_Diffuse          = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	//
	glm::float32 m_Shininess        = glm::float32(0.0f);
	glm::float32 m_Metallness       = glm::float32(0.0f);
	glm::float32 m_Fresnel          = glm::float32(0.0f);
	glm::float32 m_Emission         = glm::float32(0.0f);
	//
	glm::float32 m_Transparency     = glm::float32(0.0f);
	glm::float32 m_Reflectivity     = glm::float32(0.0f);
	glm::int32   m_UseAlbedoTexture = 0;
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
		void               SetModelTexturePath(const std::string& modeTexturePath);
		
		std::string        GetModelTexturePath();

		SDescriptorSets&   GetDescriptorSetsRef();
		SDescriptorSets    GetDescriptorSets();

		glm::mat4          GetTransform();
		void               SetTransform(glm::mat4 transform);

		void               SetUsesModelTexture(bool uses_texture);
		bool               UsesModelTexture();

		CTexture*          GetModelTexture();
		void               SetModelTexture(CTexture* texture);

		void               CreateGeometryBindingTable(CGraphicsContext* context);
		void               CreateShadowBindingTable(CGraphicsContext* context);

		// Creates the geometry and shadow UBOs. Fetch with functions just below this
		void               CreateGeometryMemoryBuffer(CGraphicsContext* context, const VkDeviceSize size);
		void               CreateShadowMemoryBuffer(CGraphicsContext* context, const VkDeviceSize size);

		SUniformMemoryBuffer GetGeometryMemoryBuffer();
		SUniformMemoryBuffer GetShadowMemoryBuffer();

		VkDescriptorSetLayout GetModelDescriptorSetLayout();

		uint32_t           GetNumMeshes();
		SMesh              GetMesh(const uint32_t index);
		
		uint32_t           GetNumIndices();

		// Get number of indices of model
		SModelMaterial     GetMaterial(uint32_t materialId);

		// Bind vertex and index buffers for a mesh
		void               BindVertexAndIndexBuffers(VkCommandBuffer commandBuffer);
		void               BindGeometryTable(CGraphicsContext* context, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
		void               BindShadowTable(CGraphicsContext* context, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

		// Cleanup model and meshes
		void               Cleanup(CGraphicsContext* context);
	private:
		std::string            m_ModelFilepath      = {};
		std::string            m_MaterialFilepath   = {};
		std::string            m_ModelTexturePath   = {};

		std::vector<SMesh>          m_Meshes        = {};
		std::vector<SModelMaterial> m_Materials     = {};
			
		std::vector<SModelVertex>   m_Vertices      = {};
		std::vector<uint32_t>       m_Indices       = {};

		glm::mat4              m_Transform          = glm::identity<glm::mat4>();

		SDescriptorSets        m_DescriptorSets     = {};

		VkBuffer               m_VertexBuffer       = VK_NULL_HANDLE;
		VkDeviceMemory         m_VertexBufferMemory = VK_NULL_HANDLE;

		VkBuffer               m_IndexBuffer        = VK_NULL_HANDLE;
		VkDeviceMemory         m_IndexBufferMemory  = VK_NULL_HANDLE;

		// UBOs
		SUniformMemoryBuffer   m_GeometryBuffer     = {};
		SUniformMemoryBuffer   m_ShadowBuffer       = {};

		uint32_t               m_GeometryBufferSize = 0;
		uint32_t               m_ShadowBufferSize   = 0;

		bool                   m_UsesModelTexture   = false;
		CTexture*              m_ModelTexture       = nullptr;

		CBindingTable*         m_GeometryTable      = nullptr;
		CBindingTable*         m_ShadowTable        = nullptr;

		// Load a model .obj file using relative path
		bool LoadModel(const std::string modelFilepath, const std::string materialSearchPath);
		void CreateVertexBuffer(CGraphicsContext* context);
		void CreateIndexBuffer(CGraphicsContext* context);

		// Generate a normal vector given 3 points (vertices)
		glm::vec3 GenerateNormal(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2);
	};
}
