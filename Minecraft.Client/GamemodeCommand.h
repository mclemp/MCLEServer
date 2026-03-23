#pragma once
#include "GenericCommand.h"
#include "MinecraftServer.h"
#include "Common/Consoles_App.h"
#include "Common/App_enums.h"
#include "Windows64/Windows64_App.h"
#include "PlayerList.h"
#include "../Minecraft.Server/Utils/Logger.h"

class GamemodeCommand : public GenericCommand {
public:
	GamemodeCommand(const wchar_t* _name, const wchar_t* _description) : GenericCommand(_name, _description) {};

	void Execute(CommandSenderType senderType, std::vector<std::wstring>& args, std::shared_ptr<ServerPlayer> player) override {
		
	}
};