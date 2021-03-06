#pragma once
#include "../SubSystem.h"
namespace smug {
struct EntityCache;
class SSCamera : public SubSystem {
  public:
	SSCamera();
	~SSCamera();

	virtual void Startup();
	virtual void Update(const double deltaTime, Profiler* profiler);
	virtual void Shutdown();
  private:
	EntityCache* m_Cache;
	bool m_FreeFlight = false;
	float m_JumpVelocity;
	float m_JumpForce;
};
}
