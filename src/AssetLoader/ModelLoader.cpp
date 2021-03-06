#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <algorithm>
#include "AssetLoader.h"
#include <Utility/Hash.h>
#include <Utility/Memory.h>
using namespace smug;
ModelLoader::ModelLoader() {}
ModelLoader::~ModelLoader() {}

eastl::string GetDir(const eastl::string& file) {
	for (uint32_t i = (uint32_t)file.length() - 1; i >= 0; i--) {
		if (file[i] == '\\' || file[i] == '/') {
			return file.substr(0, i + 1);
		}
	}
	return "";
}

char* ModelLoader::LoadModel(const eastl::string& filename, ModelInfo& model) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filename.c_str(), aiProcess_CalcTangentSpace | aiProcess_ConvertToLeftHanded | aiProcess_GenUVCoords | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_PreTransformVertices);
	if (scene) {
		//meshes
		if (scene->HasMeshes()) {
			model.MeshCount = scene->mNumMeshes;
			model.Meshes = new MeshInfo[model.MeshCount];
			uint32_t indexCount = 0;
			for (uint32_t m = 0; m < model.MeshCount; ++m) {
				MeshInfo& meshInfo = model.Meshes[m];
				aiMesh* mesh = scene->mMeshes[m];
				if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) {
					meshInfo.IndexCount = 0;
					meshInfo.VertexCount = 0;
					meshInfo.Vertices = nullptr;
					meshInfo.Indices = nullptr;
					meshInfo.Material = 0;
					continue;
				}
				meshInfo.Material = mesh->mMaterialIndex;
				meshInfo.VertexCount = mesh->mNumVertices;
				meshInfo.Vertices = new Vertex[meshInfo.VertexCount];
				for (uint32_t v = 0; v < meshInfo.VertexCount; ++v) {
					meshInfo.Vertices[v].Position = glm::vec3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);
					meshInfo.Vertices[v].Normal = glm::normalize(glm::vec3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z));
					if (mesh->HasTangentsAndBitangents()) {
						glm::vec3 t = glm::normalize(glm::vec3(mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z));
						glm::vec3 b = glm::normalize(glm::vec3(mesh->mBitangents[v].x, mesh->mBitangents[v].y, mesh->mBitangents[v].z));
						if (glm::dot(glm::cross(meshInfo.Vertices[v].Normal, t), b) < 0.0f) {
							t *= -1;
						}
						meshInfo.Vertices[v].Tangent = t;
					}
					if (mesh->GetNumUVChannels() > 0) {
						meshInfo.Vertices[v].TexCoord = glm::vec2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
					}
				}
				meshInfo.IndexCount = mesh->mNumFaces * 3;
				meshInfo.Indices = new uint32_t[meshInfo.IndexCount];
				for (uint32_t f = 0; f < mesh->mNumFaces; ++f) {
					meshInfo.Indices[f * 3] = indexCount + mesh->mFaces[f].mIndices[0];
					meshInfo.Indices[f * 3 + 1] = indexCount + mesh->mFaces[f].mIndices[1];
					meshInfo.Indices[f * 3 + 2] = indexCount + mesh->mFaces[f].mIndices[2];
				}
				indexCount += meshInfo.VertexCount;
			}
		}
		if (scene->HasMaterials()) {
			model.MaterialCount = scene->mNumMaterials;
			model.Materials = new MaterialInfo[model.MaterialCount];
			for (uint32_t m = 0; m < model.MaterialCount; ++m) {
				aiMaterial* mat = scene->mMaterials[m];
				printf("Material key %s\n", mat->GetName().C_Str());
				for (uint32_t p = 0; p < mat->mNumProperties; ++p) {
					printf("Material key %s\n", mat->mProperties[p]->mKey.C_Str());
				}

				eastl::string dir = GetDir(filename);// filename.substr(0, filename.find_last_of() + 1);
				aiString path;
				if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
					mat->GetTexture(aiTextureType_DIFFUSE, 0, &path);
					eastl::string p = dir + path.data;
					model.Materials[m].Albedo = g_AssetLoader.LoadAsset(p.c_str());
				}
				//obj stores normals in height
				if (mat->GetTextureCount(aiTextureType_HEIGHT)) {
					mat->GetTexture(aiTextureType_HEIGHT, 0, &path);
					eastl::string p = dir + path.data;
					model.Materials[m].Normal = g_AssetLoader.LoadAsset(p.c_str());
				}
				//dae in normals
				if (mat->GetTextureCount(aiTextureType_NORMALS)) {
					mat->GetTexture(aiTextureType_NORMALS, 0, &path);
					eastl::string p = dir + path.data;
					model.Materials[m].Normal = g_AssetLoader.LoadAsset(p.c_str());
				}
				if (mat->GetTextureCount(aiTextureType_SPECULAR)) {
					mat->GetTexture(aiTextureType_SPECULAR, 0, &path);
					eastl::string p = dir + path.data;
					model.Materials[m].Roughness = g_AssetLoader.LoadAsset(p.c_str());
				}
				if (mat->GetTextureCount(aiTextureType_EMISSIVE)) {
					mat->GetTexture(aiTextureType_EMISSIVE, 0, &path);
					eastl::string p = dir + path.data;
					model.Materials[m].Metal = g_AssetLoader.LoadAsset(p.c_str());
				}
			}
		} else {
			//force default material
			model.MaterialCount = 1;
			model.Materials = new MaterialInfo();
			for (uint32_t m = 0; m < model.MeshCount; ++m) {
				model.Meshes[m].Material = 0;
			}
		}
	} else {
		return "Assimp error";
	}
	return nullptr;
}

