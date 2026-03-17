#pragma once
#include <string>
#include <vector>
#include "GenericCommand.h"

class CommandDispatcher {
public:
	static const std::wstring NoPermissionMessage;

	CommandDispatcher(MinecraftServer* server);
	~CommandDispatcher();

	static bool OnConsoleCommand(const std::wstring& rawMessage);
	static bool OnPlayerCommand(std::shared_ptr<ServerPlayer> sender, const std::wstring& rawMessage);
	static bool IsPlayerOperator(PlayerUID player);

	static bool ChangeOperatorState(PlayerUID xuid, bool newValue);
private:
	static std::vector<GenericCommand*> registeredCommands;

	static CRITICAL_SECTION m_operatorPlayersCS;

	//todo: add actual permission system
	static std::vector<PlayerUID> operatorPlayerXUIDS;

};