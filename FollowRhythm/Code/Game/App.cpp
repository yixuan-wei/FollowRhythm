#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

//////////////////////////////////////////////////////////////////////////
COMMAND(quit, "Quit the game", eEventFlag::EVENT_GLOBAL) {
    UNUSED(args);
    g_theApp->HandleQuitRequisted();
    return true;
}

//////////////////////////////////////////////////////////////////////////
void App::Startup()
{
    //config setting
    float windowClientRatioOfHeight = g_gameConfigBlackboard->GetValue("windowHeightRatio", 0.8f);
    float aspectRatio = g_gameConfigBlackboard->GetValue("windowAspect", 16.0f / 9.0f);
    std::string windowTitle = g_gameConfigBlackboard->GetValue("windowTitle", "SD2.A01");

    g_theApp = &(*this);						//initialize global App pointer
    g_theRenderer = new RenderContext();		//initialize global RendererContext pointer
    g_theInput = new InputSystem();
    g_theEvents = new EventSystem();
    g_theAudio = new AudioSystem();
    g_theConsole = new DevConsole(g_theInput);
    m_theGame = new Game();

    //set up window
    m_theWindow = new Window();
    m_theWindow->Open(windowTitle, aspectRatio, windowClientRatioOfHeight);
    m_theWindow->SetInputSystem(g_theInput);

    g_theRenderer->Startup(m_theWindow);
    g_theInput->Startup();
    g_theAudio->Startup();
    g_theConsole->Startup();

    Clock::SystemStartup();
    DebugRenderSystemStartup(g_theRenderer);

    m_theGame->StartUp();
}

//////////////////////////////////////////////////////////////////////////
void App::Shutdown()
{
    DebugRenderSystemShutdown();
    Clock::SystemShutdown();

    m_theGame->ShutDown();
    g_theConsole->Shutdown();
    g_theAudio->Shutdown();
    g_theRenderer->Shutdown();
    g_theInput->Shutdown();
    m_theWindow->Close();

    delete m_theGame;
    m_theGame = nullptr;

    delete g_theConsole;
    g_theConsole = nullptr;

    delete g_theAudio;
    g_theAudio = nullptr;

    delete g_theRenderer;
    g_theRenderer = nullptr;

    delete g_theInput;
    g_theInput = nullptr;

    delete g_theEvents;
    g_theEvents = nullptr;

    delete m_theWindow;
    m_theWindow = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void App::RunFrame()
{
	BeginFrame();     //engine only
	Update();//game only
	Render();         //game only
	EndFrame();	      //engine only
}

//////////////////////////////////////////////////////////////////////////
bool App::HandleQuitRequisted()
{
	m_isQuiting = true;
	return true;
}

//////////////////////////////////////////////////////////////////////////
Vec2 App::GetWindowDimensions() const
{
    float width = (float)m_theWindow->GetClientWidth();
    float height = (float)m_theWindow->GetClientHeight();
    return Vec2(width, height);
}

//////////////////////////////////////////////////////////////////////////
void App::BeginFrame()
{
    Clock::BeginFrame();

    m_theWindow->BeginFrame();
	g_theInput->BeginFrame();
	g_theConsole->BeginFrame();
	g_theRenderer->BeginFrame();	
	g_theAudio->BeginFrame();	

    DebugRenderBeginFrame();
}

//////////////////////////////////////////////////////////////////////////
void App::Update()
{
    if (m_theWindow->IsQuiting())
    {
        HandleQuitRequisted();
        return;
    }

	m_theGame->Update();
    g_theConsole->Update();
}

//////////////////////////////////////////////////////////////////////////
void App::Render() const
{	
	m_theGame->Render();
    g_theConsole->Render(g_theRenderer);
    DebugRenderScreenTo(g_theRenderer->GetFrameColorTarget());
}

//////////////////////////////////////////////////////////////////////////
void App::EndFrame()
{
    DebugRenderEndFrame();

    g_theConsole->EndFrame();
    g_theAudio->EndFrame();
    g_theInput->EndFrame();
    g_theRenderer->EndFrame();
    m_theWindow->EndFrame();
}

