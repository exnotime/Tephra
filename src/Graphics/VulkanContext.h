#pragma once
#include <vulkan/vulkan.hpp>

#define BUFFER_COUNT 2

struct VulkanContext {
	vk::Instance Instance;
	vk::Device Device;
	vk::PhysicalDevice PhysicalDevice;
	uint32_t FrameIndex;
};

struct VulkanSwapChain {
	vk::SurfaceKHR Surface;
	vk::SwapchainKHR SwapChain;
	vk::Image Images[BUFFER_COUNT];
	vk::Image ResolveImages[BUFFER_COUNT];
	vk::ImageView ImageViews[BUFFER_COUNT];
	vk::Framebuffer FrameBuffers[BUFFER_COUNT];
	vk::Image DepthStencilImages[BUFFER_COUNT];
	vk::Image DepthResolveImages[BUFFER_COUNT];
	vk::ImageView DepthStencilImageViews[BUFFER_COUNT];

	bool MSAA;
	vk::SampleCountFlags SampleCount;
	vk::Format Format;
};

class VulkanCommandBuffer : public vk::CommandBuffer {
public:
	VulkanCommandBuffer() {

	}
	~VulkanCommandBuffer() {

	}
	void Init(vk::Device device, int queueFamilyIndex) {
		vk::CommandPoolCreateInfo poolInfo;
		poolInfo.queueFamilyIndex = queueFamilyIndex;
		poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
		m_CmdPools[0] = device.createCommandPool(poolInfo);
		m_CmdPools[1] = device.createCommandPool(poolInfo);

		vk::CommandBufferAllocateInfo bufferInfo;
		bufferInfo.commandBufferCount = 1;
		bufferInfo.commandPool = m_CmdPools[0];
		bufferInfo.level = vk::CommandBufferLevel::ePrimary;
		*static_cast<vk::CommandBuffer*>(this) = device.allocateCommandBuffers(bufferInfo)[0];
	}

	void Begin(const vk::Framebuffer& frameBuffer, const vk::RenderPass& renderPass) {
		vk::CommandBufferBeginInfo beginInfo;
		vk::CommandBufferInheritanceInfo inheritanceInfo;
		inheritanceInfo.framebuffer = frameBuffer;
		inheritanceInfo.renderPass = renderPass;
		beginInfo.pInheritanceInfo = &inheritanceInfo;
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;

		this->begin(&beginInfo);
	}

	void ImageBarrier(const vk::Image& img, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
		vk::ImageMemoryBarrier imgBarrier;
		imgBarrier.image = img;
		imgBarrier.oldLayout = oldLayout;
		imgBarrier.newLayout = newLayout;
		imgBarrier.srcAccessMask = LayoutToAccessMask(oldLayout);
		imgBarrier.dstAccessMask = LayoutToAccessMask(newLayout);
		imgBarrier.subresourceRange.aspectMask = LayoutToAspectMask(newLayout);
		imgBarrier.subresourceRange.baseArrayLayer = 0;
		imgBarrier.subresourceRange.baseMipLevel = 0;
		imgBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		imgBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		m_ImgBarriers.push_back(imgBarrier);
	}

	void PushPipelineBarrier() {
		this->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe, vk::DependencyFlagBits::eByRegion,
			0, nullptr, 0, nullptr, m_ImgBarriers.size(), m_ImgBarriers.data());
		m_ImgBarriers.clear();
		//TODO add memory and buffer barriers
	}
	void Reset(vk::Device device, int frameIndex){
		device.resetCommandPool(m_CmdPools[frameIndex], vk::CommandPoolResetFlagBits::eReleaseResources);
	}
