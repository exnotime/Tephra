#include "if_Render.h"
#include <assert.h>
#include <AngelScript/ScriptEngine.h>
#include <Core/entity/EntityManager.h>
#include <Core/datasystem/ComponentManager.h>
#include <Core/components/ModelComponent.h>
#include <Core/components/TransformComponent.h>
#include <Graphics/GraphicsEngine.h>
#include <Core/GlobalSystems.h>
#include <Utility/Hash.h>
using namespace AngelScript;
namespace smug {
	namespace if_render {

#define DEF_ENUM(v) #v, v

		enum RENDER_TARGET_FORMAT {
			R8,
			R8G8B8A8,
			R16G16B16A16,
			D32,
			D24S8,
			NUM_FORMATS
		};

		enum RENDER_QUEUE {
			RENDER_QUEUE_GFX,
			RENDER_QUEUE_COMPUTE0,
			RENDER_QUEUE_COMPUTE1,
			RENDER_QUEUE_COMPUTE2,
			RENDER_QUEUE_COMPUTE3,
			RENDER_QUEUE_COPY
		};

		VkFormat RTFormatToVKFormat(RENDER_TARGET_FORMAT format) {
			switch (format){
			case smug::if_render::R8:
					return VkFormat::VK_FORMAT_R8_UNORM;
					break;
			case smug::if_render::R8G8B8A8:
				return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
				break;
			case smug::if_render::R16G16B16A16:
				return VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
				break;
			case smug::if_render::D32:
				return VkFormat::VK_FORMAT_D32_SFLOAT;
				break;
			case smug::if_render::D24S8:
				return VkFormat::VK_FORMAT_D24_UNORM_S8_UINT;
				break;
			}
			return VkFormat::VK_FORMAT_UNDEFINED;
		}

		void AllocateRenderTarget(int width, int height, int format, const eastl::string name) {
			globals::g_Gfx->GetFrameBufferManager().AllocRenderTarget(HashString(name), width, height, 1, RTFormatToVKFormat((RENDER_TARGET_FORMAT)format));
		}

		void CreateRenderPass(eastl::string name, const CScriptArray* renderTargets) {
			eastl::vector<uint32_t> renderTargetHashes;
			uint32_t rtCount = renderTargets->GetSize();
			for (uint32_t i = 0; i < rtCount; i++) {
				const eastl::string* rt = (const eastl::string*)renderTargets->At(i);
				renderTargetHashes.push_back(HashString(*rt));
			}
			renderTargets->Release();
			globals::g_Gfx->GetFrameBufferManager().CreateRenderPass(HashString(name), renderTargetHashes);
			
		}

		enum BUFFER_USAGE {
			BUFFER_USAGE_UNIFORM = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			BUFFER_USAGE_STORAGE = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			BUFFER_USAGE_VERTEX = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			BUFFER_USAGE_INDEX = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			BUFFER_USAGE_INDIRECT = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
			BUFFER_USAGE_TRANSFER_DST = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			BUFFER_USAGE_TRANSFER_SRC = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
		};

		void AllocateBuffer(uint64_t size, int usage, const eastl::string name) {
			ResourceHandle handle = CreateHandle(HashString(name.c_str()), RT_BUFFER);
			globals::g_Gfx->GetResourceHandler().AllocateBuffer(size, usage, handle);
		}

		void AllocateRenderPass(const CScriptArray* renderTargets) {

		}

		void AllocateFrameBuffer(const CScriptArray* renderTargets) {

		}

		uint64_t LoadShader(eastl::string name) {
			ResourceHandle h = g_AssetLoader.LoadAsset(name.c_str());
			return h;
		}

		void Render(int key, uint32_t stencilRef) {
			RenderCmd cmd;
			cmd.RenderKey = (RENDER_KEY)key;
			cmd.SortKey = UINT64_MAX;
			cmd.StencilRef = stencilRef;
			//globals::g_Gfx->GetRenderPipeline().RecordRenderCommand(cmd);
		}

		void Dispatch(const uint64_t shader, int workGroupX, int workGroupY, int workGroupZ, int queue) {
			DispatchCmd cmd;
			cmd.Queue = queue;
			cmd.Shader = shader;
			cmd.WorkGroupCountX = workGroupX;
			cmd.WorkGroupCountY = workGroupY;
			cmd.WorkGroupCountZ = workGroupZ;
			//globals::g_Gfx->GetRenderPipeline().RecordDispatchCommand(cmd);
		}

		void Copy(const eastl::string name, const eastl::string dst, const eastl::string src, uint64_t offsetDst, uint64_t offsetSrc, uint64_t size) {

		}

		void Fence(const eastl::string name, const eastl::string fence, FENCE_CMD cmd) {

		}

		void AddToStaticQueue(uint32_t entity) {
			RenderQueue* rq = globals::g_Gfx->GetStaticQueue();
			int flag = ModelComponent::Flag | TransformComponent::Flag;
			auto& e = globals::g_EntityManager->GetEntity(entity);
			if ((e.ComponentBitfield & flag) == flag) {
				ModelComponent* mc = (ModelComponent*)globals::g_Components->GetComponent(e, ModelComponent::Flag);
				TransformComponent* tc = (TransformComponent*)globals::g_Components->GetComponent(e, TransformComponent::Flag);
				tc->Transform = glm::transpose(glm::toMat4(tc->Orientation));
				tc->Transform[0] *= tc->Scale.x;
				tc->Transform[1] *= tc->Scale.y;
				tc->Transform[2] *= tc->Scale.z;
				tc->Transform[0][3] = tc->Position.x;
				tc->Transform[1][3] = tc->Position.y;
				tc->Transform[2][3] = tc->Position.z;

				rq->AddModel(mc->ModelHandle, mc->Shader, tc->Transform, mc->Tint, mc->Layer);
			}
		}

