#pragma once
#include "CommandSender.h"

class Command {
public:
	Command(const char* _name, const char* _description) : name(_name), description(_description) {};
	~Command() = default;

	virtual void Execute(CommandSender sender, ) {}

private:
	const char* name;
	const char* description;
};