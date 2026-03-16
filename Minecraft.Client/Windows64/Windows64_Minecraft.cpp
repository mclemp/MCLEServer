
#include "stdafx.h"

#include <assert.h>
#include <iostream>
#include <ShellScalingApi.h>
#include <shellapi.h>
#include "GameConfig\Minecraft.spa.h"
#include "..\MinecraftServer.h"
#include "..\LocalPlayer.h"
#include "..\..\Minecraft.World\ItemInstance.h"
#include "..\..\Minecraft.World\MapItem.h"
#include "..\..\Minecraft.World\Recipes.h"
#include "..\..\Minecraft.World\Recipy.h"
#include "..\..\Minecraft.World\Language.h"
#include "..\..\Minecraft.World\StringHelpers.h"
#include "..\..\Minecraft.World\AABB.h"
#include "..\..\Minecraft.World\Vec3.h"
#include "..\..\Minecraft.World\Level.h"
#include "..\..\Minecraft.World\net.minecraft.world.level.tile.h"

#include "..\ClientConnection.h"
#include "..\Minecraft.h"
#include "..\ChatScreen.h"
#include "KeyboardMouseInput.h"
#include "..\User.h"
#include "..\..\Minecraft.World\Socket.h"
#include "..\..\Minecraft.World\ThreadName.h"
#include "..\..\Minecraft.Client\StatsCounter.h"
#include "..\ConnectScreen.h"
//#include "Social\SocialManager.h"
//#include "Leaderboards\LeaderboardManager.h"
//#include "XUI\XUI_Scene_Container.h"
//#include "NetworkManager.h"
#include "..\..\Minecraft.Client\Tesselator.h"
#include "..\..\Minecraft.Client\Options.h"
#include "Sentient\SentientManager.h"
#include "..\..\Minecraft.World\IntCache.h"
#include "..\Textures.h"
#include "..\Settings.h"
#include "Resource.h"
#include "..\..\Minecraft.World\compression.h"
#include "..\..\Minecraft.World\OldChunkStorage.h"
#include "..\GameRenderer.h"
#include "Network\WinsockNetLayer.h"
#include "Windows64_Xuid.h"
#include "Windows64_Minecraft.h"
#include "../../Minecraft.Server/Utils/Logger.h"

// Forward-declare the internal Renderer class and its global instance from 4J_Render_PC_d.lib.
// C4JRender (RenderManager) is a stateless wrapper — all D3D state lives in InternalRenderManager.
class Renderer;
extern Renderer InternalRenderManager;

#ifdef _MSC_VER
#pragma comment(lib, "legacy_stdio_definitions.lib")
#endif

#define THEME_NAME		"584111F70AAAAAAA"
#define THEME_FILESIZE	2797568

#define FIFTY_ONE_MB (1000000*51) // Maximum TCR space required for a save is 52MB (checking for this on a selected device)


#define NUM_PROFILE_VALUES	5
#define NUM_PROFILE_SETTINGS 4
DWORD dwProfileSettingsA[NUM_PROFILE_VALUES] = { 0,0,0,0,0 };

char g_Win64Username[17] = { 0 };
wchar_t g_Win64UsernameW[17] = { 0 };

//#define MEMORY_TRACKING

#ifdef MEMORY_TRACKING
void ResetMem();
void DumpMem();
void MemPixStuff();
#else
void MemSect(int sect)
{
}
#endif


// 4J Stu - These functions are referenced from the Windows Input library
void ClearGlobalText() { }
uint16_t* GetGlobalText() { return nullptr; }
void SeedEditBox() { }


static Minecraft* InitialiseMinecraftRuntime()
{
	//app.loadMediaArchive();
	//app.loadStringTable();

	ProfileManager.Initialise(TITLEID_MINECRAFT,
		app.m_dwOfferID,
		PROFILE_VERSION_10,
		NUM_PROFILE_VALUES,
		NUM_PROFILE_SETTINGS,
		dwProfileSettingsA,
		app.GAME_DEFINED_PROFILE_DATA_BYTES * XUSER_MAX_COUNT,
		&app.uiGameDefinedDataChangedBitmask
	);
	ProfileManager.SetDefaultOptionsCallback(&CConsoleMinecraftApp::DefaultOptionsCallback, (LPVOID)&app);

	g_NetworkManager.Initialise();

	for (int i = 0; i < MINECRAFT_NET_MAX_PLAYERS; i++)
	{
		IQNet::m_player[i].m_smallId = static_cast<BYTE>(i);
		IQNet::m_player[i].m_isRemote = false;
		IQNet::m_player[i].m_isHostPlayer = (i == 0);
		swprintf_s(IQNet::m_player[i].m_gamertag, 32, L"Player%d", i);
	}
	wcscpy_s(IQNet::m_player[0].m_gamertag, 32, g_Win64UsernameW);

	WinsockNetLayer::Initialize();

	ProfileManager.SetDebugFullOverride(true);

	Tesselator::CreateNewThreadStorage(1024 * 1024);
	AABB::CreateNewThreadStorage();
	Vec3::CreateNewThreadStorage();
	IntCache::CreateNewThreadStorage();
	Compression::CreateNewThreadStorage();
	OldChunkStorage::CreateNewThreadStorage();
	Level::enableLightingCache();
	Tile::CreateNewThreadStorage();

	Minecraft::main();
	Minecraft* pMinecraft = Minecraft::GetInstance();
	if (pMinecraft == nullptr)
		return nullptr;

	//app.InitGameSettings();
	app.InitialiseTips();

	return pMinecraft;
}

