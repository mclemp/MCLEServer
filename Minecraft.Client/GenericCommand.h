#pragma once
#include <memory>
#include <vector>
#include <ServerPlayer.h>

enum CommandSenderType {
	ConsoleSender,
	PlayerSender,

	CommandSender_Count
};

class GenericCommand {
public:
	GenericCommand(const wchar_t* _name, const wchar_t* _description) : name(_name), description(_description) {};
	~GenericCommand() = default;
	
	virtual void Execute(CommandSenderType senderType, std::vector<std::wstring>& args, std::shared_ptr<ServerPlayer> player) {};

	const wchar_t* GetName() const { return this->name; }
private:
	const wchar_t* name;
	const wchar_t* description;
};