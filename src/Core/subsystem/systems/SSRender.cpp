#include "SSRender.h"
#include "Core/components/ModelComponent.h"
#include "Core/components/TransformComponent.h"
#include "Core/datasystem/ComponentManager.h"
#include "Core/entity/EntityManager.h"
#include <Graphics/GraphicsEngine.h>
#include <AssetLoader/AssetLoader.h>
#include <glm/gtx/transform.hpp>
#include "../../GlobalSystems.h"
#include <Imgui/imgui.h>

SSRender::SSRender(){

}

SSRender::~SSRender(){

}

void SSRender::Startup() {
	Entity& e = g_EntityManager.CreateEntity();

	TransformComponent tc;
	tc.Orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	tc.Position = glm::vec3(0.0f, 0.08f, 0.0f);
	tc.Scale = glm::vec3(10.0f);
	tc.Transform = glm::mat4(1.0f);
	g_ComponentManager.CreateComponent(&tc, e, tc.Flag);

	ModelComponent mc;
	mc.ModelHandle = g_AssetLoader.LoadAsset("assets/KoopaTroopa/koopa.obj");
	mc.Tint = glm::vec4(1.0f);
	g_ComponentManager.CreateComponent(&mc, e, mc.Flag);
}

void SSRender::Update(const double deltaTime) {

	int flag = ModelComponent::Flag | TransformComponent::Flag;
	RenderQueue* rq = globals::g_Gfx->GetRenderQueue();
	for (auto& e : g_EntityManager.GetEntityList()) {
		if ((e.ComponentBitfield & flag) == flag) {
			ModelComponent* mc = (ModelComponent*)g_ComponentManager.GetComponent(e, ModelComponent::Flag);
			TransformComponent* tc = (TransformComponent*)g_ComponentManager.GetComponent(e, TransformComponent::Flag);
			if (e.UID == 2) {
				ImGui::InputFloat3("KoopaPos", &tc->Position[0]);
				ImGui::InputFloat4("KoopaRot", &tc->Orientation[0]);
			}

			tc->Transform = glm::scale(tc->Scale) * glm::translate(tc->Position) * glm::toMat4(tc->Orientation);

			ShaderInput si;
			si.Transform = tc->Transform;
			si.Color = mc->Tint;
			rq->AddModel(mc->ModelHandle, si);
		}
	}

}

void SSRender::Shutdown() {
}