static int HeadlessServerConsoleThreadProc(void* lpParameter)
{
	UNREFERENCED_PARAMETER(lpParameter);

	std::string line;
	while (!app.m_bShutdown)
	{
		if (!std::getline(std::cin, line))
		{
			if (std::cin.eof())
			{
				break;
			}

			std::cin.clear();
			Sleep(50);
			continue;
		}

		wstring command = trimString(convStringToWstring(line));
		if (command.empty())
			continue;

		MinecraftServer* server = MinecraftServer::getInstance();
		if (server != nullptr)
		{
			server->handleConsoleInput(command, server);
		}
	}

	return 0;
}

void Windows64Minecraft::StartDedicatedServer() {
	__int64 startupTime = System::currentRealTimeMillis();

	Settings serverSettings(new File(L"server.properties"));
	const wstring configuredBindIp = serverSettings.getString(L"server-ip", L"");

	const char* bindIp = "0.0.0.0";

	if (g_Win64DedicatedServerBindIP[0] != 0)
		bindIp = g_Win64DedicatedServerBindIP;
	else if (!configuredBindIp.empty())
		bindIp = wstringtochararray(configuredBindIp);

	const int port = g_Win64DedicatedServerPort > 0 ? g_Win64DedicatedServerPort : serverSettings.getInt(L"server-port", WIN64_NET_DEFAULT_PORT);
	const std::string addressCombo = std::string(bindIp) + ":" + std::to_string(port);

	Logger::Info(("Starting QuadA On " + addressCombo).c_str());

	strncpy_s(g_Win64Username, sizeof(g_Win64Username), "Player", _TRUNCATE);
	MultiByteToWideChar(CP_ACP, 0, g_Win64Username, -1, g_Win64UsernameW, 17);

	const Minecraft* pMinecraft = InitialiseMinecraftRuntime();
	if (pMinecraft == nullptr)
	{
		Logger::Error("Failed To Initialise Minecraft Runtime");
		return;
	}

	app.SetGameHostOption(eGameHostOption_Difficulty, serverSettings.getInt(L"difficulty", 1));
	app.SetGameHostOption(eGameHostOption_Gamertags, serverSettings.getBoolean(L"show-gamertags", true) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_GameType, serverSettings.getInt(L"gamemode", 0));
	app.SetGameHostOption(eGameHostOption_LevelType, serverSettings.getBoolean(L"superflat", false) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_Structures, serverSettings.getBoolean(L"generate-structures", true) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_BonusChest, serverSettings.getBoolean(L"bonus-chest", false) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_PvP, serverSettings.getBoolean(L"pvp", true) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_TrustPlayers, serverSettings.getBoolean(L"trust-players", true) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_FireSpreads, serverSettings.getBoolean(L"fire-spreads", true) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_TNT, serverSettings.getBoolean(L"tnt", true) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_HostCanFly, 1);
	app.SetGameHostOption(eGameHostOption_HostCanChangeHunger, 1);
	app.SetGameHostOption(eGameHostOption_HostCanBeInvisible, 1);
	app.SetGameHostOption(eGameHostOption_MobGriefing, serverSettings.getBoolean(L"mob-griefing", true) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_KeepInventory, serverSettings.getBoolean(L"keep-inventory", false) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_DoMobSpawning, 1);
	app.SetGameHostOption(eGameHostOption_DoMobLoot, 1);
	app.SetGameHostOption(eGameHostOption_DoTileDrops, 1);
	app.SetGameHostOption(eGameHostOption_NaturalRegeneration, serverSettings.getBoolean(L"natural-regeneration", true) ? 1 : 0);
	app.SetGameHostOption(eGameHostOption_DoDaylightCycle, serverSettings.getBoolean(L"daylight-cycle", true) ? 1 : 0);


	MinecraftServer::resetFlags();
	g_NetworkManager.HostGame(0, false, true, MINECRAFT_NET_MAX_PLAYERS, 0);

	if (!WinsockNetLayer::IsActive())
	{
		Logger::Error(("Failed To Activate Socket Binding On" + addressCombo).c_str());
		return;
	}

	g_NetworkManager.FakeLocalPlayerJoined();

	NetworkGameInitData* param = new NetworkGameInitData();
	param->seed = serverSettings.getInt(L"seed", 0);

	std::wstring WorldSize = serverSettings.getString(L"world-size", L"small");

	if (WorldSize == L"classic") {
		param->hellScale = HELL_LEVEL_SCALE_CLASSIC;
		param->xzSize = LEVEL_WIDTH_CLASSIC;
		app.SetGameHostOption(eGameHostOption_WorldSize, 1);
	}
	else if (WorldSize == L"small") {
		param->hellScale = HELL_LEVEL_SCALE_SMALL;
		param->xzSize = LEVEL_WIDTH_SMALL;
		app.SetGameHostOption(eGameHostOption_WorldSize, 2);
	}
	else if (WorldSize == L"medium") {
		param->hellScale = HELL_LEVEL_SCALE_MEDIUM;
		param->xzSize = LEVEL_WIDTH_MEDIUM;
		app.SetGameHostOption(eGameHostOption_WorldSize, 3);
	}
	else if (WorldSize == L"large") {
		param->hellScale = HELL_LEVEL_SCALE_LARGE;
		param->xzSize = LEVEL_WIDTH_LARGE;
		app.SetGameHostOption(eGameHostOption_WorldSize, 4);
	}

	//param->savePlatform = SAVE_FILE_PLATFORM_X360;

	param->settings = app.GetGameHostOption(eGameHostOption_All);

	g_NetworkManager.ServerStoppedCreate(true);
	g_NetworkManager.ServerReadyCreate(true);

	C4JThread* thread = new C4JThread(&CGameNetworkManager::ServerThreadProc, param, "Server", 256 * 1024);
	thread->SetProcessor(CPU_CORE_SERVER);
	thread->Run();

	g_NetworkManager.ServerReadyWait();
	g_NetworkManager.ServerReadyDestroy();

	if (MinecraftServer::serverHalted())
	{
		Logger::Error("MinecraftServer Has Haulted During Startup");
		g_NetworkManager.LeaveGame(false);
		return;
	}

	app.SetGameStarted(true);
	g_NetworkManager.DoWork();

	double finishedStartupTime = ((System::currentRealTimeMillis() - startupTime) / 1000.0);

	Logger::Info(("Server Has Started In: " + std::to_string(finishedStartupTime)).c_str());

	C4JThread* consoleThread = new C4JThread(&HeadlessServerConsoleThreadProc, nullptr, "Server console", 128 * 1024);
	consoleThread->Run();

	MSG msg = { 0 };
	while (WM_QUIT != msg.message && !app.m_bShutdown && !MinecraftServer::serverHalted())
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}

		app.UpdateTime();
		ProfileManager.Tick();
		StorageManager.Tick();
		g_NetworkManager.DoWork();
		app.HandleXuiActions();

		Sleep(10);
	}

	printf("Stopping server...\n");
	fflush(stdout);

	app.m_bShutdown = true;
	MinecraftServer::HaltServer();
	g_NetworkManager.LeaveGame(false);
	return;
}

