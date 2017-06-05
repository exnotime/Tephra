#pragma once
#include <string>
#include "Resources.h"
//Textureloader uses GLI to load dds and ktx
class TextureLoader {
  public:
	TextureLoader();
	~TextureLoader();
	char* LoadTexture(const std::string& filename, TextureInfo& info);

};