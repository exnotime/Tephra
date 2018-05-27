#include "FrameBuffer.h"

using namespace smug;

FrameBuffer::FrameBuffer() {

}

FrameBuffer::~FrameBuffer() {

}

bool IsDepthStencil(vk::Format f) {
	switch (f)
	{
	case vk::Format::eD16Unorm:
	case vk::Format::eX8D24UnormPack32:
	case vk::Format::eD32Sfloat:
	case vk::Format::eD16UnormS8Uint:
	case vk::Format::eD24UnormS8Uint:
	case vk::Format::eD32SfloatS8Uint:
		return true;
		break;
	}
	return false;
}

void FrameBuffer::Init(const vk::Device& device, const vk::PhysicalDevice& gpu, const glm::vec2& size, const Vector<vk::Format>& formats, const Vector<vk::ImageUsageFlags>& usages, uint32_t bufferCount) {
	m_FrameBufferSize = size;
	m_FormatCount = (uint32_t)formats.size();
	//init image and views
	for (int i = 0; i < bufferCount; i++) {
		for (uint32_t f = 0; f < m_FormatCount; ++f) {
			vk::ImageCreateInfo imageInfo;
			imageInfo.arrayLayers = 1;
			imageInfo.extent = { (uint32_t)size.x, (uint32_t)size.y, 1 };
			imageInfo.format = formats[f];
			imageInfo.imageType = vk::ImageType::e2D;
			imageInfo.initialLayout = vk::ImageLayout::eUndefined;
			imageInfo.mipLevels = 1;
			imageInfo.samples = vk::SampleCountFlagBits::e1; //dont support msaa for now
			imageInfo.sharingMode = vk::SharingMode::eExclusive;
			imageInfo.tiling = vk::ImageTiling::eOptimal;
			imageInfo.usage = usages[f];
			m_Images.push_back(device.createImage(imageInfo));
			m_CurrentLayouts.push_back(vk::ImageLayout::eUndefined);
		}
	}

	//init memory
	uint64_t memSize = 0;
	uint32_t memBits = 0;
	uint32_t imageCount = m_Images.size();
	for (uint32_t i = 0; i < imageCount; ++i) {
		auto memReq = device.getImageMemoryRequirements(m_Images[i]);
		memBits |= memReq.memoryTypeBits;
		memSize = (memSize + (memReq.alignment - 1)) & ~(memReq.alignment - 1); //handle alignment spill
		memSize += memReq.size;
	}
	vk::MemoryPropertyFlagBits deviceMemFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
	auto& memProps = gpu.getMemoryProperties();
	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
		if ((memProps.memoryTypes[i].propertyFlags & deviceMemFlags) == deviceMemFlags && memBits & (1 << i)) {
			vk::MemoryAllocateInfo allocInfo;
			allocInfo.allocationSize = memSize;
			allocInfo.memoryTypeIndex = i;
			m_Memory = device.allocateMemory(allocInfo);
			m_MemorySize = memSize;
			break;
		}
	}

	uint32_t image = 0;
	uint64_t memOffset = 0;
	for (int i = 0; i < bufferCount; i++) {
		for (uint32_t f = 0; f < m_FormatCount; ++f) {
			auto memReq = device.getImageMemoryRequirements(m_Images[image]);
			memOffset = (memOffset + (memReq.alignment - 1)) & ~(memReq.alignment - 1); //handle alignment
			device.bindImageMemory(m_Images[image], m_Memory, memOffset);
			memOffset += memReq.size;

			vk::ImageViewCreateInfo imageViewInfo;
			imageViewInfo.components = { vk::ComponentSwizzle::eR,vk::ComponentSwizzle::eG,vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA };
			imageViewInfo.format = formats[f];
			imageViewInfo.image = m_Images[image];
			imageViewInfo.viewType = vk::ImageViewType::e2D;
			imageViewInfo.subresourceRange.aspectMask = IsDepthStencil(formats[f]) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor; //for now assume that the stencil is not gonna be read in a later pass
			imageViewInfo.subresourceRange.baseArrayLayer = 0;
			imageViewInfo.subresourceRange.baseMipLevel = 0;
			imageViewInfo.subresourceRange.layerCount = 1;
			imageViewInfo.subresourceRange.levelCount = 1;

			m_ImageViews.push_back(device.createImageView(imageViewInfo));
			image++;
		}
	}

	//init renderpass
	vk::RenderPassCreateInfo renderPassInfo;
	renderPassInfo.attachmentCount = (uint32_t)formats.size();
	renderPassInfo.dependencyCount = 0; //future optimization, get dependency off another framebuffer
	std::vector<vk::AttachmentDescription> attachments;
	for (uint32_t f = 0; f < m_FormatCount; ++f) {
		vk::AttachmentDescription attachmentDesc;
		attachmentDesc.format = formats[f];
		vk::ImageLayout layout = IsDepthStencil(formats[f]) ? vk::ImageLayout::eDepthStencilAttachmentOptimal : vk::ImageLayout::eColorAttachmentOptimal;
		attachmentDesc.finalLayout = layout;
		attachmentDesc.initialLayout = layout;
		attachmentDesc.loadOp = vk::AttachmentLoadOp::eClear;
		attachmentDesc.storeOp = vk::AttachmentStoreOp::eStore;
		attachmentDesc.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		attachmentDesc.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		attachmentDesc.samples = vk::SampleCountFlagBits::e1; //dont support msaa for now
		attachments.push_back(attachmentDesc);
	}
	renderPassInfo.attachmentCount = (uint32_t)attachments.size();
	renderPassInfo.pAttachments = attachments.data();
	//init subpass
	vk::SubpassDescription subPassDesc;
	std::vector<vk::AttachmentReference> colorAttachments;
	std::vector<vk::AttachmentReference> depthAttachments;
	for (uint32_t i = 0; i < attachments.size(); ++i) {
		if (attachments[i].initialLayout == vk::ImageLayout::eColorAttachmentOptimal) {
			colorAttachments.push_back({ i, vk::ImageLayout::eColorAttachmentOptimal });
		}
		else {
			depthAttachments.push_back({ i, vk::ImageLayout::eDepthStencilAttachmentOptimal });
		}
	}
	assert(depthAttachments.size() <= 1);
	subPassDesc.colorAttachmentCount = (uint32_t)colorAttachments.size();
	subPassDesc.pColorAttachments = colorAttachments.data();
	subPassDesc.pDepthStencilAttachment = depthAttachments.data();
	subPassDesc.inputAttachmentCount = 0;
	subPassDesc.pInputAttachments = nullptr;
	subPassDesc.pPreserveAttachments = nullptr;
	subPassDesc.pResolveAttachments = nullptr;
	subPassDesc.preserveAttachmentCount = 0;
	subPassDesc.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subPassDesc;

	m_RenderPass = device.createRenderPass(renderPassInfo);

	//init framebuffer
	vk::FramebufferCreateInfo framebufferInfo;
	framebufferInfo.attachmentCount = (uint32_t)formats.size();
	framebufferInfo.height = size.y;
	framebufferInfo.width = size.x;
	framebufferInfo.layers = 1;
	framebufferInfo.renderPass = m_RenderPass;
	for (int i = 0; i < bufferCount; i++) {
		framebufferInfo.pAttachments = &m_ImageViews[i * framebufferInfo.attachmentCount];
		m_FrameBuffers[i] = device.createFramebuffer(framebufferInfo);
	}

	//Init descriptors
	for (int i = 0; i < m_FormatCount; ++i){
		vk::DescriptorImageInfo imageDescInfo = {};
		if (usages[i] & vk::ImageUsageFlagBits::eSampled) {
			imageDescInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			imageDescInfo.imageView = m_ImageViews[i];

			vk::SamplerCreateInfo samplerInfo = {};
			samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
			samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
			samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
			samplerInfo.anisotropyEnable = false;
			samplerInfo.magFilter = vk::Filter::eNearest;
			samplerInfo.minFilter = vk::Filter::eNearest;
			samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
			if (IsDepthStencil(m_Formats[i])) {
				//used for shadow maps
				samplerInfo.compareEnable = true;
				samplerInfo.compareOp = vk::CompareOp::eLessOrEqual;
				samplerInfo.magFilter = vk::Filter::eLinear;
				samplerInfo.minFilter = vk::Filter::eLinear;
				samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
				samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
				samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
			} else {
				samplerInfo.compareEnable = false;
			}
			imageDescInfo.sampler = device.createSampler(samplerInfo);
		}
		m_Descriptors.push_back(imageDescInfo);
	}

	m_Formats += formats;

	m_BufferCount = bufferCount;
}

