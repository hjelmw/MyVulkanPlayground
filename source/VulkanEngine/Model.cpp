#include "Model.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <cstdlib>
#include <iostream>
#include <unordered_map>


namespace NVulkanEngine
{
	void CModel::SetModelFilepath(const std::string& modelFilepath, const std::string materialSearchPath)
	{
		m_ModelFilepath = modelFilepath;
		m_MaterialFilepath = materialSearchPath;
	}

	void CModel::CreateModelMeshes(CGraphicsContext* context)
	{
		LoadModel(m_ModelFilepath, m_MaterialFilepath);
		CreateVertexBuffer(context);
		CreateIndexBuffer(context);
	}

	bool CModel::LoadModel(const std::string modelFilepath, const std::string materialSearchPath)
	{
		tinyobj::ObjReaderConfig reader_config;
		reader_config.mtl_search_path = materialSearchPath; // Path to material files

		tinyobj::ObjReader reader;

		if (!reader.ParseFromFile(modelFilepath, reader_config))
		{
			if (!reader.Error().empty())
			{
				std::cerr << " --- TinyObjReader error ---\n" << std::endl << std::endl << reader.Error();
			}
			exit(1);
		}

		if (!reader.Warning().empty()) 
		{
			std::cerr << " --- TinyObjReader warning ---\n" << std::endl << std::endl << reader.Warning();
		}

		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();
		auto& materials = reader.GetMaterials();

		for (const auto& material: materials)
		{
			SMaterial newMaterial;

			tinyobj::real_t cr = material.diffuse[0];
			tinyobj::real_t cg = material.diffuse[1];
			tinyobj::real_t cb = material.diffuse[2];

			newMaterial.m_Diffuse      = glm::vec3(cr, cg, cb);
			newMaterial.m_Reflectivity = material.specular[0];
			newMaterial.m_Metallness   = material.metallic;
			newMaterial.m_Fresnel      = material.sheen;
			newMaterial.m_Shininess    = material.shininess;
			newMaterial.m_Emission     = material.emission[0];
			newMaterial.m_Transparency = material.transmittance[0];

			m_Materials.push_back(newMaterial);
		}

		

		//std::vector<SVertex> uniqueVertices{};
		std::unordered_map<SVertex, uint32_t> uniqueVertices{};
		uint32_t verticesSoFar = 0;

		// Loop over shapes
		for (size_t s = 0; s < shapes.size(); s++)
		{
			// per-face material
			int      nextMaterialId = shapes[s].mesh.material_ids[0];
			uint32_t numMaterialsInShape = 0;
			uint32_t nextMaterialStartingFace = 0;

			std::vector<bool> finishedMaterials(materials.size(), false);

			// -1 means no more material
			while (nextMaterialId != -1)
			{
				int currentMaterialId = nextMaterialId;
				int currentMaterialStartingFace = nextMaterialStartingFace;
				
				nextMaterialId           = -1;
				nextMaterialStartingFace = -1;
				
				SMesh mesh{};
				mesh.m_MaterialId = currentMaterialId;
				mesh.m_StartIndex = verticesSoFar;

				numMaterialsInShape += 1;

				uint64_t numberOfFaces = shapes[s].mesh.indices.size() / 3;
				for (uint32_t i = currentMaterialStartingFace; i < numberOfFaces; i++)
				{
					// Check if a new material appears
					if (shapes[s].mesh.material_ids[i] != currentMaterialId)
					{
						if (nextMaterialId >= 0)
							continue;
						else if (finishedMaterials[shapes[s].mesh.material_ids[i]])
							continue;
						else
						{
							// Found a new material that we have not processed.
							nextMaterialId = shapes[s].mesh.material_ids[i];
							nextMaterialStartingFace = i;
						}
					}
					else
					{
						// Loop over vertices in the face.
						for (int j = 0; j < 3; j++)
						{
							SVertex newVertex{};

							// access to vertex
							tinyobj::index_t idx = shapes[s].mesh.indices[i * 3 + j];
							tinyobj::real_t vx = attrib.vertices[size_t(idx.vertex_index) * 3 + 0];
							tinyobj::real_t vy = attrib.vertices[size_t(idx.vertex_index) * 3 + 1];
							tinyobj::real_t vz = attrib.vertices[size_t(idx.vertex_index) * 3 + 2];

							newVertex.m_Position = glm::vec3(vx, vy, vz);

							// Check if `normal_index` is zero or positive. negative = no normal data
							if (idx.normal_index == -1)
							{
								tinyobj::index_t idx0 = shapes[s].mesh.indices[i * 3 + 0];
								tinyobj::index_t idx1 = shapes[s].mesh.indices[i * 3 + 1];
								tinyobj::index_t idx2 = shapes[s].mesh.indices[i * 3 + 2];

								glm::vec3 v0 = glm::vec3(
									attrib.vertices[idx0.vertex_index * 3 + 0],
                                    attrib.vertices[idx0.vertex_index * 3 + 1],
                                    attrib.vertices[idx0.vertex_index * 3 + 2]
								);

								glm::vec3 v1 = glm::vec3(
									attrib.vertices[idx1.vertex_index * 3 + 0],
									attrib.vertices[idx1.vertex_index * 3 + 1],
									attrib.vertices[idx1.vertex_index * 3 + 2]
								);

								glm::vec3 v2 = glm::vec3(
									attrib.vertices[idx2.vertex_index * 3 + 0],
									attrib.vertices[idx2.vertex_index * 3 + 1],
									attrib.vertices[idx2.vertex_index * 3 + 2]
								);

								newVertex.m_Normal = GenerateNormal(v0, v1, v2);
							}
							else if (idx.normal_index >= 0)
							{
								tinyobj::real_t nx = attrib.normals[size_t(idx.normal_index) * 3 + 0];
								tinyobj::real_t ny = attrib.normals[size_t(idx.normal_index) * 3 + 1];
								tinyobj::real_t nz = attrib.normals[size_t(idx.normal_index) * 3 + 2];

								newVertex.m_Normal = glm::vec3(nx, ny, nz);
							}

							// Check if `texcoord_index` is zero or positive. negative = no texcoord data
							if (idx.texcoord_index >= 0)
							{
								tinyobj::real_t tx = attrib.texcoords[size_t(idx.texcoord_index) * 2 + 0];
								tinyobj::real_t ty = attrib.texcoords[size_t(idx.texcoord_index) * 2 + 1];

								newVertex.m_TexCoord = glm::vec2(tx, 1.0f - ty);
							}

							newVertex.m_Color = glm::vec3(1.0f, 1.0f, 1.0f);

							if (uniqueVertices.count(newVertex) == 0) 
							{
								uniqueVertices[newVertex] = static_cast<uint32_t>(m_Vertices.size());
								m_Vertices.push_back(newVertex);
							}

							uint32_t vertexIndex = uniqueVertices[newVertex];
							m_Indices.push_back(vertexIndex);
						}
						verticesSoFar += 3;
					}
				}
				mesh.m_NumVertices = verticesSoFar - mesh.m_StartIndex;
				m_Meshes.push_back(mesh);
			}
		}

#if defined(_DEBUG)
		const uint32_t nVertices  = static_cast<uint32_t>(m_Vertices.size());
		const uint32_t nMeshes    = static_cast<uint32_t>(m_Meshes.size());
		const uint32_t nMaterials = static_cast<uint32_t>(m_Materials.size());

		const std::string loaded  = nVertices != 0 ? "loaded" : "";
		std::cout << "Vertex model [" << "path: " << modelFilepath.c_str() << ", vertices: " << nVertices << ", meshes: " << nMeshes << ", materials: " << nMaterials << "] " << loaded.c_str() << std::endl;
#endif

		return true;
	}

