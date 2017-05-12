#pragma once
#include <string>
#include <glm/glm.hpp>
#include "VkMemory.h"
#include <AssetLoader/Resources.h>
class VkTexture {
public:
	VkTexture();
	~VkTexture();

	void Init(const std::string& filename, VkMemory& memory, const vk::Device& device);
	void Init(const TextureInfo& texInfo, VkMemory& memory, const vk::Device& device);
	vk::DescriptorImageInfo GetDescriptorInfo();
	vk::Image GetImage() { return m_Image; }

private:
	int m_Width;
	int m_Height;
	int m_Channels;
	vk::Image m_Image;
	vk::Sampler m_Sampler;
	vk::ImageView m_View;
};