#ifdef MEMORY_TRACKING

int totalAllocGen = 0;
unordered_map<int, int> allocCounts;
bool trackEnable = false;
bool trackStarted = false;
volatile size_t sizeCheckMin = 1160;
volatile size_t sizeCheckMax = 1160;
volatile int sectCheck = 48;
CRITICAL_SECTION memCS;
DWORD tlsIdx;

LPVOID XMemAlloc(SIZE_T dwSize, DWORD dwAllocAttributes)
{
	if (!trackStarted)
	{
		void* p = XMemAllocDefault(dwSize, dwAllocAttributes);
		size_t realSize = XMemSizeDefault(p, dwAllocAttributes);
		totalAllocGen += realSize;
		return p;
	}

	EnterCriticalSection(&memCS);

	void* p = XMemAllocDefault(dwSize + 16, dwAllocAttributes);
	size_t realSize = XMemSizeDefault(p, dwAllocAttributes) - 16;

	if (trackEnable)
	{
#if 1
		int sect = ((int)TlsGetValue(tlsIdx)) & 0x3f;
		*(((unsigned char*)p) + realSize) = sect;

		if ((realSize >= sizeCheckMin) && (realSize <= sizeCheckMax) && ((sect == sectCheck) || (sectCheck == -1)))
		{
			app.DebugPrintf("Found one\n");
		}
#endif

		if (p)
		{
			totalAllocGen += realSize;
			trackEnable = false;
			int key = (sect << 26) | realSize;
			int oldCount = allocCounts[key];
			allocCounts[key] = oldCount + 1;

			trackEnable = true;
		}
	}

	LeaveCriticalSection(&memCS);

	return p;
}

