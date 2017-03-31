#pragma once
class Window;
class SubSystemSet;
class Timer;

class Engine {
public:
	Engine();
	~Engine();
	void Init();
	void Run();
private:
	Window* m_Window;
	SubSystemSet* m_MainSubSystemSet;
	Timer* m_GlobalTimer;
};