#include "AssetLoader.h"
#include <string>
#include <stdio.h>
AssetLoader::AssetLoader(){}
AssetLoader::~AssetLoader(){}

void AssetLoader::SetResourceAllocator(ResourceAllocator allocator) {
	m_Allocator = allocator;
}

AssetLoader* AssetLoader::GetInstance() {
	if (!m_Instance) {
		m_Instance = new AssetLoader();
	}
	return m_Instance;
}

bool IsTexture(const std::string& ext) {
	const std::string textureTypes[] = { "dds", "ktx" };
	for (auto& e : textureTypes) {
		if (e == ext)
			return true;
	}
	return false;
}

bool IsModel(const std::string& ext) {
	const std::string modelTypes[] = { "dae", "obj" };
	for (auto& e : modelTypes) {
		if (e == ext)
			return true;
	}
	return false;
}

ResourceHandle AssetLoader::LoadAsset(const char* filename) {
	std::string file(filename);
	std::string extention = file.substr(file.find_last_of('.') + 1);
	char* error = nullptr;
	if (IsTexture(extention)) {
		TextureInfo texInfo;
		error = m_TexLoader.LoadTexture(file, texInfo);
		if (!error) {
			ResourceHandle handle = m_Allocator.AllocTexture(texInfo);
			handle &= (32 << RT_TEXTURE);
			return handle;
		}
	}
	else if (IsModel(extention)) {
		ModelInfo modelInfo;
		error = m_ModelLoader.LoadModel(file, modelInfo);
		if (!error) {
			ResourceHandle handle = m_Allocator.AllocModel(modelInfo);
			handle &= (32 << RT_MODEL);
			return handle;
		}
	}

	printf("Error loading asset %s\n Error: %s\n", filename, error);
	return -1;
}