void FrameBuffer::Resize(const glm::vec2& size) {
	//TODO
}

///Push barrier needs to be called forthis to make an effect
void FrameBuffer::ChangeLayout(CommandBuffer& cmdBuffer, const Vector<vk::ImageLayout>& newLayouts, uint32_t frameIndex) {
	
	uint32_t imageCount = (uint32_t)m_Formats.size();
	assert(imageCount == newLayouts.size());
	assert(frameIndex < m_BufferCount);

	for (uint32_t i = 0; i < imageCount; ++i) {
		//if (m_CurrentLayouts[frameIndex * imageCount + i] == newLayouts[i]) {
		//	continue;
		//}
		cmdBuffer.ImageBarrier(m_Images[frameIndex * imageCount + i], m_CurrentLayouts[frameIndex * imageCount + i], newLayouts[i]);
		m_CurrentLayouts[frameIndex * imageCount + i] = newLayouts[i];
	}
}

void FrameBuffer::SetLayouts(const std::vector<vk::ImageLayout>& newLayouts, uint32_t frameIndex) {
	uint32_t size = (uint32_t)newLayouts.size();
	uint32_t begin = m_FormatCount * frameIndex;
	for (uint32_t i = begin; i < size; ++i) {
		m_CurrentLayouts[begin + i] = newLayouts[i];
	}
}