#pragma once
#include "GraphicsObjects.h"
#include "VkMemory.h"
#include "Texture.h"

///this class manages the lighting environment
class GFX_DLL LightEnvironment {
  public:
	LightEnvironment();
	~LightEnvironment();
	void Init(const VkMemory* texMem, const VkMemory* bufferMem, uint32_t maxPointLights = 64, uint32_t maxSpotLights = 16, uint32_t maxDirLights = 4);
	void Update();
	void SetIBL(const std::string& irradiance, const std::string& radiance);
	void Clear();
	void AddDirLight(const DirLight& dl);
	void AddPointLight(const PointLight& pl);
	void TransferStaticLights();
	void TransferDynamicLights(std::vector<DirLight>& dirLights, std::vector<PointLight>& pointLights);

  private:
	VkTexture m_Irradiance;
	VkTexture m_Radiance;
	VkTexture m_IntegratedBRDF;

	std::vector<DirLight> m_DirLights;
	std::vector<PointLight> m_PointLights;
	std::vector<SpotLight> m_SpotLights;

	vk::DescriptorSet m_DescSet;
	VkAlloc m_DirLightBuffer;
	VkAlloc m_PointLightBuffer;
	VkAlloc m_SpotLightBuffer;

	VkMemory* m_TexMem;
	VkMemory* m_BufferMem;
};