#include "SSPhysics.h"
#include "../../GlobalSystems.h"
#include "Core/components/TransformComponent.h"
#include "Core/components/ModelComponent.h"
#include "Core/components/RigidBodyComponent.h"
#include "Core/datasystem/ComponentManager.h"
#include "Core/entity/EntityManager.h"
#include <Core/Input.h>
SSPhysics::SSPhysics(){

}

SSPhysics::~SSPhysics(){

}

void SSPhysics::Startup() {
	//create camera entity
	Entity& e = g_EntityManager.CreateEntity();

	TransformComponent tc;
	tc.Position = glm::vec3(-5, 5, 0);
	tc.Scale = glm::vec3(1.0f);
	g_ComponentManager.CreateComponent(&tc, e, TransformComponent::Flag);

	RigidBodyComponent rc;
	rc.Body = globals::g_Physics->CreateDynamicActor(tc.Position, tc.Orientation, tc.Scale, 1.0f, SPHERE);
	rc.Body->Force = glm::vec3(0, 0, 0);
	g_ComponentManager.CreateComponent(&rc, e, RigidBodyComponent::Flag);

	ModelComponent mc;
	mc.ModelHandle = g_AssetLoader.LoadAsset("assets/suzzanne/suzzanne.obj");
	g_ComponentManager.CreateComponent(&mc, e, ModelComponent::Flag);

	globals::g_Physics->SetGravityPoint(glm::vec3(0, -100, 0), 1.0f);

	globals::g_Physics->CreateStaticActor(glm::vec3(0, -100, 0), glm::quat(), glm::vec3(100), SPHERE);
	globals::g_Physics->CreateStaticActor(glm::vec3(-10, 20, 0), glm::quat(), glm::vec3(10), SPHERE);
}

void SSPhysics::Update(const double deltaTime) {
	//tick physics
	globals::g_Physics->Update(deltaTime);

	//fetch results
	int flag = TransformComponent::Flag | RigidBodyComponent::Flag;
	for (auto& e : g_EntityManager.GetEntityList()) {
		if ((e.ComponentBitfield & flag) == flag) {
			TransformComponent* tc = (TransformComponent*)g_ComponentManager.GetComponent(e, TransformComponent::Flag);
			RigidBodyComponent* rc = (RigidBodyComponent*)g_ComponentManager.GetComponent(e, RigidBodyComponent::Flag);

			tc->Position = rc->Body->Position;
			tc->Orientation = rc->Body->Orientation;
		}
	}


}

void SSPhysics::Shutdown() {
}
