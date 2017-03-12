#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb-master/stb_image.h"
#include <gli/gli.hpp>
Texture::Texture() {

}
Texture::~Texture() {

}

void Texture::Init(const std::string& filename, Memory& memory, const vk::Device& device) {

	//stbi_uc* imageData = stbi_load(filename.c_str(), &m_Width, &m_Height, &m_Channels, STBI_rgb_alpha);

	gli::texture texture(gli::load(filename));

	if (texture.target() == gli::TARGET_CUBE) {
		gli::texture_cube cubeTex(texture);
		vk::ImageCreateInfo imageInfo;
		imageInfo.arrayLayers = cubeTex.faces();
		imageInfo.extent = vk::Extent3D(cubeTex[0].extent().x, cubeTex[0].extent().y, 1);
		imageInfo.format = static_cast<vk::Format>(cubeTex.format());
		imageInfo.imageType = vk::ImageType::e2D;
		imageInfo.initialLayout = vk::ImageLayout::eUndefined;
		imageInfo.mipLevels = cubeTex.levels();
		imageInfo.samples = vk::SampleCountFlagBits::e1;
		imageInfo.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
		imageInfo.tiling = vk::ImageTiling::eOptimal;
		m_Image = device.createImage(imageInfo);

		memory.AllocateImageCube(m_Image, &cubeTex, cubeTex.data());

		vk::ImageViewCreateInfo viewInfo;
		viewInfo.components = { vk::ComponentSwizzle::eR,vk::ComponentSwizzle::eG ,vk::ComponentSwizzle::eB ,vk::ComponentSwizzle::eA };
		viewInfo.format = imageInfo.format;
		viewInfo.image = m_Image;
		viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.layerCount = cubeTex.faces();
		viewInfo.subresourceRange.levelCount = cubeTex.levels();
		viewInfo.viewType = vk::ImageViewType::eCube;

		m_View = device.createImageView(viewInfo);

	} else if(texture.target() == gli::TARGET_2D) {
		gli::texture2d tex2D(texture);

		vk::ImageCreateInfo imageInfo;
		imageInfo.arrayLayers = 1;
		imageInfo.extent = vk::Extent3D(tex2D[0].extent().x, tex2D[0].extent().y, 1);
		imageInfo.format = static_cast<vk::Format>(tex2D.format());
		imageInfo.imageType = vk::ImageType::e2D;
		imageInfo.initialLayout = vk::ImageLayout::eUndefined;
		imageInfo.mipLevels = tex2D.levels();
		imageInfo.samples = vk::SampleCountFlagBits::e1;
		imageInfo.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
		imageInfo.tiling = vk::ImageTiling::eOptimal;

		uint32_t size = tex2D.size();
		m_Image = device.createImage(imageInfo);
		memory.AllocateImage(m_Image, &tex2D, tex2D.data());

		vk::ImageViewCreateInfo viewInfo;
		viewInfo.components = { vk::ComponentSwizzle::eR,vk::ComponentSwizzle::eG ,vk::ComponentSwizzle::eB ,vk::ComponentSwizzle::eA };
		viewInfo.format = imageInfo.format;
		viewInfo.image = m_Image;
		viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.subresourceRange.levelCount = tex2D.levels();
		viewInfo.viewType = vk::ImageViewType::e2D;

		m_View = device.createImageView(viewInfo);
	}

	vk::SamplerCreateInfo sampInfo;
	sampInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
	sampInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
	sampInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
	sampInfo.anisotropyEnable = true;
	sampInfo.maxAnisotropy = 16.0f;
	sampInfo.borderColor = vk::BorderColor::eFloatOpaqueBlack;
	sampInfo.compareEnable = false;
	sampInfo.compareOp = vk::CompareOp::eNever;
	sampInfo.magFilter = vk::Filter::eLinear;
	sampInfo.minFilter = vk::Filter::eLinear;
	sampInfo.maxLod = texture.levels();
	sampInfo.minLod = 0.0f;
	sampInfo.mipLodBias = 0.0f;
	sampInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	sampInfo.unnormalizedCoordinates = false;
	m_Sampler = device.createSampler(sampInfo);

	//STBI_FREE(imageData);
}

vk::DescriptorImageInfo Texture::GetDescriptorInfo() {
	vk::DescriptorImageInfo info;
	info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	info.imageView = m_View;
	info.sampler = m_Sampler;
	return info;
}