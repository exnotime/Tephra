#pragma once
#include "PhysicsExport.h"
#include <PhysX4.0/PxPhysicsAPI.h>
#include <EASTL/vector.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
namespace smug {
enum PHYSICS_SHAPE {
	SPHERE,
	CUBE,
	PLANE,
	CAPSULE,
	SHAPE_COUNT
};

/*
Before physics update this is used to move kinematics and apply force on the body
After physics update this contains the updated position orientation of the actor shape
*/
struct PHYSICS_DLL PhysicsBody {
	glm::vec3 Position;
	glm::quat Orientation;
	glm::vec3 Force;
	uint32_t Actor;
	bool Updated;
	bool Kinematic;
	bool Controller;
	bool IsOnGround;
	uint32_t UserData;
};

struct PHYSICS_DLL PhysicsMesh {
	eastl::vector<glm::vec3> Vertices;
	eastl::vector<uint32_t> Indices;
};

struct PHYSICS_DLL RayCastResult {
	glm::vec3 Positon;
	float Distance;
	glm::vec3 Normal;
	uint32_t HitObject;
};

class PHYSICS_DLL PhysicsEngine {
  public:
	PhysicsEngine();
	~PhysicsEngine();

	void Init();
	void Update(double deltaTime);
	void Shutdown();

	PhysicsBody* CreateDynamicActor(const glm::vec3& pos, const glm::quat& orientation, const glm::vec3& size, float mass, PHYSICS_SHAPE shape, bool kinematic = false);
	PhysicsBody* CreateStaticActor(const glm::vec3& pos, const glm::quat& orientation, const glm::vec3& size, PHYSICS_SHAPE shape);
	PhysicsBody* CreateDynamicActorFromTriMesh(const glm::vec3& pos, const glm::quat& orientation, const glm::vec3& size, float mass, PhysicsMesh& mesh, bool kinematic = false);
	PhysicsBody* CreateStaticActorFromTriMesh(const glm::vec3& pos, const glm::quat& orientation, const glm::vec3& size, PhysicsMesh& mesh);
	PhysicsBody* CreateController(const glm::vec3& pos, const glm::quat& orientation, const glm::vec3& size, PHYSICS_SHAPE shape);

	bool RayCast(glm::vec3& origin, glm::vec3& direction, float maxDistance, RayCastResult& result);

	void LockRotationAxes(PhysicsBody* body, const glm::bvec3& axes);
	void DeleteActor(uint32_t actor);

	void SetGravityPoint(const glm::vec3& pos, float strength);
  private:
	physx::PxFoundation* m_Foundation;
	physx::PxPhysics* m_Physics;
	physx::PxScene* m_Scene;
	physx::PxCooking* m_Cooking;
	physx::PxControllerManager* m_ControllerManager;
#ifdef _DEBUG
	physx::PxPvd* m_PVD;
#endif
	//physx::PxDefaultAllocator m_Allocator;
	physx::PxDefaultErrorCallback m_Error;
	physx::PxDefaultCpuDispatcher* m_Disbatcher;

	double m_Accumulator = 0.0f;
	const double m_StepTime = 1.0 / 60.0;

	eastl::vector<physx::PxRigidActor*> m_Actors;
	eastl::vector<physx::PxController*> m_Controllers;
	eastl::vector<uint32_t> m_FreeActors;
	eastl::vector<PhysicsBody*> m_Bodies;
	glm::vec3 m_GravityPoint;
	float m_GravityFactor;
	bool m_HasActiveCustomGravity = false;

};
}