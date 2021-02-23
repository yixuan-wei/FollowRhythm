#pragma once

class Game;
class Window;
struct Vec2;

//---------------------------------------------------
class App
{
public:
	App()=default;
	~App()=default;
	void Startup();
	void Shutdown();
	void RunFrame();

	bool IsQuiting() const { return m_isQuiting; };
	bool HandleQuitRequisted();
	Vec2 GetWindowDimensions() const;

private:
	void BeginFrame();
	void Update();
	void Render() const;
	void EndFrame();

	//Variables
	bool  m_isQuiting = false;
	Game* m_theGame = nullptr;
	Window* m_theWindow = nullptr;
};