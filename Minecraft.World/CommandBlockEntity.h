#pragma once

#include "TileEntity.h"
#include "Class.h"

class ChatMessageComponent;

class CommandBlockEntity : public TileEntity
{
public:
	eINSTANCEOF GetType() { return eTYPE_COMMANDBLOCKTILEENTITY; }
	static TileEntity *create() { return new CommandBlockEntity(); }

	// 4J Added
	virtual shared_ptr<TileEntity> clone();

private:
	int successCount;
	wstring command;
	wstring name;

public:
	CommandBlockEntity();

	void setCommand(const wstring &command);
	wstring getCommand();
	int performCommand(Level *level);
	wstring getName();
	void setName(const wstring &name);
	void save(CompoundTag *tag);
	void load(CompoundTag *tag);
	Pos *getCommandSenderWorldPosition();
	Level *getCommandSenderWorld();
	shared_ptr<Packet> getUpdatePacket();
	int getSuccessCount();
	void setSuccessCount(int successCount);
};