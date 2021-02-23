#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>			// #include this (massive, platform-specific) header in very few places
#include <math.h>
#include <cassert>
#include <crtdbg.h>
#include "Game/App.hpp"
#include "Engine/Platform/Window.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/NamedStrings.hpp"

extern App* g_theApp;

//////////////////////////////////////////////////////////////////////////
void Startup()
{
	std::string filePath = "Data/GameConfig.xml";
	XmlDocument xmlDoc;
	XmlError code = xmlDoc.LoadFile( filePath.c_str() );
    if (code != XmlError::XML_SUCCESS)
    {
        std::string errorMessage = "Error when loading xml file: ";
        errorMessage += filePath;
        errorMessage += " \n Error code is ";
        errorMessage += std::to_string((int)code);
        ERROR_AND_DIE(errorMessage);
        return;
    }

	g_gameConfigBlackboard = new NamedStrings();
	g_gameConfigBlackboard->PopulateFromXmlElementAttributes( *xmlDoc.RootElement() );
}

//-----------------------------------------------------------------------------------------------
int WINAPI WinMain( _In_ HINSTANCE applicationInstanceHandle, _In_opt_ HINSTANCE, _In_ LPSTR commandLineString, _In_ int)
{
	UNUSED( applicationInstanceHandle );
	UNUSED( commandLineString );

	Startup();

	g_theApp = new App();
	g_theApp->Startup();

	while( !g_theApp->IsQuiting() )
	{
		//Sleep(16);
		g_theApp->RunFrame();
	}

	g_theApp->Shutdown();
	delete g_theApp;
	g_theApp = nullptr;

    delete g_gameConfigBlackboard;
    g_gameConfigBlackboard = nullptr;

	return 0;
}


