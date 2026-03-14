#pragma once

#ifdef _WIN64
#include <d3d11.h>
#endif

class Minecraft;
class Entity;
class Random;
class FloatBuffer;
class DataLayer;
class SparseLightStorage;
class CompressedTileStorage;
class SparseDataStorage;

#include "..\Minecraft.World\SmoothFloat.h"
#include "..\Minecraft.World\C4JThread.h"
#include "ResourceLocation.h"

class GameRenderer
{
private:
	static ResourceLocation RAIN_LOCATION;
    static ResourceLocation SNOW_LOCATION;

public:
	
	static const int NUM_LIGHT_TEXTURES = 4;// * 3;

	GameRenderer(Minecraft* mc);
	~GameRenderer();


#ifdef MULTITHREAD_ENABLE
	static C4JThread*	m_updateThread;
	static int runUpdate(LPVOID lpParam);
	static C4JThread::EventArray* m_updateEvents;
	enum EUpdateEvents
	{
		eUpdateCanRun,
		eUpdateEventIsFinished,
		eUpdateEventCount,
	};
	static bool			nearThingsToDo;
	static bool			updateRunning;
#endif
	static vector<byte *> m_deleteStackByte;
	static vector<SparseLightStorage *> m_deleteStackSparseLightStorage;
	static vector<CompressedTileStorage *> m_deleteStackCompressedTileStorage;
	static vector<SparseDataStorage *> m_deleteStackSparseDataStorage;
	static CRITICAL_SECTION m_csDeleteStack;
	static void         AddForDelete(byte *deleteThis);
	static void         AddForDelete(SparseLightStorage *deleteThis);
	static void         AddForDelete(CompressedTileStorage *deleteThis);
	static void         AddForDelete(SparseDataStorage *deleteThis);
	static void			FinishedReassigning();
	void				EnableUpdateThread();
	void				DisableUpdateThread();
};
