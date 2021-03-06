#include "VkMemory.h"

VkMemory::VkMemory() {

}

VkMemory::~VkMemory() {

}

void VkMemory::Init(const vk::Device& device, const vk::PhysicalDevice& physDev, uint64_t deviceSize, uint64_t stageSize, uint32_t memTypeBits) {
	m_MemProps = physDev.getMemoryProperties();
	m_Device = device;

	//allocate device memory
	vk::BufferCreateInfo devBufferInfo;
	devBufferInfo.sharingMode = vk::SharingMode::eExclusive;
	devBufferInfo.size = deviceSize;
	devBufferInfo.usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
	                      vk::BufferUsageFlagBits::eStorageTexelBuffer | vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eVertexBuffer |
	                      vk::BufferUsageFlagBits::eUniformTexelBuffer;
	m_DeviceBuffer = device.createBuffer(devBufferInfo);
	vk::MemoryRequirements memReq = device.getBufferMemoryRequirements(m_DeviceBuffer);

	vk::MemoryPropertyFlagBits deviceMemFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
	if (memTypeBits != 0)
		memReq.memoryTypeBits = memTypeBits;
	for (uint32_t i = 0; i < m_MemProps.memoryTypeCount; i++) {
		if ((m_MemProps.memoryTypes[i].propertyFlags & deviceMemFlags) == deviceMemFlags && memReq.memoryTypeBits & (1 << i)) {
			vk::MemoryAllocateInfo allocInfo;
			allocInfo.allocationSize = memReq.size;
			allocInfo.memoryTypeIndex = i;
			m_DevMem = device.allocateMemory(allocInfo);
			m_DeviceSize = memReq.size;
			break;
		}
	}
	device.bindBufferMemory(m_DeviceBuffer, m_DevMem, 0);
	if (stageSize > 0) {
		vk::BufferCreateInfo stageBufferInfo;
		stageBufferInfo.sharingMode = vk::SharingMode::eExclusive;
		stageBufferInfo.size = stageSize;
		stageBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
		m_StagingBuffer = device.createBuffer(stageBufferInfo);
		vk::MemoryRequirements stageMemReq = device.getBufferMemoryRequirements(m_StagingBuffer);
		vk::MemoryPropertyFlags stagingMemFlags = vk::MemoryPropertyFlagBits::eHostVisible;

		for (uint32_t i = 0; i < m_MemProps.memoryTypeCount; i++) {
			if ((m_MemProps.memoryTypes[i].propertyFlags & stagingMemFlags) == stagingMemFlags &&
				stageMemReq.memoryTypeBits & (1 << i)) {
				vk::MemoryAllocateInfo allocInfo;
				allocInfo.allocationSize = stageSize;
				allocInfo.memoryTypeIndex = i;
				m_StagingMem = device.allocateMemory(allocInfo);
				m_StagingSize = stageSize;
				break;
			}
		}
		device.bindBufferMemory(m_StagingBuffer, m_StagingMem, 0);
	}
	m_DeviceOffset = 0;
	m_StagingOffset = 0;
}

VkAlloc VkMemory::AllocateBuffer(uint64_t size, vk::BufferUsageFlags usage, void* data) {
	if (m_DeviceOffset + size > m_DeviceSize) {
		printf("Failed allocation of size %d\n Total memory in buffer %d, Used %d\n", (uint32_t)size, (uint32_t)m_DeviceSize, (uint32_t)m_DeviceOffset);
		return VkAlloc();
	}

	vk::BufferCreateInfo bufferInfo;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	vk::Buffer buffer = m_Device.createBuffer(bufferInfo);

	auto memReq = m_Device.getBufferMemoryRequirements(buffer);
	m_DeviceOffset = (m_DeviceOffset + memReq.alignment - 1) & ~(memReq.alignment - 1);
	//bind buffer to memory
	m_Device.bindBufferMemory(buffer, m_DevMem, m_DeviceOffset);

	//if there is data allocate space in staging buffer
	if (data && m_StagingOffset + size < m_StagingSize) {
		//transfer data to staging buffer
		void* bufferPtr = m_Device.mapMemory(m_StagingMem, m_StagingOffset, size);
		memcpy(bufferPtr, data, size);
		vk::MappedMemoryRange range;
		range.memory = m_StagingMem;
		range.offset = m_StagingOffset;
		range.size = size;
		m_Device.flushMappedMemoryRanges(1, &range);
		m_Device.unmapMemory(m_StagingMem);

		vk::BufferCopy copy;
		copy.dstOffset = m_DeviceOffset;
		copy.srcOffset = m_StagingOffset;
		copy.size = size;
		m_Transfers.push_back(copy);
		m_StagingOffset += size;
	}

	VkAlloc buf;
	buf.BufferHandle = buffer;
	buf.Offset = m_DeviceOffset;
	buf.Size = size;

	m_DeviceOffset += size;

	return buf;
}