	void CModel::CreateVertexBuffer(CGraphicsContext* context)
	{
		VkDeviceSize bufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();

		/* CPU side Staging buffer */
		VkDeviceMemory stagingBufferMemory;

		VkBuffer stagingBuffer = CreateBuffer(
			context,
			stagingBufferMemory,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data;
		vkMapMemory(context->GetLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, m_Vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(context->GetLogicalDevice(), stagingBufferMemory);

		/* GPU Side */
		m_VertexBuffer = CreateBuffer(
			context,		
			m_VertexBufferMemory,
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		/* Copy staging buffer to device*/
		CopyBuffer(context, stagingBuffer, m_VertexBuffer, bufferSize);

		vkDestroyBuffer(context->GetLogicalDevice(), stagingBuffer, nullptr);
		vkFreeMemory(context->GetLogicalDevice(), stagingBufferMemory, nullptr);
	}

	void CModel::CreateIndexBuffer(CGraphicsContext* context)
	{
		VkDeviceSize bufferSize = sizeof(m_Indices[0]) * m_Indices.size();

		/* CPU side Staging buffer */
		VkDeviceMemory stagingBufferMemory;

		VkBuffer stagingBuffer = CreateBuffer(
			context,
			stagingBufferMemory,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);

		void* data;
		vkMapMemory(context->GetLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, m_Indices.data(), (size_t)bufferSize);
		vkUnmapMemory(context->GetLogicalDevice(), stagingBufferMemory);

		/* GPU Side Index buffer */
		m_IndexBuffer = CreateBuffer(
			context,
			m_IndexBufferMemory,
			bufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		/* Copy staging buffer to device*/
		CopyBuffer(context, stagingBuffer, m_IndexBuffer, bufferSize);

		vkDestroyBuffer(context->GetLogicalDevice(), stagingBuffer, nullptr);
		vkFreeMemory(context->GetLogicalDevice(), stagingBufferMemory, nullptr);
	}

	glm::vec3 CModel::GenerateNormal(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2)
	{
		glm::vec3 e0 = glm::normalize(v1 - v0);
		glm::vec3 e1 = glm::normalize(v2 - v0);
		glm::vec4 faceNormal = glm::vec4(cross(e0, e1), 1.0f);

		glm::vec3 generatedNormal = glm::normalize(glm::vec3((1.0f / faceNormal.w) * faceNormal));

		return generatedNormal;
	}

	void CModel::CreateDescriptorSets()
	{
		m_DescriptorSets = 
	}

	glm::mat4 CModel::GetTransform()
	{
		return m_Transform;
	}

	void CModel::SetTransform(glm::mat4 transform)
	{
		m_Transform = transform;
	}

	std::vector<SMesh> CModel::GetMeshes()
	{
		return m_Meshes;
	}
	
	SMaterial CModel::GetMaterial(uint32_t materialId)
	{
		return m_Materials[materialId];
	}

	uint32_t CModel::GetNumIndices()
	{
		return static_cast<uint32_t>(m_Indices.size());
	}

	void CModel::BindMesh(VkCommandBuffer commandBuffer, SMesh mesh)
	{
		VkBuffer vertexBuffers[] = { m_VertexBuffer };

		VkDeviceSize vertexOffsets[] = { 0 };
		VkDeviceSize indexOfssets    = 0;

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, vertexOffsets);
		vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer, indexOfssets, VK_INDEX_TYPE_UINT32);
	}

	void CModel::BindMesh(VkCommandBuffer commandBuffer)
	{
		VkBuffer vertexBuffers[] = { m_VertexBuffer };

		VkDeviceSize vertexOffsets[] = { 0 };
		VkDeviceSize indexOfssets = 0;

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, vertexOffsets);
		vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer, indexOfssets, VK_INDEX_TYPE_UINT32);
	}

	void CModel::CleanupModel(CGraphicsContext* context)
	{
		vkDestroyBuffer(context->GetLogicalDevice(), m_VertexBuffer, nullptr);
		vkFreeMemory(context->GetLogicalDevice(), m_VertexBufferMemory, nullptr);

		vkDestroyBuffer(context->GetLogicalDevice(), m_IndexBuffer, nullptr);
		vkFreeMemory(context->GetLogicalDevice(), m_IndexBufferMemory, nullptr);
	}
}