#pragma once
#include <string>

typedef unsigned long long PlayerUID;

class Windows64Minecraft {
public:
	static PlayerUID ResolvePersistentXuidFromName(const std::wstring& playerName);
	static void StartDedicatedServer();

	
};