VkAlloc VkMemory::AllocateImage(vk::Image img, gli::texture2d* texture, void* data) {
	vk::MemoryRequirements memReq = m_Device.getImageMemoryRequirements(img);
	m_DeviceOffset = (m_DeviceOffset + (memReq.alignment - 1)) & ~(memReq.alignment - 1);

	if (m_DeviceOffset + memReq.size > m_DeviceSize) {
		printf("Failed allocation of size %d\nTotal memory in buffer %d, Used %d\n", (uint32_t)memReq.size, (uint32_t)m_DeviceSize, (uint32_t)m_DeviceOffset);
		return VkAlloc();
	}
	VkDeviceSize size = (texture) ? texture->size() : memReq.size;
	m_Device.bindImageMemory(img, m_DevMem, m_DeviceOffset);

	if (data && m_StagingOffset + size < m_StagingSize) {
		//transfer data to staging buffer
		void* bufferPtr = m_Device.mapMemory(m_StagingMem, m_StagingOffset, size);
		memcpy(bufferPtr, data, size);
		vk::MappedMemoryRange range;
		range.memory = m_StagingMem;
		range.offset = m_StagingOffset;
		range.size = size;
		m_Device.flushMappedMemoryRanges(1, &range);
		m_Device.unmapMemory(m_StagingMem);

		if (texture) {
			TextureTransfer transfer;
			transfer.Image = img;
			for (int i = 0; i < texture->levels(); i++) {
				vk::BufferImageCopy copy;
				gli::image mip = (*texture)[i];
				copy.bufferOffset = m_StagingOffset;
				copy.imageExtent = vk::Extent3D(mip.extent().x, mip.extent().y, 1);
				copy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
				copy.imageSubresource.baseArrayLayer = 0;
				copy.imageSubresource.layerCount = 1;
				copy.imageSubresource.mipLevel = i;
				transfer.copies.push_back(copy);
				m_StagingOffset += mip.size();
			}
			m_ImageTransfers.push_back(transfer);
		} else {
			vk::BufferCopy copy;
			copy.dstOffset = m_DeviceOffset;
			copy.srcOffset = m_StagingOffset;
			copy.size = memReq.size;
			m_Transfers.push_back(copy);
			m_StagingOffset += memReq.size;
		}

	}

	VkAlloc alloc;
	alloc.Offset = m_DeviceOffset;
	alloc.Size = memReq.size;

	m_DeviceOffset += memReq.size;

	return alloc;
}

VkAlloc VkMemory::AllocateImage(vk::Image img, const TextureInfo& texInfo) {
	vk::MemoryRequirements memReq = m_Device.getImageMemoryRequirements(img);
	m_DeviceOffset = (m_DeviceOffset + memReq.alignment - 1) & ~(memReq.alignment - 1);

	if (m_DeviceOffset + memReq.size > m_DeviceSize) {
		printf("Failed allocation of size %d\n Total memory in buffer %d, Used %d\n", (uint32_t)memReq.size, (uint32_t)m_DeviceSize, (uint32_t)m_DeviceOffset);
		return VkAlloc();
	}
	VkDeviceSize size = (texInfo.Data) ? texInfo.LinearSize : memReq.size;
	m_Device.bindImageMemory(img, m_DevMem, m_DeviceOffset);

	if (texInfo.Data && m_StagingOffset + size < m_StagingSize) {
		//transfer data to staging buffer
		void* bufferPtr = m_Device.mapMemory(m_StagingMem, m_StagingOffset, size);
		memcpy(bufferPtr, texInfo.Data, size);
		vk::MappedMemoryRange range;
		range.memory = m_StagingMem;
		range.offset = m_StagingOffset;
		range.size = size;
		m_Device.flushMappedMemoryRanges(1, &range);
		m_Device.unmapMemory(m_StagingMem);

		if (texInfo.Data) {
			TextureTransfer transfer;
			transfer.Image = img;
			int w = texInfo.Width, h = texInfo.Height;
			for (uint32_t i = 0; i < texInfo.MipCount; i++) {
				vk::BufferImageCopy copy;
				copy.bufferOffset = m_StagingOffset;
				copy.imageExtent = vk::Extent3D(w, h, 1);
				copy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
				copy.imageSubresource.baseArrayLayer = 0;
				copy.imageSubresource.layerCount = 1;
				copy.imageSubresource.mipLevel = i;
				transfer.copies.push_back(copy);
				m_StagingOffset += w * h * (texInfo.BPP  / 8);
				w >>= 1;
				h >>= 1;
			}
			m_ImageTransfers.push_back(transfer);
		} else {
			vk::BufferCopy copy;
			copy.dstOffset = m_DeviceOffset;
			copy.srcOffset = m_StagingOffset;
			copy.size = memReq.size;
			m_Transfers.push_back(copy);
			m_StagingOffset += memReq.size;
		}
	}
	VkAlloc alloc;
	alloc.Offset = m_DeviceOffset;
	alloc.Size = memReq.size;

	m_DeviceOffset += memReq.size;

	return alloc;
}

