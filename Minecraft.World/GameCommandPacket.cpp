#include "stdafx.h"
#include "InputOutputStream.h"
#include "PacketListener.h"
#include "BasicTypeContainers.h"
#include "GameCommandPacket.h"

GameCommandPacket::GameCommandPacket() { }

GameCommandPacket::~GameCommandPacket() { }

void GameCommandPacket::read(DataInputStream *dis) { }

void GameCommandPacket::write(DataOutputStream *dos) { }

void GameCommandPacket::handle(PacketListener *listener) { }

int GameCommandPacket::getEstimatedSize()
{
	return 0;
}