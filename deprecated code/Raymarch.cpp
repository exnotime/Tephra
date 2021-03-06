#include "Raymarch.h"

Raymarcher::Raymarcher() {

}

Raymarcher::~Raymarcher() {

}

void Raymarcher::Init(const vk::Device& device,FrameBuffer& fbo, const vk::PhysicalDevice& physDev) {
	m_Pipeline.LoadPipelineFromFile(device, "shader/Raymarch.json", vk::Viewport(), nullptr);

	//build descriptor sets
	vk::DescriptorPoolCreateInfo descPoolInfo;
	descPoolInfo.maxSets = BUFFER_COUNT;
	vk::DescriptorPoolSize sizes[4];
	sizes[0].descriptorCount = BUFFER_COUNT;
	sizes[0].type = vk::DescriptorType::eStorageImage;
	sizes[1].descriptorCount = BUFFER_COUNT;
	sizes[1].type = vk::DescriptorType::eCombinedImageSampler;
	sizes[2].descriptorCount = BUFFER_COUNT;
	sizes[2].type = vk::DescriptorType::eUniformBuffer;
	sizes[3].descriptorCount = BUFFER_COUNT;
	sizes[3].type = vk::DescriptorType::eStorageBuffer;
	descPoolInfo.pPoolSizes = sizes;
	descPoolInfo.poolSizeCount = ARRAYSIZE(sizes);
	m_DescPool = device.createDescriptorPool(descPoolInfo);

	vk::DescriptorSetAllocateInfo descSetAllocInfo;
	descSetAllocInfo.descriptorPool = m_DescPool;
	descSetAllocInfo.descriptorSetCount = 1;
	descSetAllocInfo.pSetLayouts = m_Pipeline.GetDescriptorSetLayouts().data();
	//allocate twice because vulkan wont let me reuse the set layout othervise
	device.allocateDescriptorSets(&descSetAllocInfo, &m_DescSets[0]);
	device.allocateDescriptorSets(&descSetAllocInfo, &m_DescSets[1]);
	for (int i = 0; i < BUFFER_COUNT; i++) {
		vk::ImageViewCreateInfo dsiViewInfo;
		dsiViewInfo.components = { vk::ComponentSwizzle::eR,vk::ComponentSwizzle::eG,vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA };
		dsiViewInfo.format = vk::Format::eD24UnormS8Uint;
		dsiViewInfo.image = fbo.GetImage(1, i);
		dsiViewInfo.viewType = vk::ImageViewType::e2D;
		dsiViewInfo.subresourceRange = { vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 };
		m_DepthViews[i] = device.createImageView(dsiViewInfo);
	}
	vk::SamplerCreateInfo sampInfo;
	sampInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	sampInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	sampInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	sampInfo.anisotropyEnable = false;
	sampInfo.maxAnisotropy = 1.0f;
	sampInfo.borderColor = vk::BorderColor::eFloatOpaqueBlack;
	sampInfo.compareEnable = false;
	sampInfo.compareOp = vk::CompareOp::eNever;
	sampInfo.magFilter = vk::Filter::eNearest;
	sampInfo.minFilter = vk::Filter::eNearest;
	sampInfo.maxLod = 1.0f;
	sampInfo.minLod = 0.0f;
	sampInfo.mipLodBias = 0.0f;
	sampInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
	sampInfo.unnormalizedCoordinates = true;
	m_DepthSampler = device.createSampler(sampInfo);

	std::array<vk::ImageView, BUFFER_COUNT> imageViews;
	for (int i = 0; i < BUFFER_COUNT; i++) {
		imageViews[i] = fbo.GetViews()[i * fbo.GetFormats().size()];
	}

	//uniform buffer
	m_BufferMem.Init(device, physDev, 4 * MEGA_BYTE, 4 * MEGA_BYTE);
	m_UniformBuffer = m_BufferMem.AllocateBuffer(sizeof(PerFrame), vk::BufferUsageFlagBits::eUniformBuffer, nullptr);
	m_PrimitiveBuffer = m_BufferMem.AllocateBuffer((sizeof(SDFSphere) + sizeof(SDFBox) + sizeof(glm::vec4)) * 128, vk::BufferUsageFlagBits::eStorageBuffer, nullptr);

	for (int i = 0; i < BUFFER_COUNT; i++) {
		std::array<vk::WriteDescriptorSet,4> writeSet;
		//framebuffer
		writeSet[0].descriptorCount = 1;
		writeSet[0].descriptorType = vk::DescriptorType::eStorageImage;
		writeSet[0].dstArrayElement = 0;
		writeSet[0].dstBinding = 0;
		writeSet[0].dstSet = m_DescSets[i];
		vk::DescriptorImageInfo imageInfo;
		imageInfo.imageLayout = vk::ImageLayout::eGeneral;
		imageInfo.imageView = fbo.GetView(0, i);
		writeSet[0].pImageInfo = &imageInfo;
		//depth stencil image
		writeSet[1].descriptorCount = 1;
		writeSet[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
		writeSet[1].dstArrayElement = 0;
		writeSet[1].dstBinding = 1;
		writeSet[1].dstSet = m_DescSets[i];
		vk::DescriptorImageInfo imageInfo2;
		imageInfo2.imageLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
		imageInfo2.imageView = m_DepthViews[i];
		imageInfo2.sampler = m_DepthSampler;
		writeSet[1].pImageInfo = &imageInfo2;
		//frame data
		writeSet[2].descriptorCount = 1;
		writeSet[2].descriptorType = vk::DescriptorType::eUniformBuffer;
		writeSet[2].dstArrayElement = 0;
		writeSet[2].dstBinding = 2;
		writeSet[2].dstSet = m_DescSets[i];
		vk::DescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = m_UniformBuffer.BufferHandle;
		bufferInfo.offset = 0;
		bufferInfo.range = VK_WHOLE_SIZE;
		writeSet[2].pBufferInfo = &bufferInfo;
		//primitives
		writeSet[3].descriptorCount = 1;
		writeSet[3].descriptorType = vk::DescriptorType::eStorageBuffer;
		writeSet[3].dstArrayElement = 0;
		writeSet[3].dstBinding = 3;
		writeSet[3].dstSet = m_DescSets[i];
		vk::DescriptorBufferInfo bufferInfo2;
		bufferInfo2.buffer = m_PrimitiveBuffer.BufferHandle;
		bufferInfo2.offset = 0;
		bufferInfo2.range = VK_WHOLE_SIZE;
		writeSet[3].pBufferInfo = &bufferInfo2;

		device.updateDescriptorSets(writeSet, nullptr);
	}
}

void Raymarcher::UpdateUniforms(VulkanCommandBuffer& cmdBuffer, const glm::mat4& viewProj, const glm::vec3& position, const glm::vec3& LightDir, const RenderQueue& queue, const glm::vec2& screenSize) {
	PerFrame frameInfo;
	frameInfo.CamPos = glm::vec4(position,1);
	frameInfo.invViewProj = glm::inverse(viewProj);
	frameInfo.LightDir = glm::vec4(LightDir, 0);
	frameInfo.ScreenSize = screenSize;
	//frameInfo.SphereCount = (uint32_t)queue.GetSpheres().size();
	//frameInfo.BoxCount = (uint32_t)queue.GetBoxes().size();
	//m_BufferMem.UpdateBuffer(m_UniformBuffer, sizeof(PerFrame), &frameInfo);

	//m_BufferMem.UpdateBuffer(m_PrimitiveBuffer, sizeof(SDFSphere) * frameInfo.SphereCount, (void*)queue.GetSpheres().data());
	//m_BufferMem.UpdateBuffer(m_PrimitiveBuffer, sizeof(SDFSphere) * 128, sizeof(SDFBox) * frameInfo.BoxCount, (void*)queue.GetBoxes().data());
	//m_BufferMem.UpdateBuffer(m_PrimitiveBuffer, (sizeof(SDFSphere) + sizeof(SDFBox)) * 128, sizeof(glm::vec4) * queue.GetSDFColors().size(), (void*)queue.GetSDFColors().data());
	m_BufferMem.ScheduleTransfers(cmdBuffer);
}

void Raymarcher::Render(VulkanCommandBuffer& cmdBuffer, uint32_t frameIndex, vk::DescriptorSet& ibl, glm::vec2 screenSize) {
	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, m_Pipeline.GetPipeline());
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_Pipeline.GetPipelineLayout(), 0, m_DescSets[frameIndex], nullptr);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_Pipeline.GetPipelineLayout(), 1, ibl, nullptr);
	const uint32_t gs = 32;
	uint32_t x = ((uint32_t)screenSize.x + gs - 1) / gs;
	uint32_t y = ((uint32_t)screenSize.y + gs - 1) / gs;
	cmdBuffer.dispatch(x, y, 1);
}