private:
	vk::AccessFlags LayoutToAccessMask(vk::ImageLayout layout) {
		switch (layout) {
		case vk::ImageLayout::eColorAttachmentOptimal:
			return vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
			break;
		case vk::ImageLayout::eDepthStencilAttachmentOptimal:
			return vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			break;
		case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
			return vk::AccessFlagBits::eShaderRead;
			break;
		case vk::ImageLayout::ePresentSrcKHR:
			return vk::AccessFlagBits::eMemoryRead;
			break;
		case vk::ImageLayout::eShaderReadOnlyOptimal:
			return vk::AccessFlagBits::eShaderRead;
			break;
		case vk::ImageLayout::eTransferDstOptimal:
			return vk::AccessFlagBits::eTransferWrite;
			break;
		case vk::ImageLayout::eTransferSrcOptimal:
			return vk::AccessFlagBits::eTransferRead;
			break;
		case vk::ImageLayout::eGeneral:
			return vk::AccessFlagBits::eShaderWrite;
			break;
		case vk::ImageLayout::ePreinitialized:
			return vk::AccessFlags();
			break;
		case vk::ImageLayout::eUndefined:
			return vk::AccessFlags();
			break;
		}
	}
	vk::ImageAspectFlags LayoutToAspectMask(vk::ImageLayout layout) {
		if (layout == vk::ImageLayout::eColorAttachmentOptimal || layout == vk::ImageLayout::eShaderReadOnlyOptimal ||
			layout == vk::ImageLayout::eGeneral || layout == vk::ImageLayout::ePresentSrcKHR || layout == vk::ImageLayout::eTransferDstOptimal ||
			layout == vk::ImageLayout::eTransferSrcOptimal) {
			return vk::ImageAspectFlagBits::eColor;
		}
		else if (layout == vk::ImageLayout::eDepthStencilAttachmentOptimal || layout == vk::ImageLayout::eDepthStencilReadOnlyOptimal) {
			return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
		}
		else {
			return vk::ImageAspectFlagBits::eMetadata;
		}
	}

	vk::CommandPool m_CmdPools[BUFFER_COUNT];
	std::vector<vk::ImageMemoryBarrier> m_ImgBarriers;
};

class VulkanQueue : public vk::Queue {
public:
	VulkanQueue() {

	}
	~VulkanQueue() {

	}

	void Init(const vk::PhysicalDevice& physDevice, vk::QueueFlagBits type) {
		//find queue index for type
		m_QueueIndex = 0;
		for (auto& queueProps : physDevice.getQueueFamilyProperties()) {
			if (queueProps.queueFlags & type) {
				break;
			}
			m_QueueIndex++;
		}
		m_QueueInfo.queueCount = 1;
		m_QueueInfo.queueFamilyIndex = m_QueueIndex;
		m_QueuePrio = 0.0f;
		m_QueueInfo.pQueuePriorities = &m_QueuePrio;
	}

	void Submit(const std::vector<vk::CommandBuffer>& cmdBuffers, const std::vector<vk::Semaphore> waitSemaphores,
		const std::vector<vk::Semaphore> signalSemaphores, vk::Fence fence) {
		vk::SubmitInfo submit;
		submit.commandBufferCount = cmdBuffers.size();
		submit.pCommandBuffers = cmdBuffers.data();
		submit.signalSemaphoreCount = signalSemaphores.size();
		submit.pSignalSemaphores = signalSemaphores.data();
		submit.waitSemaphoreCount = waitSemaphores.size();
		submit.pWaitSemaphores = waitSemaphores.data();
		vk::PipelineStageFlags flags = vk::PipelineStageFlagBits::eBottomOfPipe;
		submit.pWaitDstStageMask = &flags;

		this->submit(1, &submit, fence);
	}

	void Submit(const vk::CommandBuffer& cmdBuffer, const vk::Semaphore& waitSemaphore,
		const vk::Semaphore& signalSemaphore, const vk::Fence fence) {
		vk::SubmitInfo submit;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmdBuffer;
		submit.signalSemaphoreCount = (signalSemaphore) ? 1 : 0;
		submit.pSignalSemaphores = &signalSemaphore;
		submit.waitSemaphoreCount = (waitSemaphore) ? 1 : 0;
		submit.pWaitSemaphores = &waitSemaphore;
		vk::PipelineStageFlags flags = vk::PipelineStageFlagBits::eBottomOfPipe;
		submit.pWaitDstStageMask = &flags;

		this->submit(1, &submit, fence);
	}

	void Submit(const vk::CommandBuffer cmdBuffer) {
		vk::SubmitInfo submit;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmdBuffer;
		vk::PipelineStageFlags flags = vk::PipelineStageFlagBits::eBottomOfPipe;
		submit.pWaitDstStageMask = &flags;

		this->submit(1, &submit, nullptr);
	}

	vk::DeviceQueueCreateInfo& GetInfo() { return m_QueueInfo; }
	int GetQueueIndex() { return m_QueueIndex; }

private:
	int m_QueueIndex;
	float m_QueuePrio;
	vk::DeviceQueueCreateInfo m_QueueInfo;

};