		void InitInterface() {
			
			asIScriptEngine* engine = g_ScriptEngine.GetEngine();
			engine->RegisterEnum("RENDER_TARGET_FORMAT");
			engine->RegisterEnumValue("RENDER_TARGET_FORMAT", DEF_ENUM(R8));
			engine->RegisterEnumValue("RENDER_TARGET_FORMAT", DEF_ENUM(R8G8B8A8));
			engine->RegisterEnumValue("RENDER_TARGET_FORMAT", DEF_ENUM(R16G16B16A16));
			engine->RegisterEnumValue("RENDER_TARGET_FORMAT", DEF_ENUM(D32));
			engine->RegisterEnumValue("RENDER_TARGET_FORMAT", DEF_ENUM(D24S8));
			engine->RegisterEnum("RENDER_KEY");
			engine->RegisterEnumValue("RENDER_KEY", DEF_ENUM(RENDER_KEY_OPAQUE));
			engine->RegisterEnumValue("RENDER_KEY", DEF_ENUM(RENDER_KEY_TRANSPARENT));
			engine->RegisterEnumValue("RENDER_KEY", DEF_ENUM(RENDER_KEY_SHADOW));
			engine->RegisterEnumValue("RENDER_KEY", DEF_ENUM(RENDER_KEY_RAY_TRACED));
			engine->RegisterEnumValue("RENDER_KEY", DEF_ENUM(RENDER_KEY_DYNAMIC));
			engine->RegisterEnumValue("RENDER_KEY", DEF_ENUM(RENDER_KEY_STATIC));
			engine->RegisterEnum("FENCE_CMD");
			engine->RegisterEnumValue("FENCE_CMD", DEF_ENUM(FENCE_WAIT));
			engine->RegisterEnumValue("FENCE_CMD", DEF_ENUM(FENCE_WRTIE));
			engine->RegisterEnum("RENDER_QUEUE");
			engine->RegisterEnumValue("RENDER_QUEUE", DEF_ENUM(RENDER_QUEUE_GFX));
			engine->RegisterEnumValue("RENDER_QUEUE", DEF_ENUM(RENDER_QUEUE_COMPUTE0));
			engine->RegisterEnumValue("RENDER_QUEUE", DEF_ENUM(RENDER_QUEUE_COMPUTE1));
			engine->RegisterEnumValue("RENDER_QUEUE", DEF_ENUM(RENDER_QUEUE_COMPUTE2));
			engine->RegisterEnumValue("RENDER_QUEUE", DEF_ENUM(RENDER_QUEUE_COMPUTE3));
			engine->RegisterEnumValue("RENDER_QUEUE", DEF_ENUM(RENDER_QUEUE_COPY));
			engine->RegisterEnum("BUFFER_USAGE");
			engine->RegisterEnumValue("BUFFER_USAGE", DEF_ENUM(BUFFER_USAGE_UNIFORM));
			engine->RegisterEnumValue("BUFFER_USAGE", DEF_ENUM(BUFFER_USAGE_STORAGE));
			engine->RegisterEnumValue("BUFFER_USAGE", DEF_ENUM(BUFFER_USAGE_VERTEX));
			engine->RegisterEnumValue("BUFFER_USAGE", DEF_ENUM(BUFFER_USAGE_INDEX));
			engine->RegisterEnumValue("BUFFER_USAGE", DEF_ENUM(BUFFER_USAGE_INDIRECT));
			engine->RegisterEnumValue("BUFFER_USAGE", DEF_ENUM(BUFFER_USAGE_TRANSFER_DST));
			engine->RegisterEnumValue("BUFFER_USAGE", DEF_ENUM(BUFFER_USAGE_TRANSFER_SRC));

			int r = 0;
			r = engine->RegisterGlobalFunction("void AllocateRenderTarget(int width, int height, RENDER_TARGET_FORMAT format, string name)", asFUNCTION(AllocateRenderTarget), asCALL_CDECL); assert(r >= 0);
			r = engine->RegisterGlobalFunction("void AllocateBuffer(uint64 size, int usage, string name)", asFUNCTION(AllocateBuffer), asCALL_CDECL); assert(r >= 0);
			r = engine->RegisterGlobalFunction("uint64 LoadShader(string name)", asFUNCTION(LoadShader), asCALL_CDECL); assert(r >= 0);
			r = engine->RegisterGlobalFunction("void Render(int key, uint stencilRef)", asFUNCTION(Render), asCALL_CDECL); assert(r >= 0);
			r = engine->RegisterGlobalFunction("void Dispatch(const uint64 shader, int workGroupX, int workGroupY, int workGroupZ, int queue)", asFUNCTION(Dispatch), asCALL_CDECL); assert(r >= 0);
			r = engine->RegisterGlobalFunction("void CreateRenderPass(string name, const array<string>@ renderTargets)", asFUNCTION(CreateRenderPass), asCALL_CDECL); assert(r >= 0);
			r = engine->RegisterGlobalFunction("void AddToStaticQueue(uint entity)", asFUNCTION(AddToStaticQueue), asCALL_CDECL); assert(r >= 0);
			
		}
	}
}