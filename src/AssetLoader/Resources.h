#pragma once
#include <stdint.h>
#include <EASTL/string.h>
#include <glm/glm.hpp>
#include "AssetExport.h"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#define RESOURCE_TYPE_MASK 0xffffffff00000000
#define RESOURCE_HASH_SHIFT 32
#define RESOURCE_INVALID -1

namespace smug {
	enum RESOURCE_TYPE : uint32_t {
		RT_TEXTURE = 0x1,
		RT_MODEL = 0x2,
		RT_SHADER = 0x4,
		RT_ANIMATION = 0x8,
		RT_SKELETON = 0x10,
		RT_SCRIPT = 0x20,
		RT_BUFFER = 0x40,
		RT_RENDERPASS = 0x80,
		RT_FRAMEBUFFER = 0x100,
		RT_RENDER_TARGET = 0x200
	};

typedef ASSET_DLL uint64_t ResourceHandle; //first 32 bits say type, second 32 bits say resource hash

static ResourceHandle CreateHandle(uint32_t hash, RESOURCE_TYPE type) {
	ResourceHandle h = type;
	h = h << RESOURCE_HASH_SHIFT;
	h |= hash;
	return h;
}

static RESOURCE_TYPE GetType(ResourceHandle h) {
	return (RESOURCE_TYPE)((h & RESOURCE_TYPE_MASK) >> RESOURCE_HASH_SHIFT);
}

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


enum MATERIAL_FLAGS {
	MAT_FLAG_NONE = 0x0,
	MAT_FLAG_TRANSPARENT = 0x1,
	MAT_FLAG_NO_SHADOW = 0x2,
};

struct ASSET_DLL MaterialInfo {
	ResourceHandle Albedo = RESOURCE_INVALID;
	ResourceHandle Normal = RESOURCE_INVALID;
	ResourceHandle Roughness = RESOURCE_INVALID;
	ResourceHandle Metal = RESOURCE_INVALID;
	uint32_t MaterialFlags = MAT_FLAG_NONE;
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
	VERTEX, FRAGMENT, GEOMETRY, EVALUATION, CONTROL, COMPUTE, MESH, TASK, RAY_GEN, RAY_INTERSECTION, RAY_ANY_HIT, RAY_CLOSEST_HIT, RAY_MISS, RAY_CALLABLE,  PRECOMPILED
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
};

struct ASSET_DLL Descriptor {
	uint32_t Bindpoint;
	uint32_t Set;
	uint32_t Resource;
	uint32_t Type;
	uint32_t Stage;
	uint32_t Count;
};

struct ASSET_DLL PushConstantBuffer {
	uint32_t Size;
	uint32_t Offset;
};

struct ASSET_DLL PipelineStateInfo {
	uint32_t ShaderCount;
	ShaderByteCode* Shaders;
	PushConstantBuffer PushConstants;
	uint32_t DescriptorCount;
	Descriptor* Descriptors;
	uint32_t AttachmentCount;
	VkPipelineColorBlendAttachmentState* Attachments;
	VkPipelineDepthStencilStateCreateInfo DepthStencilState;
	VkPipelineRasterizationStateCreateInfo RasterState;
	VkPrimitiveTopology Topology;
	uint32_t RenderPass;
};

}