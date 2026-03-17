#pragma once
using namespace std;

#include "Packet.h"

class GameCommandPacket : public Packet, public enable_shared_from_this<GameCommandPacket>
{
public:
	GameCommandPacket();
	~GameCommandPacket();

	virtual void read(DataInputStream *dis);
	virtual void write(DataOutputStream *dos);
	virtual void handle(PacketListener *listener);
	virtual int getEstimatedSize();

public:
	static shared_ptr<Packet> create() { return std::make_shared<GameCommandPacket>(); }
	virtual int getId() { return 167; }
};