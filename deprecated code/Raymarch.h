#pragma once
#include "VulkanContext.h"
#include "VkPipeline.h"
#include "FrameBuffer.h"
#include "RenderQueue.h"

class Raymarcher {
  public:
	Raymarcher();
	~Raymarcher();

	void Init(const vk::Device& device, FrameBuffer& fbo, const vk::PhysicalDevice& physDev);
	void UpdateUniforms(VulkanCommandBuffer& cmdBuffer, const glm::mat4& viewProj, const glm::vec3& position, const glm::vec3& LightDir, const RenderQueue& queue, const glm::vec2& screenSize);
	void Render(VulkanCommandBuffer& cmdBuffer, uint32_t frameIndex, vk::DescriptorSet& ibl, glm::vec2 screenSize);
  private:
	Tephra::VkPipeline m_Pipeline;
	vk::Image m_TargetImage;
	vk::DescriptorPool m_DescPool;
	vk::DescriptorSet m_DescSets[BUFFER_COUNT];
	vk::ImageView m_DepthViews[BUFFER_COUNT];
	vk::Sampler m_DepthSampler;
	VkMemory m_BufferMem;
	VkAlloc m_UniformBuffer;
	VkAlloc m_PrimitiveBuffer;

	struct alignas(16) PerFrame {
		glm::mat4 invViewProj;
		glm::vec4 CamPos;
		glm::vec4 LightDir;
		glm::vec2 ScreenSize;
		uint32_t SphereCount;
		uint32_t BoxCount;
	};
};