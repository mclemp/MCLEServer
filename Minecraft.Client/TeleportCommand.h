#pragma once
#include "GenericCommand.h"

class TeleportCommand : public GenericCommand {
public:
	TeleportCommand(const wchar_t* _name, const wchar_t* _description) : GenericCommand(_name, _description) {};

	void Execute(CommandSenderType senderType, std::vector<std::wstring>& args, std::shared_ptr<ServerPlayer> player) override {
		player->sendMessage(L"This Be A Working Command Callback");

	}
};