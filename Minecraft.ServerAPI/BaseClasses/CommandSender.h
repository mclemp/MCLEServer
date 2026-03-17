#pragma once

enum CommandSenderType : char {
	Player,
	Console,

	CommandSender_Count
};

class CommandSender {
public:
	CommandSenderType GetType() {
		return this->type;
	}
private:
	CommandSenderType type;
};