LoadResult ModelLoader::LoadAsset(const char* filename) {
	ModelInfo* info = new ModelInfo();
	char* error = LoadModel(filename, *info);
	LoadResult res;
	if (error) {
		res.Error = error;
		res.Data = nullptr;
		res.Type = RT_MODEL;
	} else {
		res.Hash = HashString(filename);
		res.Data = info;
		res.Type = RT_MODEL;
		res.Error = "";
	}
	return res;
}

void ModelLoader::UnloadAsset(void* asset) {
	ModelInfo* info = (ModelInfo*)asset;
	for (uint32_t m = 0; m < info->MeshCount; ++m) {
		free(info->Meshes[m].Vertices);
		free(info->Meshes[m].Indices);
	}
	free(info->Materials);
	free(info->Meshes);
	free(info);
}

void ModelLoader::SerializeAsset(FileBuffer* buffer, LoadResult* asset) {
	// Models are stored as:
	// Header + mesh headers + materials + mesh data (vertices + indices)
	ModelInfo* info = (ModelInfo*)asset->Data;
	size_t meshDataOffset = sizeof(ModelInfo) + info->MeshCount * sizeof(MeshInfo) + info->MaterialCount * sizeof(MaterialInfo);
	eastl::vector<MeshInfo> meshes;
	size_t offset = meshDataOffset;
	for (uint32_t m = 0; m < info->MeshCount; ++m) {
		MeshInfo mesh;
		mesh = info->Meshes[m];
		mesh.Vertices = (Vertex*)offset;
		offset += sizeof(Vertex) * mesh.VertexCount;
		mesh.Indices = (uint32_t*)offset;
		offset += mesh.IndexCount * sizeof(uint32_t);
		meshes.push_back(mesh);
	}
	for (uint32_t m = 0; m < info->MaterialCount; ++m) {
		eastl::string albedo = g_AssetLoader.GetFilenameFromCache(info->Materials[m].Albedo);
		if (!albedo.empty()) {
			info->Materials[m].Albedo = HashString(albedo);
		} else {
			info->Materials[m].Albedo = RESOURCE_INVALID;
		}

		eastl::string normal = g_AssetLoader.GetFilenameFromCache(info->Materials[m].Normal);
		if (!normal.empty()) {
			info->Materials[m].Normal = HashString(normal);
		} else {
			info->Materials[m].Normal = RESOURCE_INVALID;
		}

		eastl::string metal = g_AssetLoader.GetFilenameFromCache(info->Materials[m].Metal);
		if (!metal.empty()) {
			info->Materials[m].Metal = HashString(metal);
		} else {
			info->Materials[m].Metal = RESOURCE_INVALID;
		}

		eastl::string roughness = g_AssetLoader.GetFilenameFromCache(info->Materials[m].Roughness);
		if (!albedo.empty()) {
			info->Materials[m].Roughness = HashString(roughness);
		} else {
			info->Materials[m].Roughness = RESOURCE_INVALID;
		}
	}

	ModelInfo* model = new ModelInfo();
	*model = *info;
	model->Meshes = (MeshInfo*)sizeof(ModelInfo);
	size_t meshSize = sizeof(MeshInfo);
	model->Materials = (MaterialInfo*)((uint8_t*)model->Meshes + sizeof(MeshInfo) * info->MeshCount);

	buffer->Write(sizeof(ModelInfo), model, asset->Hash);
	buffer->Write(sizeof(MeshInfo) * meshes.size(), meshes.data(), asset->Hash);
	buffer->Write(sizeof(MaterialInfo) * info->MaterialCount, info->Materials, asset->Hash);
	for (uint32_t m = 0; m < info->MeshCount; ++m) {
		buffer->Write(info->Meshes[m].VertexCount * sizeof(Vertex), info->Meshes[m].Vertices, asset->Hash);
		buffer->Write(info->Meshes[m].IndexCount * sizeof(uint32_t), info->Meshes[m].Indices, asset->Hash);
	}
}

