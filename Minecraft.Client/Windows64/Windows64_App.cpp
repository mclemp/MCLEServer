#include "stdafx.h"
#include "..\Common\Consoles_App.h"
#include "..\User.h"
#include "..\..\Minecraft.Client\Minecraft.h"
#include "..\..\Minecraft.Client\MinecraftServer.h"
#include "..\..\Minecraft.Client\PlayerList.h"
#include "..\..\Minecraft.Client\ServerPlayer.h"
#include "..\..\Minecraft.World\Level.h"
#include "..\..\Minecraft.World\LevelSettings.h"
#include "..\..\Minecraft.World\BiomeSource.h"
#include "..\..\Minecraft.World\LevelType.h"

CConsoleMinecraftApp app;

CConsoleMinecraftApp::CConsoleMinecraftApp() : CMinecraftApp()
{
	m_bShutdown = false;
}

void CConsoleMinecraftApp::SetRichPresenceContext(int iPad, int contextId)
{
	ProfileManager.SetRichPresenceContextValue(iPad,CONTEXT_GAME_STATE,contextId);
}

void CConsoleMinecraftApp::StoreLaunchData()
{
}
void CConsoleMinecraftApp::ExitGame()
{
	m_bShutdown = true;
}
void CConsoleMinecraftApp::FatalLoadError()
{
}

void CConsoleMinecraftApp::CaptureSaveThumbnail()
{
	RenderManager.CaptureThumbnail(&m_ThumbnailBuffer);
}
void CConsoleMinecraftApp::GetSaveThumbnail(PBYTE *pbData,DWORD *pdwSize)
{
	// On a save caused by a create world, the thumbnail capture won't have happened
	if (m_ThumbnailBuffer.Allocated())
	{
		if (pbData)
		{
			*pbData  = new BYTE[m_ThumbnailBuffer.GetBufferSize()];
			*pdwSize = m_ThumbnailBuffer.GetBufferSize();
			memcpy(*pbData, m_ThumbnailBuffer.GetBufferPointer(), *pdwSize);
		}
		m_ThumbnailBuffer.Release();
	}
	else
	{
		// No capture happened (e.g. first save on world creation) leave thumbnail as nullptr
		if (pbData)  *pbData  = nullptr;
		if (pdwSize) *pdwSize = 0;
	}
}
void CConsoleMinecraftApp::ReleaseSaveThumbnail()
{
}

void CConsoleMinecraftApp::GetScreenshot(int iPad,PBYTE *pbData,DWORD *pdwSize)
{
}

void CConsoleMinecraftApp::TemporaryCreateGameStart()
{

}

int CConsoleMinecraftApp::GetLocalTMSFileIndex(WCHAR *wchTMSFile,bool bFilenameIncludesExtension,eFileExtensionType eEXT)
{
	return -1;
}

int CConsoleMinecraftApp::LoadLocalTMSFile(WCHAR *wchTMSFile)
{
	return -1;
}

int CConsoleMinecraftApp::LoadLocalTMSFile(WCHAR *wchTMSFile, eFileExtensionType eExt)
{
	return -1;
}

void CConsoleMinecraftApp::FreeLocalTMSFiles(eTMSFileType eType)
{
}
