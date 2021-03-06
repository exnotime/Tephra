#pragma once
struct ImGuiContext;

namespace smug {
class Window;
class SubSystemSet;
class Timer;
class Profiler;

class Engine {
  public:
	Engine();
	~Engine();
	void Init();
	void Run();
	void Shutdown();
  private:
	Window* m_Window;
	SubSystemSet* m_MainSubSystemSet;
	Timer* m_GlobalTimer;
	Timer* m_ProfilerTimer;
	ImGuiContext* m_ImguiCtx;
	Profiler* m_Profiler;
};
}