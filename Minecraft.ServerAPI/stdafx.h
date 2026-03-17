#pragma once
#define _HAS_STD_BYTE 0     // solve (std::)'byte' ambiguity with windows headers
#define WIN32_LEAN_AND_MEAN

#include <malloc.h>
#include <tchar.h>
#include <windows.h>
#include <windowsx.h>
#include <memory>
#include <unordered_map>
#include <string>

using namespace std;

#include "../Minecraft.Client/MinecraftServer.h"
#include "../Minecraft.ServerAPI/BaseClasses/Command.h"


#include <filesystem>
#include <vector>
#include <memory>
#include <unordered_map>

//using namespace std;