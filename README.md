![alt text](engine.png)
# MyVulkanPlayground
This repository contains the code and assets for my hobby graphics engine where I experiment with implementing modern rendering techniques and systems. It's written in C++ and uses Vulkan for the graphics API and GLSL for shaders.

# Features
In this project I have implemented several systems meant for simplifying interaction with the [Graphics Pipeline](https://en.wikipedia.org/wiki/Graphics_pipeline) and exposed them to a base class called CDrawPass. 

There are managers for handling keyboard/mouse inputs, resource state tracking, pipeline helpers & shader binding tables as well as a rudimentary model rendering system. The base class is overridable and has two functions: InitPass and DrawPass. InitPass is called once on startup while DrawPass runs once per frame. 

You can have a look under [source/VulkanEngine/DrawPasses/](https://github.com/hjelmw/MyVulkanPlayground/tree/main/source/VulkanEngine/DrawPasses
) to see what rendering techniques have currently been implemented. Some sample code can be found below

```c++
void CAtmosphericsPass::InitPass(CGraphicsContext* context, SGraphicsManagers* managers)
{
	const SRenderAttachment atmosphericsAttachment  = managers->m_AttachmentManager->GetAttachment(EAttachmentIndices::AtmosphericsSkyBox);
	const SRenderAttachment depthAttachment         = managers->m_AttachmentManager->GetAttachment(EAttachmentIndices::Depth);

	m_AtmosphericsUniformBuffer = CreateUniformBuffer(context, m_AtmosphericsBufferMemory, sizeof(SAtmosphericsFragmentConstants));

	m_AtmosphericsTable = new CBindingTable();
	m_AtmosphericsTable->AddSampledImageBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT, depthAttachment.m_ImageView, depthAttachment.m_Format, context->GetLinearClampSampler());
	m_AtmosphericsTable->AddUniformBufferBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT, m_AtmosphericsUniformBuffer, sizeof(SAtmosphericsFragmentConstants));
	m_AtmosphericsTable->CreateBindings(context);

	m_AtmosphericsPipeline = new CPipeline();
	m_AtmosphericsPipeline->SetVertexShader("shaders/atmospherics.vert.spv");
	m_AtmosphericsPipeline->SetFragmentShader("shaders/atmospherics.frag.spv");
	m_AtmosphericsPipeline->SetCullingMode(VK_CULL_MODE_FRONT_BIT);
	m_AtmosphericsPipeline->AddColorAttachment(atmosphericsAttachment.m_Format);
	m_AtmosphericsPipeline->AddDepthAttachment(depthAttachment.m_Format);
	m_AtmosphericsPipeline->AddPushConstantSlot(VK_SHADER_STAGE_VERTEX_BIT, sizeof(SAtmosphericsVertexPushConstants), 0);
	m_AtmosphericsPipeline->CreatePipeline(context, m_AtmosphericsTable->GetDescriptorSetLayout());
}
```

```c++
 void CAtmosphericsPass::DrawPass(CGraphicsContext* context, SGraphicsManagers* managers, VkCommandBuffer commandBuffer)
{
	CAttachmentManager* attachmentManager = managers->m_AttachmentManager;

	attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::Depth, VK_ATTACHMENT_LOAD_OP_LOAD, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL);
	SRenderAttachment atmosphericsAttachment = attachmentManager->TransitionAttachment(commandBuffer, EAttachmentIndices::AtmosphericsSkyBox, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	SAtmosphericsVertexPushConstants vertexPushConstants = UpdateAtmosphericsConstants();

	std::vector<SRenderAttachment> inscatteringAttachments = { atmosphericsAttachment };
	BeginRendering(context, commandBuffer, inscatteringAttachments);

	m_AtmosphericsTable->BindTable(context, commandBuffer, m_AtmosphericsPipeline->GetPipelineLayout());
	m_AtmosphericsPipeline->BindPipeline(commandBuffer);
	m_AtmosphericsPipeline->PushConstants(commandBuffer, (void*)&vertexPushConstants);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	EndRendering(commandBuffer);
}
```


# Setup
TODO: Make the dependencies into subrepos and add a build script

## Dependencies
* Vulkan SDK
  * Extensions: `VK_KHR_SWAPCHAIN`, `VK_KHR_DYNAMIC_RENDERING` (gets rid of renderpasses), `VK_EXT_DESCRIPTOR_INDEXING`
* GLFW-3.4
* GLM-0.9.3.4
* ImGUI
* TinyOBJLoader-2.0
* stbi_image

