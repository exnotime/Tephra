#include "SkyBox.h"
#include <par_shapes.h>
#include <glm/gtx/transform.hpp>
SkyBox::SkyBox() {

}
SkyBox::~SkyBox(){
}

void SkyBox::Init(const vk::Device& device, const vk::PhysicalDevice& physDev, const std::string& filename,const vk::Viewport& vp, const vk::RenderPass& rp, const vk::PipelineMultisampleStateCreateInfo& mss) {
	//load pipestate
	m_Pipeline.SetDefaultMulitSampleState(mss);
	m_Pipeline.LoadPipelineFromFile(device, "shader/Skybox.json", vp, rp);
	//allocate memory
	m_Memory.Init(device, physDev, 64 * MEGA_BYTE, 64 * MEGA_BYTE);
	m_Texture.Init(filename, m_Memory, device);
	//load model
	par_shapes_mesh* mesh = par_shapes_create_cube();
	par_shapes_scale(mesh, 2, 2, 2);
	par_shapes_translate(mesh, -1, -1, -1);
	std::vector<glm::vec3> vertices;
	for (int i = 0; i < mesh->ntriangles * 3; i += 3) {
		for (int k = 0; k < 3; k++) {
			glm::vec3 p = glm::vec3(mesh->points[mesh->triangles[i + k] * 3],
				mesh->points[mesh->triangles[i + k] * 3 + 1], mesh->points[mesh->triangles[i + k] * 3 + 2]);
			vertices.push_back(p);
		}
	}
	m_VBO = m_Memory.AllocateBuffer(sizeof(glm::vec3) * vertices.size(), vk::BufferUsageFlagBits::eVertexBuffer, vertices.data());
	m_UBO = m_Memory.AllocateBuffer(sizeof(glm::mat4) * 2, vk::BufferUsageFlagBits::eUniformBuffer, nullptr);

	par_shapes_free_mesh(mesh);
	vk::DescriptorPoolCreateInfo poolInfo;
	poolInfo.maxSets = 1;
	vk::DescriptorPoolSize poolSizes[2];
	poolSizes[0].descriptorCount = 1;
	poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
	poolSizes[1].descriptorCount = 1;
	poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSizes;
	m_DescPool = device.createDescriptorPool(poolInfo);

	vk::DescriptorSetAllocateInfo descAllocInfo;
	descAllocInfo.descriptorPool = m_DescPool;
	descAllocInfo.descriptorSetCount = 1;
	descAllocInfo.pSetLayouts = m_Pipeline.GetDescriptorSetLayouts().data();
	m_DescSet = device.allocateDescriptorSets(descAllocInfo)[0];

	std::array<vk::WriteDescriptorSet, 2> writeDescs;
	writeDescs[0].descriptorCount = 1;
	writeDescs[0].descriptorType = vk::DescriptorType::eUniformBuffer;
	writeDescs[0].dstArrayElement = 0;
	writeDescs[0].dstBinding = 0;
	writeDescs[0].dstSet = m_DescSet;
	vk::DescriptorBufferInfo descBufferInfo;
	descBufferInfo.buffer = m_UBO.BufferHandle;
	descBufferInfo.offset = 0;
	descBufferInfo.range = VK_WHOLE_SIZE;
	writeDescs[0].pBufferInfo = &descBufferInfo;
	writeDescs[1].descriptorCount = 1;
	writeDescs[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	writeDescs[1].dstArrayElement = 0;
	writeDescs[1].dstBinding = 1;
	writeDescs[1].dstSet = m_DescSet;
	vk::DescriptorImageInfo imgInfo = m_Texture.GetDescriptorInfo();
	writeDescs[1].pImageInfo = &imgInfo;
	device.updateDescriptorSets(writeDescs, nullptr);
}

void SkyBox::PrepareUniformBuffer(VulkanCommandBuffer cmdBuffer, const Camera& cam) {
	struct perFrameBuffer {
		glm::mat4 vp;
		glm::mat4 w;
	} pfb;
	pfb.w = glm::translate(cam.GetPosition());
	pfb.vp = cam.GetViewProjection();

	m_Memory.UpdateBuffer(m_UBO, sizeof(perFrameBuffer), &pfb);
	m_Memory.ScheduleTransfers(cmdBuffer);
}

void SkyBox::Render(VulkanCommandBuffer cmdBuffer) {
	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline.GetPipeline());
	vk::DeviceSize offset = 0;
	cmdBuffer.bindVertexBuffers(0, 1, &m_VBO.BufferHandle, &offset);
	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_Pipeline.GetPipelineLayout(), 0, 1, &m_DescSet, 0, nullptr);
	cmdBuffer.draw(36, 1, 0, 0);
}