DeSerializedResult ModelLoader::DeSerializeAsset(void* assetBuffer) {
	ModelInfo* src = (ModelInfo*)assetBuffer;
	ModelInfo* dst = new ModelInfo();
	*dst = *src;
	//copy materials
	dst->Materials = (MaterialInfo*)malloc(src->MaterialCount * sizeof(MaterialInfo));
	memcpy(dst->Materials, PointerAdd(src, (size_t)src->Materials), src->MaterialCount * sizeof(MaterialInfo));
	//copy mesh headers
	dst->Meshes = (MeshInfo*)malloc(src->MeshCount * sizeof(MeshInfo));
	memcpy(dst->Meshes, PointerAdd(src, (size_t)src->Meshes), src->MeshCount * sizeof(MeshInfo));
	for (uint32_t i = 0; i < src->MeshCount; i++) {
		//copy vertices
		size_t vertexOffset = (size_t)dst->Meshes[i].Vertices;
		dst->Meshes[i].Vertices = (Vertex*)malloc( dst->Meshes[i].VertexCount * sizeof(Vertex));
		memcpy(dst->Meshes[i].Vertices, PointerAdd(src, vertexOffset), dst->Meshes[i].VertexCount * sizeof(Vertex));
		//copy indices
		size_t indexOffset = (size_t)dst->Meshes[i].Indices;
		dst->Meshes[i].Indices = (uint32_t*)malloc(dst->Meshes[i].IndexCount * sizeof(uint32_t));
		memcpy(dst->Meshes[i].Indices, PointerAdd(src, indexOffset), dst->Meshes[i].IndexCount * sizeof(uint32_t));
	}

	for (uint32_t i = 0; i < src->MaterialCount; i++) {
		dst->Materials[i].Albedo = g_AssetLoader.LoadAsset((uint32_t)dst->Materials[i].Albedo);
		dst->Materials[i].Metal = g_AssetLoader.LoadAsset((uint32_t)dst->Materials[i].Metal);
		dst->Materials[i].Normal = g_AssetLoader.LoadAsset((uint32_t)dst->Materials[i].Normal);
		dst->Materials[i].Roughness = g_AssetLoader.LoadAsset((uint32_t)dst->Materials[i].Roughness);
	}

	DeSerializedResult res;
	res.Data = dst;
	res.Type = RT_MODEL;
	return res;
}

bool ModelLoader::IsExtensionSupported(const char* extension) {
	const char* extensions[] = { "obj", "dae", "blend", "gltf2", "gltf", "fbx" };
	for (auto& ext : extensions) {
		if (strcmp(ext, extension) == 0)
			return true;
	}
	return false;
}

bool ModelLoader::IsTypeSupported(int type) {
	return type == RT_MODEL;
}