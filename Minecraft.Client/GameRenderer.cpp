#include "stdafx.h"
#include "GameRenderer.h"
#include "ItemInHandRenderer.h"
#include "LevelRenderer.h"
#include "Frustum.h"
#include "FrustumCuller.h"
#include "Textures.h"
#include "Tesselator.h"
#include "ParticleEngine.h"
#include "SmokeParticle.h"
#include "WaterDropParticle.h"
#include "GameMode.h"
#include "CreativeMode.h"
#include "Lighting.h"
#include "Options.h"
#include "MultiplayerLocalPlayer.h"
#include "GuiParticles.h"
#include "MultiPlayerLevel.h"
#include "Chunk.h"
#include "..\Minecraft.World\net.minecraft.world.entity.h"
#include "..\Minecraft.World\net.minecraft.world.entity.player.h"
#include "..\Minecraft.World\net.minecraft.world.item.enchantment.h"
#include "..\Minecraft.World\net.minecraft.world.level.h"
#include "..\Minecraft.World\net.minecraft.world.level.material.h"
#include "..\Minecraft.World\net.minecraft.world.level.tile.h"
#include "..\Minecraft.World\net.minecraft.world.level.chunk.h"
#include "..\Minecraft.World\net.minecraft.world.level.biome.h"
#include "..\Minecraft.World\net.minecraft.world.level.dimension.h"
#include "..\Minecraft.World\net.minecraft.world.phys.h"
#include "..\Minecraft.World\System.h"
#include "..\Minecraft.World\FloatBuffer.h"
#include "..\Minecraft.World\ThreadName.h"
#include "..\Minecraft.World\SparseLightStorage.h"
#include "..\Minecraft.World\CompressedTileStorage.h"
#include "..\Minecraft.World\SparseDataStorage.h"
#include "..\Minecraft.World\JavaMath.h"
#include "..\Minecraft.World\Facing.h"
#include "..\Minecraft.World\MobEffect.h"
#include "..\Minecraft.World\IntCache.h"
#include "..\Minecraft.World\SmoothFloat.h"
#include "..\Minecraft.World\MobEffectInstance.h"
#include "..\Minecraft.World\Item.h"
#include "Camera.h"
#include "..\Minecraft.World\SoundTypes.h"
#include "HumanoidModel.h"
#include "..\Minecraft.World\Item.h"
#include "..\Minecraft.World\compression.h"
#include "BossMobGuiInfo.h"

#include "TexturePackRepository.h"
#include "TexturePack.h"
#include "TextureAtlas.h"

#ifdef MULTITHREAD_ENABLE
C4JThread*		GameRenderer::m_updateThread;
C4JThread::EventArray* GameRenderer::m_updateEvents;
bool GameRenderer::nearThingsToDo = false;
bool GameRenderer::updateRunning = false;
vector<byte *> GameRenderer::m_deleteStackByte;
vector<SparseLightStorage *> GameRenderer::m_deleteStackSparseLightStorage;
vector<CompressedTileStorage *> GameRenderer::m_deleteStackCompressedTileStorage;
vector<SparseDataStorage *> GameRenderer::m_deleteStackSparseDataStorage;
#endif
CRITICAL_SECTION GameRenderer::m_csDeleteStack;

ResourceLocation GameRenderer::RAIN_LOCATION = ResourceLocation(TN_ENVIRONMENT_RAIN);
ResourceLocation GameRenderer::SNOW_LOCATION = ResourceLocation(TN_ENVIRONMENT_SNOW);

GameRenderer::GameRenderer(Minecraft *mc)
{

#ifdef MULTITHREAD_ENABLE
	m_updateEvents  = new C4JThread::EventArray(eUpdateEventCount, C4JThread::EventArray::e_modeAutoClear);
	m_updateEvents->Set(eUpdateEventIsFinished);

	InitializeCriticalSection(&m_csDeleteStack);
	m_updateThread = new C4JThread(runUpdate, nullptr, "Chunk update");
#ifdef __PS3__
	m_updateThread->SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);
#endif// __PS3__
	m_updateThread->SetProcessor(CPU_CORE_CHUNK_UPDATE);
	m_updateThread->Run();
#endif
}

// 4J Stu Added to go with 1.8.2 change
GameRenderer::~GameRenderer() { }


#ifdef MULTITHREAD_ENABLE
// Request that an item be deleted, when it is safe to do so
void GameRenderer::AddForDelete(byte *deleteThis)
{
	EnterCriticalSection(&m_csDeleteStack);
	m_deleteStackByte.push_back(deleteThis);
}

void GameRenderer::AddForDelete(SparseLightStorage *deleteThis)
{
	EnterCriticalSection(&m_csDeleteStack);
	m_deleteStackSparseLightStorage.push_back(deleteThis);
}

