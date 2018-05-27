#pragma once
#include <stdint.h>
#include <string>
#include <glm/glm.hpp>
#include "AssetExport.h"

#define RESOURCE_TYPE_MASK 0x00000000ffffffff
#define RESOURCE_HASH_SHIFT 32
#define RESOURCE_INVALID -1

namespace smug {
	typedef uint64_t ResourceHandle; //first 32 bits say type, second 32 bits say resource hash

	enum RESOURCE_TYPE : uint32_t {
		RT_TEXTURE = 0x1,
		RT_MODEL = 0x2,
		RT_SHADER = 0x4,
		RT_ANIMATION = 0x8,
		RT_SKELETON = 0x10,
		RT_SCRIPT = 0x20
	};

	struct ASSET_DLL TextureInfo {
		uint32_t Width;
		uint32_t Height;
		uint32_t MipCount;
		uint32_t Layers;
		uint32_t Format; //matches ogl/vulkan format
		uint32_t BPP;
		uint32_t LinearSize;
		void* Data = nullptr;
	};

	struct ASSET_DLL MaterialInfo {
		ResourceHandle Albedo = RESOURCE_INVALID;
		ResourceHandle Normal = RESOURCE_INVALID;
		ResourceHandle Roughness = RESOURCE_INVALID;
		ResourceHandle Metal = RESOURCE_INVALID;
	};

	struct ASSET_DLL Vertex {
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec2 TexCoord;
	};

	struct ASSET_DLL MeshInfo {
		uint32_t VertexCount;
		Vertex* Vertices = nullptr;
		uint32_t IndexCount;
		uint32_t* Indices = nullptr;
		uint32_t Material;
	};

	struct ASSET_DLL ModelInfo {
		uint32_t MeshCount;
		MeshInfo* Meshes = nullptr;
		uint32_t MaterialCount;
		MaterialInfo* Materials = nullptr;
	};

	enum ASSET_DLL SHADER_KIND {
		VERTEX, FRAGMENT, GEOMETRY, EVALUATION, CONTROL, COMPUTE, PRECOMPILED
	};

	enum ASSET_DLL SHADER_BYTECODE_TYPE {
		DXBC, DXIL, SPIRV
	};
	enum ASSET_DLL SHADER_LANGUAGE {
		GLSL, HLSL
	};

	struct ASSET_DLL ShaderByteCode {
		SHADER_BYTECODE_TYPE Type;
		SHADER_KIND Kind;
		SHADER_LANGUAGE SrcLanguage;
		uint32_t ByteCodeSize;
		void* ByteCode;
		uint32_t DependencyCount;
		uint32_t* DependenciesHashes;
	};

	struct ASSET_DLL ShaderInfo {
		uint32_t ShaderCount;
		ShaderByteCode* Shaders;
	};
}