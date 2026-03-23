#include "stdafx.h"
#include "CommandDispatcher.h"
#include <sstream>
#include "../Minecraft.World/StringHelpers.h"

#include "OPCommand.h"
#include "TeleportCommand.h"
#include "StopCommand.h"
#include "SaveCommand.h"

CRITICAL_SECTION CommandDispatcher::m_operatorPlayersCS;
std::vector<PlayerUID> CommandDispatcher::operatorPlayerXUIDS;

std::vector<GenericCommand*> CommandDispatcher::registeredCommands = {
	new OPCommand(L"op", L""),
	new TeleportCommand(L"teleport", L""),
	new StopCommand(L"stop", L""),
	new SaveCommand(L"save", L""),
};

const std::wstring CommandDispatcher::NoPermissionMessage = L"No Permission To Run This Command";

CommandDispatcher::CommandDispatcher(MinecraftServer* server) {
	InitializeCriticalSection(&m_operatorPlayersCS);
}

CommandDispatcher::~CommandDispatcher()
{
	DeleteCriticalSection(&m_operatorPlayersCS);
}

vector<wstring> SplitConsoleCommand(const std::wstring& command);

bool CommandDispatcher::OnConsoleCommand(const std::wstring& rawMessage) {
	wstring command = trimString(rawMessage);
	if (command.empty()) return true;

	if (command[0] == L'/') command = trimString(command.substr(1));

	vector<std::wstring> args = SplitConsoleCommand(command);
	if (args.empty()) return true;

	const std::wstring commandName = toLower(args[0]);

	args.erase(args.begin()); //remove command name from args

	for (int i = 0; i < CommandDispatcher::registeredCommands.size(); i++) {
		GenericCommand* command = CommandDispatcher::registeredCommands[i];

		if (lstrcmpW(command->GetName(), commandName.c_str()) == 0) {
			command->Execute(CommandSenderType::ConsoleSender, args, nullptr);
			break;
		}
	}

	return true;
}

bool CommandDispatcher::OnPlayerCommand(std::shared_ptr<ServerPlayer> sender, const std::wstring& rawMessage) {
	wstring command = trimString(rawMessage);
	if (command.empty()) return true;

	if (command[0] == L'/') command = trimString(command.substr(1));

	vector<std::wstring> args = SplitConsoleCommand(command);
	if (args.empty()) return true;

	const std::wstring commandName = toLower(args[0]);

	args.erase(args.begin()); //remove command name from args

	for (int i = 0; i < CommandDispatcher::registeredCommands.size(); i++) {
		GenericCommand* command = CommandDispatcher::registeredCommands[i];

		if (lstrcmpW(command->GetName(), commandName.c_str()) == 0) {
			command->Execute(CommandSenderType::PlayerSender, args, sender);
			break;
		}
	}

	return true;
}

bool CommandDispatcher::IsPlayerOperator(PlayerUID xuid) {
	bool returnValue = false;
	EnterCriticalSection(&m_operatorPlayersCS);
	for (int i = 0; i < CommandDispatcher::operatorPlayerXUIDS.size(); i++) {
		PlayerUID savedUID = CommandDispatcher::operatorPlayerXUIDS[i];
		if (xuid == savedUID) {
			returnValue = true;
			break;
		}
	}
	LeaveCriticalSection(&m_operatorPlayersCS);

	return returnValue;
}

bool CommandDispatcher::ChangeOperatorState(PlayerUID xuid, bool newValue) {
	bool currentState = IsPlayerOperator(xuid);
	bool returnValue = false;
	EnterCriticalSection(&m_operatorPlayersCS);

	if (newValue && !currentState) {
		CommandDispatcher::operatorPlayerXUIDS.push_back(xuid);
		returnValue = true;
	} else if (!newValue && currentState) {
		auto it = std::find(CommandDispatcher::operatorPlayerXUIDS.begin(), CommandDispatcher::operatorPlayerXUIDS.end(), xuid);
		if (it != CommandDispatcher::operatorPlayerXUIDS.end()) {
			CommandDispatcher::operatorPlayerXUIDS.erase(it);
		}
		returnValue = true;
	}
	LeaveCriticalSection(&m_operatorPlayersCS);

	return returnValue;
}


std::vector<std::wstring> SplitConsoleCommand(const std::wstring& command)
{
	std::vector<std::wstring> tokens;
	std::wstring current;
	bool inQuotes = false;

	for (size_t i = 0; i < command.size(); i++) {
		wchar_t c = command[i];

		if (c == L'"') {
			inQuotes = !inQuotes;
			continue;
		}

		if (iswspace(c) && !inQuotes) {
			if (!current.empty()) {
				tokens.push_back(current);
				current.clear();
			}
		} else {
			current += c;
		}
	}

	if (!current.empty()) {
		tokens.push_back(current);
	}

	return tokens;
}