void GameRenderer::AddForDelete(CompressedTileStorage *deleteThis)
{
	EnterCriticalSection(&m_csDeleteStack);
	m_deleteStackCompressedTileStorage.push_back(deleteThis);
}

void GameRenderer::AddForDelete(SparseDataStorage *deleteThis)
{
	EnterCriticalSection(&m_csDeleteStack);
	m_deleteStackSparseDataStorage.push_back(deleteThis);
}

void GameRenderer::FinishedReassigning()
{
	LeaveCriticalSection(&m_csDeleteStack);
}

int GameRenderer::runUpdate(LPVOID lpParam)
{
	Minecraft *minecraft = Minecraft::GetInstance();
	Vec3::CreateNewThreadStorage();
	AABB::CreateNewThreadStorage();
	IntCache::CreateNewThreadStorage();
	Tesselator::CreateNewThreadStorage(1024*1024);
	Compression::UseDefaultThreadStorage();
	//RenderManager.InitialiseContext();
#ifdef _LARGE_WORLDS
	Chunk::CreateNewThreadStorage();
#endif
	Tile::CreateNewThreadStorage();

	while(true)
	{
		//m_updateEvents->Clear(eUpdateEventIsFinished);
		//m_updateEvents->WaitForSingle(eUpdateCanRun,INFINITE);
		// 4J Stu - We Need to have this happen atomically to avoid deadlocks
		m_updateEvents->WaitForAll(INFINITE);

		m_updateEvents->Set(eUpdateCanRun);

		//		PIXBeginNamedEvent(0,"Updating dirty chunks %d",(count++)&7);

		// Update chunks atomically until there aren't any very near ones left - they will be deferred for rendering
		// until the call to CBuffDeferredModeEnd if we have anything near to render here
		// Now limiting maximum number of updates that can be deferred as have noticed that with redstone clock circuits, it is possible to create
		// things that need constant updating, so if you stand near them, the render data Never gets updated and the game just keeps going until it runs out of render memory...
		int count = 0;
		static const int MAX_DEFERRED_UPDATES = 10;
		bool shouldContinue = false;
		do
		{
			shouldContinue = minecraft->levelRenderer->updateDirtyChunks();
			count++;
		} while ( shouldContinue && count < MAX_DEFERRED_UPDATES );

		//		while( minecraft->levelRenderer->updateDirtyChunks() )
		//			;
		RenderManager.CBuffDeferredModeEnd();

		// If any renderable tile entities were flagged in this last block of chunk(s) that were udpated, then change their
		// flags to say that this deferred chunk is over and they are actually safe to be removed now
		minecraft->levelRenderer->fullyFlagRenderableTileEntitiesToBeRemoved();

		// We've got stacks for things that can only safely be deleted whilst this thread isn't updating things - delete those things now
		EnterCriticalSection(&m_csDeleteStack);
		for(unsigned int i = 0; i < m_deleteStackByte.size(); i++ )
		{
			delete m_deleteStackByte[i];
		}
		m_deleteStackByte.clear();
		for(unsigned int i = 0; i < m_deleteStackSparseLightStorage.size(); i++ )
		{
			delete m_deleteStackSparseLightStorage[i];
		}
		m_deleteStackSparseLightStorage.clear();
		for(unsigned int i = 0; i < m_deleteStackCompressedTileStorage.size(); i++ )
		{
			delete m_deleteStackCompressedTileStorage[i];
		}
		m_deleteStackCompressedTileStorage.clear();
		for(unsigned int i = 0; i < m_deleteStackSparseDataStorage.size(); i++ )
		{
			delete m_deleteStackSparseDataStorage[i];
		}
		m_deleteStackSparseDataStorage.clear();
		LeaveCriticalSection(&m_csDeleteStack);

		//		PIXEndNamedEvent();

		AABB::resetPool();
		Vec3::resetPool();
		IntCache::Reset();
		m_updateEvents->Set(eUpdateEventIsFinished);
	}

	return 0;
}
#endif

void GameRenderer::EnableUpdateThread()
{
	// #ifdef __PS3__ // MGH - disable the update on PS3 for now
	// 	return;
	// #endif
#ifdef MULTITHREAD_ENABLE
	if( updateRunning) return;
	app.DebugPrintf("------------------EnableUpdateThread--------------------\n");
	updateRunning = true;
	m_updateEvents->Set(eUpdateCanRun);
	m_updateEvents->Set(eUpdateEventIsFinished);
#endif
}

void GameRenderer::DisableUpdateThread()
{
	// #ifdef __PS3__ // MGH - disable the update on PS3 for now
	// 	return;
	// #endif
#ifdef MULTITHREAD_ENABLE
	if( !updateRunning) return;
	app.DebugPrintf("------------------DisableUpdateThread--------------------\n");
	updateRunning = false;
	m_updateEvents->Clear(eUpdateCanRun);
	m_updateEvents->WaitForSingle(eUpdateEventIsFinished,INFINITE);
#endif
}