VkAlloc VkMemory::AllocateImageCube(vk::Image img, gli::texture_cube* texture, void* data) {
	vk::MemoryRequirements memReq = m_Device.getImageMemoryRequirements(img);
	m_DeviceOffset = (m_DeviceOffset + (memReq.alignment - 1)) & ~(memReq.alignment - 1);

	if (m_DeviceOffset + memReq.size > m_DeviceSize) {
		printf("Failed allocation of size %d\n Total memory in buffer %d, Used %d\n", (uint32_t)memReq.size, (uint32_t)m_DeviceSize, (uint32_t)m_DeviceOffset);
		return VkAlloc();
	}
	VkDeviceSize size = (texture) ? texture->size() : memReq.size;
	m_Device.bindImageMemory(img, m_DevMem, m_DeviceOffset);

	if (data && m_StagingOffset + size < m_StagingSize) {
		//transfer data to staging buffer
		void* bufferPtr = m_Device.mapMemory(m_StagingMem, m_StagingOffset, size);
		memcpy(bufferPtr, data, size);
		vk::MappedMemoryRange range;
		range.memory = m_StagingMem;
		range.offset = m_StagingOffset;
		range.size = size;
		m_Device.flushMappedMemoryRanges(1, &range);
		m_Device.unmapMemory(m_StagingMem);

		if (texture) {
			TextureTransfer transfer;
			transfer.Image = img;
			for (int f = 0; f < texture->faces(); f++) {
				for (int i = 0; i < texture->levels(); i++) {
					vk::BufferImageCopy copy;
					gli::image mip = (*texture)[f][i];
					copy.bufferOffset = m_StagingOffset;
					copy.imageExtent = vk::Extent3D(mip.extent().x, mip.extent().y, 1);
					copy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
					copy.imageSubresource.baseArrayLayer = f;
					copy.imageSubresource.layerCount = 1;
					copy.imageSubresource.mipLevel = i;
					transfer.copies.push_back(copy);
					m_StagingOffset += mip.size();
				}
			}
			m_ImageTransfers.push_back(transfer);
		} else {
			vk::BufferCopy copy;
			copy.dstOffset = m_DeviceOffset;
			copy.srcOffset = m_StagingOffset;
			copy.size = memReq.size;
			m_Transfers.push_back(copy);
			m_StagingOffset += memReq.size;
		}

	}
	VkAlloc alloc;
	alloc.Offset = m_DeviceOffset;
	alloc.Size = memReq.size;

	m_DeviceOffset += memReq.size;

	return alloc;
}

VkAlloc VkMemory::AllocateImageCube(vk::Image img, const TextureInfo& texInfo) {
	vk::MemoryRequirements memReq = m_Device.getImageMemoryRequirements(img);
	m_DeviceOffset = (m_DeviceOffset + memReq.alignment - 1) & ~memReq.alignment - 1;

	if (m_DeviceOffset + memReq.size > m_DeviceSize) {
		printf("Failed allocation of size %d\n Total memory in buffer %d, Used %d\n", (uint32_t)memReq.size, (uint32_t)m_DeviceSize, (uint32_t)m_DeviceOffset);
		return VkAlloc();
	}
	VkDeviceSize size = (texInfo.Data) ? texInfo.LinearSize : memReq.size;
	m_Device.bindImageMemory(img, m_DevMem, m_DeviceOffset);

	if (texInfo.Data && m_StagingOffset + size < m_StagingSize) {
		//transfer data to staging buffer
		void* bufferPtr = m_Device.mapMemory(m_StagingMem, m_StagingOffset, size);
		memcpy(bufferPtr, texInfo.Data, size);
		vk::MappedMemoryRange range;
		range.memory = m_StagingMem;
		range.offset = m_StagingOffset;
		range.size = size;
		m_Device.flushMappedMemoryRanges(1, &range);
		m_Device.unmapMemory(m_StagingMem);

		if (texInfo.Data) {
			TextureTransfer transfer;
			transfer.Image = img;
			for (uint32_t f = 0; f < texInfo.Layers; f++) {
				int w = texInfo.Width, h = texInfo.Height;
				for (uint32_t i = 0; i < texInfo.MipCount; i++) {
					vk::BufferImageCopy copy;
					copy.bufferOffset = m_StagingOffset;
					copy.imageExtent = vk::Extent3D(w, h, 1);
					copy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
					copy.imageSubresource.baseArrayLayer = f;
					copy.imageSubresource.layerCount = 1;
					copy.imageSubresource.mipLevel = i;
					transfer.copies.push_back(copy);
					m_StagingOffset += w * h * texInfo.BPP;
					w >>= 1;
					h >>= 1;
				}
			}
			m_ImageTransfers.push_back(transfer);
		} else {
			vk::BufferCopy copy;
			copy.dstOffset = m_DeviceOffset;
			copy.srcOffset = m_StagingOffset;
			copy.size = memReq.size;
			m_Transfers.push_back(copy);
			m_StagingOffset += memReq.size;
		}

	}
	VkAlloc alloc;
	alloc.Offset = m_DeviceOffset;
	alloc.Size = memReq.size;

	m_DeviceOffset += memReq.size;

	return alloc;
}