void* operator new (size_t size)
{
	return (unsigned char*)XMemAlloc(size, MAKE_XALLOC_ATTRIBUTES(0, FALSE, TRUE, FALSE, 0, XALLOC_PHYSICAL_ALIGNMENT_DEFAULT, XALLOC_MEMPROTECT_READWRITE, FALSE, XALLOC_MEMTYPE_HEAP));
}

void operator delete (void* p)
{
	XMemFree(p, MAKE_XALLOC_ATTRIBUTES(0, FALSE, TRUE, FALSE, 0, XALLOC_PHYSICAL_ALIGNMENT_DEFAULT, XALLOC_MEMPROTECT_READWRITE, FALSE, XALLOC_MEMTYPE_HEAP));
}

void WINAPI XMemFree(PVOID pAddress, DWORD dwAllocAttributes)
{
	bool special = false;
	if (dwAllocAttributes == 0)
	{
		dwAllocAttributes = MAKE_XALLOC_ATTRIBUTES(0, FALSE, TRUE, FALSE, 0, XALLOC_PHYSICAL_ALIGNMENT_DEFAULT, XALLOC_MEMPROTECT_READWRITE, FALSE, XALLOC_MEMTYPE_HEAP);
		special = true;
	}
	if (!trackStarted)
	{
		size_t realSize = XMemSizeDefault(pAddress, dwAllocAttributes);
		XMemFreeDefault(pAddress, dwAllocAttributes);
		totalAllocGen -= realSize;
		return;
	}
	EnterCriticalSection(&memCS);
	if (pAddress)
	{
		size_t realSize = XMemSizeDefault(pAddress, dwAllocAttributes) - 16;

		if (trackEnable)
		{
			int sect = *(((unsigned char*)pAddress) + realSize);
			totalAllocGen -= realSize;
			trackEnable = false;
			int key = (sect << 26) | realSize;
			int oldCount = allocCounts[key];
			allocCounts[key] = oldCount - 1;
			trackEnable = true;
		}
		XMemFreeDefault(pAddress, dwAllocAttributes);
	}
	LeaveCriticalSection(&memCS);
}

SIZE_T WINAPI XMemSize(
	PVOID pAddress,
	DWORD dwAllocAttributes
)
{
	if (trackStarted)
	{
		return XMemSizeDefault(pAddress, dwAllocAttributes) - 16;
	}
	else
	{
		return XMemSizeDefault(pAddress, dwAllocAttributes);
	}
}

void DumpMem()
{
	int totalLeak = 0;
	for (auto it = allocCounts.begin(); it != allocCounts.end(); it++)
	{
		if (it->second > 0)
		{
			app.DebugPrintf("%d %d %d %d\n", (it->first >> 26) & 0x3f, it->first & 0x03ffffff, it->second, (it->first & 0x03ffffff) * it->second);
			totalLeak += (it->first & 0x03ffffff) * it->second;
		}
	}
	app.DebugPrintf("Total %d\n", totalLeak);
}

void ResetMem()
{
	if (!trackStarted)
	{
		trackEnable = true;
		trackStarted = true;
		totalAllocGen = 0;
		InitializeCriticalSection(&memCS);
		tlsIdx = TlsAlloc();
	}
	EnterCriticalSection(&memCS);
	trackEnable = false;
	allocCounts.clear();
	trackEnable = true;
	LeaveCriticalSection(&memCS);
}

void MemSect(int section)
{
	unsigned int value = (unsigned int)TlsGetValue(tlsIdx);
	if (section == 0) // pop
	{
		value = (value >> 6) & 0x03ffffff;
	}
	else
	{
		value = (value << 6) | section;
	}
	TlsSetValue(tlsIdx, (LPVOID)value);
}

void MemPixStuff()
{
	const int MAX_SECT = 46;

	int totals[MAX_SECT] = { 0 };

	for (auto it = allocCounts.begin(); it != allocCounts.end(); it++)
	{
		if (it->second > 0)
		{
			int sect = (it->first >> 26) & 0x3f;
			int bytes = it->first & 0x03ffffff;
			totals[sect] += bytes * it->second;
		}
	}

	unsigned int allSectsTotal = 0;
	for (int i = 0; i < MAX_SECT; i++)
	{
		allSectsTotal += totals[i];
		PIXAddNamedCounter(((float)totals[i]) / 1024.0f, "MemSect%d", i);
	}

	PIXAddNamedCounter(((float)allSectsTotal) / (4096.0f), "MemSect total pages");
}

#endif
