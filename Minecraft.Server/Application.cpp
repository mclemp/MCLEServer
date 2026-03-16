#include "stdafx.h"
#include <Utils/Logger.h>
#include "../Minecraft.Client/Windows64/Windows64_Minecraft.h"

int main() {
	//make things resolve correctly, taken from source and moved
	{
		char szExeDir[MAX_PATH] = {};
		GetModuleFileNameA(nullptr, szExeDir, MAX_PATH);
		char* pSlash = strrchr(szExeDir, '\\');
		if (pSlash) { *(pSlash + 1) = '\0'; SetCurrentDirectoryA(szExeDir); }
	}

	Logger::Info("  /\\     /\\     /\\     /\\   ");
	Logger::Info(" /--\\   /--\\   /--\\   /--\\  ");
	Logger::Info("/    \\ /    \\ /    \\ /    \\ ");
	Logger::Info("Version: Pre-Release 01"); //todo: make the version text better

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	//SetConsoleMode(hConsole, ENABLE_VIRTUAL_TERMINAL_PROCESSING); //todo: find out why this breaks \n in printf calls

	Windows64Minecraft::StartDedicatedServer();
	return 0;
}