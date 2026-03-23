#pragma once
#include "GenericCommand.h"
#include "MinecraftServer.h"
#include "Common/Consoles_App.h"
#include "Common/App_enums.h"
#include "Windows64/Windows64_App.h"
#include "PlayerList.h"
#include "../Minecraft.Server/Utils/Logger.h"

class OPCommand : public GenericCommand {
public:
	OPCommand(const wchar_t* _name, const wchar_t* _description) : GenericCommand(_name, _description) {};

	void Execute(CommandSenderType senderType, std::vector<std::wstring>& args, std::shared_ptr<ServerPlayer> player) override {
		if (senderType == CommandSenderType::PlayerSender) {
			player->sendMessage(L"This Command Must Be Ran From Console");
			return;
		}

		std::shared_ptr<ServerPlayer> foundPlayer = MinecraftServer::getInstance()->getPlayers()->getPlayer(args[0]);

		if (!foundPlayer) {
			Logger::Info("Unable To Find Player By That Name");
			return;
		}

		if (CommandDispatcher::ChangeOperatorState(foundPlayer->getOnlineXuid(), true)) {
			foundPlayer->sendMessage(L"You Are Now Operator");
			Logger::Info("Successfully Made Player Operator");
		}
	}
};