void VkMemory::ScheduleTransfers(VulkanCommandBuffer& cmdBuffer) {
	if (!m_Transfers.empty()) {
		//transfer data from staging buffer to device buffer
		cmdBuffer.copyBuffer(m_StagingBuffer, m_DeviceBuffer, m_Transfers);
	}
	if (!m_ImageTransfers.empty()) {
		//set image layout for textures to transfer destination
		for (auto& t : m_ImageTransfers) {
			cmdBuffer.ImageBarrier(t.Image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		}
		cmdBuffer.PushPipelineBarrier();
		//push copies
		for (auto& t : m_ImageTransfers) {
			cmdBuffer.copyBufferToImage(m_StagingBuffer, t.Image, vk::ImageLayout::eTransferDstOptimal, t.copies);
		}
		//set image layout for textures to shader read
		for (auto& t : m_ImageTransfers) {
			cmdBuffer.ImageBarrier(t.Image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
		}
		cmdBuffer.PushPipelineBarrier();
		m_ImageTransfers.clear();
	}
	//nothing can do work until all transfers are finished
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTopOfPipe, vk::DependencyFlagBits::eByRegion, nullptr, nullptr, nullptr);
	//reset the stack for the staging buffer
	m_StagingOffset = 0;
	m_Transfers.clear();
}

void VkMemory::UpdateBuffer(VkAlloc buffer, uint64_t size, void* data) {
	if (data && m_StagingOffset + size < m_StagingSize) {
		//transfer data to staging buffer
		void* bufferPtr = m_Device.mapMemory(m_StagingMem, m_StagingOffset, size);
		memcpy(bufferPtr, data, size);
		vk::MappedMemoryRange range;
		range.memory = m_StagingMem;
		range.offset = m_StagingOffset;
		range.size = size;
		m_Device.flushMappedMemoryRanges(1, &range);
		m_Device.unmapMemory(m_StagingMem);
		//prepare copy
		vk::BufferCopy copy;
		copy.dstOffset = buffer.Offset;
		copy.srcOffset = m_StagingOffset;
		copy.size = size;
		m_Transfers.push_back(copy);
		m_StagingOffset += size;
	}
}

void VkMemory::UpdateBuffer(VkAlloc buffer, uint64_t offset, uint64_t size, void* data) {
	if (data && m_StagingOffset + size < m_StagingSize) {
		//transfer data to staging buffer
		void* bufferPtr = m_Device.mapMemory(m_StagingMem, m_StagingOffset, size);
		memcpy(bufferPtr, data, size);
		vk::MappedMemoryRange range;
		range.memory = m_StagingMem;
		range.offset = m_StagingOffset;
		range.size = size;
		m_Device.flushMappedMemoryRanges(1, &range);
		m_Device.unmapMemory(m_StagingMem);
		//prepare copy
		vk::BufferCopy copy;
		copy.dstOffset = buffer.Offset + offset;
		copy.srcOffset = m_StagingOffset;
		copy.size = size;
		m_Transfers.push_back(copy);
		m_StagingOffset += size;
	}
}
void VkMemory::Deallocate(VkAlloc& alloc) {
	//end of heap just needs to rewind head
	if (alloc.Offset + alloc.Size == m_DeviceOffset) {
		m_DeviceOffset -= alloc.Size;
		return;
	}
	//otherwise we increase the amount of fragmented space
	m_DeviceFragSpace += alloc.Size;
	//TODO HANDLE ACTUAL DEALLOCATION
	alloc.TextureHandle = nullptr;
	alloc.Offset = 0;
	alloc.Size = 0;

}