#pragma once
#include "GenericCommand.h"
#include "MinecraftServer.h"
#include "Common/Consoles_App.h"
#include "Common/App_enums.h"
#include "Windows64/Windows64_App.h"
#include "PlayerList.h"
#include "../Minecraft.Server/Utils/Logger.h"

class StopCommand : public GenericCommand {
public:
	StopCommand(const wchar_t* _name, const wchar_t* _description) : GenericCommand(_name, _description) {};

	void Execute(CommandSenderType senderType, std::vector<std::wstring>& args, std::shared_ptr<ServerPlayer> player) override {
		if (senderType == CommandSenderType::PlayerSender) {
			if (!CommandDispatcher::IsPlayerOperator(player->getOnlineXuid())) {
				player->sendMessage(CommandDispatcher::NoPermissionMessage);
				return;
			}
		}

		MinecraftServer::getInstance()->getPlayerList()->broadcastAll(std::make_shared<ChatPacket>(L"Stopping Server..."));
		Logger::Info("Stopping Server...");
		app.SetXuiServerAction(1, eXuiServerAction_SaveGame);
		app.SetXuiServerAction(1, eXuiServerAction_StopServer);
	}
};