#include "stdafx.h"
#include "Minecraft.h"
#include "GameMode.h"
#include "Timer.h"
#include "LevelRenderer.h"
#include "ParticleEngine.h"
#include "MultiPlayerLocalPlayer.h"
#include "User.h"
#include "Textures.h"
#include "GameRenderer.h"
#include "HumanoidModel.h"
#include "Options.h"
#include "TexturePackRepository.h"
#include "StatsCounter.h"
#include "SurvivalMode.h"
#include "Chunk.h"
#include "CreativeMode.h"
#include "DemoLevel.h"
#include "MultiPlayerLevel.h"
#include "MultiPlayerLocalPlayer.h"
#include "DemoUser.h"
#include "GuiParticles.h"
#include "Screen.h"
#include "DeathScreen.h"
#include "ErrorScreen.h"
#include "TitleScreen.h"
#include "InventoryScreen.h"
#include "InBedChatScreen.h"
#include "AchievementPopup.h"
#include "Input.h"
#include "FrustumCuller.h"
#include "Camera.h"

#include "..\Minecraft.World\MobEffect.h"
#include "..\Minecraft.World\Difficulty.h"
#include "..\Minecraft.World\net.minecraft.world.level.h"
#include "..\Minecraft.World\net.minecraft.world.entity.h"
#include "..\Minecraft.World\net.minecraft.world.entity.player.h"
#include "..\Minecraft.World\net.minecraft.world.entity.item.h"
#include "..\Minecraft.World\net.minecraft.world.phys.h"
#include "..\Minecraft.World\File.h"
#include "..\Minecraft.World\net.minecraft.world.level.storage.h"
#include "..\Minecraft.World\net.minecraft.h"
#include "..\Minecraft.World\net.minecraft.stats.h"
#include "..\Minecraft.World\System.h"
#include "..\Minecraft.World\ByteBuffer.h"
#include "..\Minecraft.World\net.minecraft.world.level.tile.h"
#include "..\Minecraft.World\net.minecraft.world.level.chunk.h"
#include "..\Minecraft.World\net.minecraft.world.level.dimension.h"
#include "..\Minecraft.World\net.minecraft.world.item.h"
#include "..\Minecraft.World\Minecraft.World.h"
#include "Windows64\Windows64_Xuid.h"
#include "ClientConnection.h"
#include "..\Minecraft.World\HellRandomLevelSource.h"
#include "..\Minecraft.World\net.minecraft.world.entity.animal.h"
#include "..\Minecraft.World\net.minecraft.world.entity.monster.h"
#include "..\Minecraft.World\StrongholdFeature.h"
#include "..\Minecraft.World\IntCache.h"
#include "..\Minecraft.World\Villager.h"
#include "..\Minecraft.World\SparseLightStorage.h"
#include "..\Minecraft.World\SparseDataStorage.h"
#include "..\Minecraft.World\ChestTileEntity.h"
#include "TextureManager.h"
#ifdef _XBOX
#include "Xbox\Network\NetworkPlayerXbox.h"
#endif
#include "DLCTexturePack.h"

#ifdef __ORBIS__
#include "Orbis\Network\PsPlusUpsellWrapper_Orbis.h"
#endif

// #define DISABLE_SPU_CODE
// 4J Turning this on will change the graph at the bottom of the debug overlay to show the number of packets of each type added per fram
//#define DEBUG_RENDER_SHOWS_PACKETS 1
//#define SPLITSCREEN_TEST

// If not disabled, this creates an event queue on a seperate thread so that the Level::tick calls can be offloaded
// from the main thread, and have longer to run, since it's called at 20Hz instead of 60
#define DISABLE_LEVELTICK_THREAD

Minecraft *Minecraft::m_instance = nullptr;
int64_t Minecraft::frameTimes[512];
int64_t Minecraft::tickTimes[512];
int Minecraft::frameTimePos = 0;
int64_t Minecraft::warezTime = 0;
File Minecraft::workDir = File(L"");

#ifdef __PSVITA__

TOUCHSCREENRECT QuickSelectRect[3]=
{
	{ 560, 890, 1360, 980 },
	{ 450, 840, 1449, 960 },
	{ 320, 840, 1600, 970 },
};

int QuickSelectBoxWidth[3]=
{
	89,
	111,
	142
};

// 4J - TomK ToDo: these really shouldn't be magic numbers, it should read the hud position from flash.
int iToolTipOffset = 85;

#endif

ResourceLocation Minecraft::DEFAULT_FONT_LOCATION = ResourceLocation(TN_DEFAULT_FONT);
ResourceLocation Minecraft::ALT_FONT_LOCATION = ResourceLocation(TN_ALT_FONT);


Minecraft::Minecraft(Component *mouseComponent, Canvas *parent, MinecraftApplet *minecraftApplet, int width, int height, bool fullscreen)
{
	// 4J - added this block of initialisers
	gameMode = nullptr;
	hasCrashed = false;
	timer = new Timer(SharedConstants::TICKS_PER_SECOND);
	oldLevel = nullptr; //4J Stu added
	level = nullptr;
	levels = MultiPlayerLevelArray(3); // 4J Added
	levelRenderer = nullptr;
	player = nullptr;
	cameraTargetPlayer = nullptr;
	particleEngine = nullptr;
	user = nullptr;
	parent = nullptr;
	pause = false;
	textures = nullptr;
	font = nullptr;
	screen = nullptr;
	localPlayerIdx = 0;
	rightClickDelay = 0;

	// 4J Stu Added
	InitializeCriticalSection(&m_setLevelCS);
	//m_hPlayerRespawned = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	progressRenderer = nullptr;
	gameRenderer = nullptr;
	bgLoader = nullptr;

	ticks = 0;
	// 4J-PB - moved into the local player
	//missTime = 0;
	//lastClickTick = 0;
	//isRaining = false;
	// 4J-PB - end

	orgWidth = orgHeight = 0;
	achievementPopup = new AchievementPopup(this);
	noRender = true;
	//humanoidModel = new HumanoidModel(0);
	hitResult = nullptr;
	options = nullptr;
	//soundEngine = new SoundEngine();
	mouseHandler = nullptr;
	skins = nullptr;
	workingDirectory = File(L"");
	levelSource = nullptr;
	stats[0] = nullptr;
	stats[1] = nullptr;
	stats[2] = nullptr;
	stats[3] = nullptr;
	connectToPort = 0;
	workDir = File(L"");
	// 4J removed
	//wasDown = false;
	lastTimer = -1;

	// 4J removed
	//lastTickTime = System::currentTimeMillis();
	recheckPlayerIn = 0;
	running = true;
	unoccupiedQuadrant = -1;

	Stats::init();

	orgHeight = height;
	this->fullscreen = fullscreen;
	this->minecraftApplet = nullptr;

	this->parent = parent;
	// 4J - Our actual physical frame buffer is always 1280x720 ie in a 16:9 ratio. If we want to do a 4:3 mode, we are telling the original minecraft code
	// that the width is 3/4 what it actually is, to correctly present a 4:3 image. Have added width_phys and height_phys for any code we add that requires
	// to know the real physical dimensions of the frame buffer.
	if( RenderManager.IsWidescreen() )
	{
		this->width = width;
	}
	else
	{
		this->width = (width * 3 ) / 4;
	}
	this->height = height;
	this->width_phys = width;
	this->height_phys = height;

	this->fullscreen = fullscreen;

	appletMode = false;

	Minecraft::m_instance = this;
	TextureManager::createInstance();

	for(int i=0;i<XUSER_MAX_COUNT;i++)
	{
		m_pendingLocalConnections[i] = nullptr;
		m_connectionFailed[i] = false;
		localgameModes[i]= nullptr;
	}

	animateTickLevel = nullptr;	// 4J added
	m_inFullTutorialBits = 0; // 4J Added
	reloadTextures = false;

	// initialise the audio before any textures are loaded - to avoid the problem in win64 of the Miles audio causing the codec for textures to be unloaded

	// 4J-PB - Removed it from here on Orbis due to it causing a crash with the network init.
	// We should work out why...
#ifndef __ORBIS__
	//this->soundEngine->init(nullptr);
#endif

#ifndef DISABLE_LEVELTICK_THREAD
	levelTickEventQueue = new C4JThread::EventQueue(levelTickUpdateFunc, levelTickThreadInitFunc, "LevelTick_EventQueuePoll");
	levelTickEventQueue->setProcessor(3);
	levelTickEventQueue->setPriority(THREAD_PRIORITY_NORMAL);
#endif // DISABLE_LEVELTICK_THREAD
}

void Minecraft::clearConnectionFailed()
{
	for(int i=0;i<XUSER_MAX_COUNT;i++)
	{
		m_connectionFailed[i] = false;
		m_connectionFailedReason[i] = DisconnectPacket::eDisconnect_None;
	}
	app.SetDisconnectReason(DisconnectPacket::eDisconnect_None);
}

void Minecraft::connectTo(const wstring& server, int port)
{
	connectToIp = server;
	connectToPort = port;
}

void Minecraft::init()
{
#if 0 // 4J - removed
	if (parent != null)
	{
		Graphics g = parent.getGraphics();
		if (g != null) {
			g.setColor(Color.BLACK);
			g.fillRect(0, 0, width, height);
			g.dispose();
		}
		Display.setParent(parent);
	} else {
		if (fullscreen) {
			Display.setFullscreen(true);
			width = Display.getDisplayMode().getWidth();
			height = Display.getDisplayMode().getHeight();
			if (width <= 0) width = 1;
			if (height <= 0) height = 1;
		} else {
			Display.setDisplayMode(new DisplayMode(width, height));
		}
	}

	Display.setTitle("Minecraft " + VERSION_STRING);
	try {
		Display.create();
		/*
		* System.out.println("LWJGL version: " + Sys.getVersion());
		* System.out.println("GL RENDERER: " +
		* GL11.glGetString(GL11.GL_RENDERER));
		* System.out.println("GL VENDOR: " +
		* GL11.glGetString(GL11.GL_VENDOR));
		* System.out.println("GL VERSION: " +
		* GL11.glGetString(GL11.GL_VERSION)); ContextCapabilities caps =
		* GLContext.getCapabilities(); System.out.println("OpenGL 3.0: " +
		* caps.OpenGL30); System.out.println("OpenGL 3.1: " +
		* caps.OpenGL31); System.out.println("OpenGL 3.2: " +
		* caps.OpenGL32); System.out.println("ARB_compatibility: " +
		* caps.GL_ARB_compatibility); if (caps.OpenGL32) { IntBuffer buffer
		* = ByteBuffer.allocateDirect(16 *
		* 4).order(ByteOrder.nativeOrder()).asIntBuffer();
		* GL11.glGetInteger(GL32.GL_CONTEXT_PROFILE_MASK, buffer); int
		* profileMask = buffer.get(0); System.out.println("PROFILE MASK: "
		* + Integer.toBinaryString(profileMask));
		* System.out.println("CORE PROFILE: " + ((profileMask &
		* GL32.GL_CONTEXT_CORE_PROFILE_BIT) != 0));
		* System.out.println("COMPATIBILITY PROFILE: " + ((profileMask &
		* GL32.GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) != 0)); }
		*/
	} catch (LWJGLException e) {
		// This COULD be because of a bug! A delay followed by a new attempt
		// is supposed getWorkingDirectoryto fix it.
		e.printStackTrace();
		try {
			Thread.sleep(1000);
		} catch (InterruptedException e1) {
		}
		Display.create();
	}

	if (Minecraft.FLYBY_MODE) {
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	}
#endif

	// glClearColor(0.2f, 0.2f, 0.2f, 1);

	workingDirectory = File(L"");//getWorkingDirectory();
	levelSource = new McRegionLevelStorageSource(File(workingDirectory, L"saves"));
	//        levelSource = new MemoryLevelStorageSource();
	options = new Options(this, workingDirectory);
	skins = new TexturePackRepository(workingDirectory, this);
	skins->addDebugPacks();
	textures = new Textures(skins, options);
	//renderLoadingScreen();

	//if (options.languageCode != null) {
	//	Language.getInstance().loadLanguage(options.languageCode);
	//	//            font.setEnforceUnicodeSheet("true".equalsIgnoreCase(I18n.get("language.enforceUnicode")));
	//	font.setEnforceUnicodeSheet(Language.getInstance().isSelectedLanguageIsUnicode());
	//	font.setBidirectional(Language.isBidirectional(options.languageCode));
	//}

	// 4J Stu - Not using these any more
	//WaterColor::init(textures->loadTexturePixels(L"misc/watercolor.png"));
	//GrassColor::init(textures->loadTexturePixels(L"misc/grasscolor.png"));
	//FoliageColor::init(textures->loadTexturePixels(L"misc/foliagecolor.png"));

	gameRenderer = new GameRenderer(this);
	//EntityRenderDispatcher::instance->itemInHandRenderer = new ItemInHandRenderer(this,false);

	for( int i=0 ; i<4 ; ++i )
		stats[i] = new StatsCounter();

	/*		4J - TODO, 4J-JEV: Unnecessary.
	Achievements::openInventory->setDescFormatter(nullptr);
	Achievements.openInventory.setDescFormatter(new DescFormatter(){
	public String format(String i18nValue) {
	return String.format(i18nValue, Keyboard.getKeyName(options.keyBuild.key));
	}
	});
	*/

	// 4J-PB - We'll do this in a xui intro
	//renderLoadingScreen();

	//Keyboard::create();
	//Mouse::create();
#if 0	// 4J - removed
	mouseHandler = new MouseHandler(parent);
	try {
		Controllers.create();
	} catch (Exception e) {
		e.printStackTrace();
	}
#endif

	MemSect(31);
	checkGlError(L"Pre startup");
	MemSect(0);

	// width = Display.getDisplayMode().getWidth();
	// height = Display.getDisplayMode().getHeight();

	//glEnable(GL_TEXTURE_2D);
	//glShadeModel(GL_SMOOTH);
	//glClearDepth(1.0);
	//glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LEQUAL);
	//glEnable(GL_ALPHA_TEST);
	//glAlphaFunc(GL_GREATER, 0.1f);
	//glCullFace(GL_BACK);

	//glMatrixMode(GL_PROJECTION);
	//glLoadIdentity();
	//glMatrixMode(GL_MODELVIEW);
	MemSect(31);
	checkGlError(L"Startup");
	MemSect(0);

	//    openGLCapabilities = new OpenGLCapabilities();	// 4J - removed

	levelRenderer = new LevelRenderer(this, textures);
	//textures->register(&TextureAtlas::LOCATION_BLOCKS, new TextureAtlas(Icon::TYPE_TERRAIN, TN_TERRAIN));
	//textures->register(&TextureAtlas::LOCATION_ITEMS, new TextureAtlas(Icon::TYPE_ITEM, TN_GUI_ITEMS));
	//textures->stitch();

	//glViewport(0, 0, width, height);

	//particleEngine = new ParticleEngine(level, textures);

	MemSect(31);
	checkGlError(L"Post startup");
	MemSect(0);

	if (connectToIp != L"")	// 4J - was nullptr comparison
	{
		//        setScreen(new ConnectScreen(this, connectToIp, connectToPort));		// 4J TODO - put back in
	}
	else
	{
		setScreen(new TitleScreen());
	}

	RenderManager.CBuffLockStaticCreations();
}

void Minecraft::renderLoadingScreen()
{
	// 4J Unused
	// testing stuff on vita just now
#ifdef __PSVITA__
	ScreenSizeCalculator ssc(options, width, height);

	// xxx
	RenderManager.StartFrame();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, (float)ssc.rawWidth, (float)ssc.rawHeight, 0, 1000, 3000);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -2000);
	glViewport(0, 0, width, height);
	glClearColor(0, 0, 0, 0);

	Tesselator *t = Tesselator::getInstance();

	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_FOG);
	// xxx
	glBindTexture(GL_TEXTURE_2D, textures->loadTexture(TN_MOB_PIG));
	t->begin();
	t->color(0xffffff);
	t->vertexUV((float)(0), (float)( height), (float)( 0), (float)( 0), (float)( 0));
	t->vertexUV((float)(width), (float)( height), (float)( 0), (float)( 0), (float)( 0));
	t->vertexUV((float)(width), (float)( 0), (float)( 0), (float)( 0), (float)( 0));
	t->vertexUV((float)(0), (float)( 0), (float)( 0), (float)( 0), (float)( 0));
	t->end();

	int lw = 256;
	int lh = 256;
	glColor4f(1, 1, 1, 1);
	t->color(0xffffff);
	blit((ssc.getWidth() - lw) / 2, (ssc.getHeight() - lh) / 2, 0, 0, lw, lh);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.1f);

	Display::swapBuffers();
	// xxx
	RenderManager.Present();
#endif
}

void Minecraft::blit(int x, int y, int sx, int sy, int w, int h)
{
	float us = 1 / 256.0f;
	float vs = 1 / 256.0f;
	Tesselator *t = Tesselator::getInstance();
	t->begin();
	t->vertexUV(static_cast<float>(x + 0), static_cast<float>(y + h), static_cast<float>(0), (float)( (sx + 0) * us), (float)( (sy + h) * vs));
	t->vertexUV(static_cast<float>(x + w), static_cast<float>(y + h), static_cast<float>(0), (float)( (sx + w) * us), (float)( (sy + h) * vs));
	t->vertexUV(static_cast<float>(x + w), static_cast<float>(y + 0), static_cast<float>(0), (float)( (sx + w) * us), (float)( (sy + 0) * vs));
	t->vertexUV(static_cast<float>(x + 0), static_cast<float>(y + 0), static_cast<float>(0), (float)( (sx + 0) * us), (float)( (sy + 0) * vs));
	t->end();
}

LevelStorageSource *Minecraft::getLevelSource()
{
	return levelSource;
}

void Minecraft::setScreen(Screen *screen)
{
	if (this->screen != nullptr)
	{
		this->screen->removed();
	}

#ifdef _WINDOWS64
	if (screen != nullptr && g_KBMInput.IsMouseGrabbed())
	{
		g_KBMInput.SetMouseGrabbed(false);
	}
#endif

	//4J Gordon: Do not force a stats save here
	/*if (dynamic_cast<TitleScreen *>(screen)!=nullptr)
	{
	stats->forceSend();
	}
	stats->forceSave();*/

	this->screen = screen;
	if (screen != nullptr)
	{
		//        releaseMouse();	// 4J - removed
		ScreenSizeCalculator ssc(options, width, height);
		int screenWidth = ssc.getWidth();
		int screenHeight = ssc.getHeight();
		screen->init(this, screenWidth, screenHeight);
		noRender = false;
	}
	else
	{
		//        grabMouse();	// 4J - removed
	}

	// 4J-PB - if a screen has been set, go into menu mode
	// it's possible that player doesn't exist here yet
	/*if(screen!=nullptr)
	{
	if(player && player->GetXboxPad()!=-1)
	{
	InputManager.SetMenuDisplayed(player->GetXboxPad(),true);
	}
	else
	{
	// set all
	//InputManager.SetMenuDisplayed(XUSER_INDEX_ANY,true);
	}
	}
	else
	{
	if(player && player->GetXboxPad()!=-1)
	{
	InputManager.SetMenuDisplayed(player->GetXboxPad(),false);
	}
	else
	{
	//InputManager.SetMenuDisplayed(XUSER_INDEX_ANY,false);
	}
	}*/
}

void Minecraft::checkGlError(const wstring& string)
{
	// 4J - TODO
}

void Minecraft::destroy()
{
	//4J Gordon: Do not force a stats save here
	/*stats->forceSend();
	stats->forceSave();*/

	//    try {
	setLevel(nullptr);
	//    } catch (Throwable e) {
	//    }

	//    try {
	MemoryTracker::release();
	//    } catch (Throwable e) {
	//    }

	soundEngine->destroy();
	//} finally {
	Display::destroy();
	//    if (!hasCrashed) System.exit(0);	//4J - removed
	//}
	//System.gc();	// 4J - removed
}

// 4J-PB - splitting this function into 3 parts, so we can call the middle part from our xbox game loop

#if 0
void Minecraft::run()
{
	running = true;
	//    try {	// 4J - removed try/catch
	init();
	//    } catch (Exception e) {
	//        e.printStackTrace();
	//       crash(new CrashReport("Failed to start game", e));
	//        return;
	//    }
	//    try {	// 4J - removed try/catch
	if (Minecraft::FLYBY_MODE)
	{
		generateFlyby();
		return;
	}

	int64_t lastTime = System::currentTimeMillis();
	int frames = 0;

	while (running)
	{
		//        try {	// 4J - removed try/catch
		//            if (minecraftApplet != null && !minecraftApplet.isActive()) break;	// 4J - removed
		AABB::resetPool();
		Vec3::resetPool();

		//            if (parent == nullptr && Display.isCloseRequested()) {		// 4J - removed
		//                stop();
		//            }

		if (pause && level != nullptr)
		{
			float lastA = timer->a;
			timer->advanceTime();
			timer->a = lastA;
		}
		else
		{
			timer->advanceTime();
		}

		int64_t beforeTickTime = System::nanoTime();
		for (int i = 0; i < timer->ticks; i++)
		{
			ticks++;
			//            try {		// 4J - try/catch removed
			tick();
			//            } catch (LevelConflictException e) {
			//                this.level = null;
			//                setLevel(null);
			//                setScreen(new LevelConflictScreen());
			//            }
		}
		int64_t tickDuraction = System::nanoTime() - beforeTickTime;
		checkGlError(L"Pre render");

		TileRenderer::fancy = options->fancyGraphics;

		// if (pause) timer.a = 1;

		soundEngine->update(player, timer->a);

		glEnable(GL_TEXTURE_2D);
		if (level != nullptr) level->updateLights();

		//        if (!Keyboard::isKeyDown(Keyboard.KEY_F7)) Display.update();		// 4J - removed

		if (player != nullptr && player->isInWall()) options->thirdPersonView = false;
		if (!noRender)
		{
			if (gameMode != nullptr) gameMode->render(timer->a);
			gameRenderer->render(timer->a);
		}

		/*	4J - removed
		if (!Display::isActive())
		{
		if (fullscreen)
		{
		this->toggleFullScreen();
		}
		Sleep(10);
		}
		*/

		if (options->renderDebug)
		{
			renderFpsMeter(tickDuraction);
		}
		else
		{
			lastTimer = System::nanoTime();
		}

		achievementPopup->render();

		Sleep(0);	// 4J - was Thread.yield()

		//        if (Keyboard::isKeyDown(Keyboard::KEY_F7)) Display.update();	// 4J - removed condition
		Display::update();

		//        checkScreenshot();	// 4J - removed

		/* 4J - removed
		if (parent != nullptr && !fullscreen)
		{
		if (parent.getWidth() != width || parent.getHeight() != height)
		{
		width = parent.getWidth();
		height = parent.getHeight();
		if (width <= 0) width = 1;
		if (height <= 0) height = 1;

		resize(width, height);
		}
		}
		*/
		checkGlError(L"Post render");
		frames++;
		pause = !isClientSide() && screen != nullptr && screen->isPauseScreen();

		while (System::currentTimeMillis() >= lastTime + 1000)
		{
			fpsString = std::to_wstring(frames) + L" fps (" + std::to_wstring(Chunk::updates) + L" chunk updates)";
			Chunk::updates = 0;
			lastTime += 1000;
			frames = 0;
		}
		/*
		} catch (LevelConflictException e) {
		this.level = null;
		setLevel(null);
		setScreen(new LevelConflictScreen());
		} catch (OutOfMemoryError e) {
		emergencySave();
		setScreen(new OutOfMemoryScreen());
		System.gc();
		}
		*/
	}
	/*
	} catch (StopGameException e) {
	} catch (Throwable e) {
	emergencySave();
	e.printStackTrace();
	crash(new CrashReport("Unexpected error", e));
	} finally {
	destroy();
	}
	*/
	destroy();
}
#endif

void Minecraft::run()
{
	running = true;
	//    try {	// 4J - removed try/catch
	init();
	//    } catch (Exception e) {
	//        e.printStackTrace();
	//       crash(new CrashReport("Failed to start game", e));
	//        return;
	//    }
	//    try {	// 4J - removed try/catch
	}

// 4J added - Selects which local player is currently active for processing by the existing minecraft code
bool Minecraft::setLocalPlayerIdx(int idx)
{
	return true;
}

int Minecraft::getLocalPlayerIdx()
{
	return localPlayerIdx;
}

void Minecraft::updatePlayerViewportAssignments()
{
	unoccupiedQuadrant = -1;
	// Find out how many viewports we'll be needing
	int viewportsRequired = 0;
	for( int i = 0; i < XUSER_MAX_COUNT; i++ )
	{
		if( localplayers[i] != nullptr ) viewportsRequired++;
	}
	if( viewportsRequired == 3 ) viewportsRequired = 4;

	// Allocate away...
	if( viewportsRequired == 1 )
	{
		// Single viewport
		for( int i = 0; i < XUSER_MAX_COUNT; i++ )
		{
			if( localplayers[i] != nullptr ) localplayers[i]->m_iScreenSection = C4JRender::VIEWPORT_TYPE_FULLSCREEN;
		}
	}
	else if( viewportsRequired == 2 )
	{
		// Split screen - TODO - option for vertical/horizontal split
		int found = 0;
		for( int i = 0; i < XUSER_MAX_COUNT; i++ )
		{
			if( localplayers[i] != nullptr )
			{
				// Primary player settings decide what the mode is
				if(app.GetGameSettings(ProfileManager.GetPrimaryPad(),eGameSetting_SplitScreenVertical))
				{
					localplayers[i]->m_iScreenSection = C4JRender::VIEWPORT_TYPE_SPLIT_LEFT + found;
				}
				else
				{
					localplayers[i]->m_iScreenSection = C4JRender::VIEWPORT_TYPE_SPLIT_TOP + found;
				}
				found++;
			}
		}
	}
	else if( viewportsRequired >= 3 )
	{
		// Quadrants - this is slightly more complicated. We don't want to move viewports around if we are going from 3 to 4, or 4 to 3 players,
		// so persist any allocations for quadrants that already exist.
		bool quadrantsAllocated[4] = {false,false,false,false};

		for( int i = 0; i < XUSER_MAX_COUNT; i++ )
		{
			if( localplayers[i] != nullptr )
			{

				// 4J Stu - If the game hasn't started, ignore current allocations (as the players won't have seen them)
				// This fixes an issue with the primary player being the 4th controller quadrant, but ending up in the 3rd viewport.
				if(app.GetGameStarted())
				{
					if( ( localplayers[i]->m_iScreenSection >= C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_LEFT ) &&
						( localplayers[i]->m_iScreenSection <= C4JRender::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT ) )
					{
						quadrantsAllocated[localplayers[i]->m_iScreenSection - C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_LEFT] = true;
					}
				}
				else
				{
					// Reset the viewport so that it can be assigned in the next loop
					localplayers[i]->m_iScreenSection = C4JRender::VIEWPORT_TYPE_FULLSCREEN;
				}
			}
		}

		// Found which quadrants are currently in use, now allocate out any spares that are required
		for( int i = 0; i < XUSER_MAX_COUNT; i++ )
		{
			if( localplayers[i] != nullptr )
			{
				if( ( localplayers[i]->m_iScreenSection < C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_LEFT ) ||
					( localplayers[i]->m_iScreenSection > C4JRender::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT ) )
				{
					for( int j = 0; j < 4; j++ )
					{
						if( !quadrantsAllocated[j] )
						{
							localplayers[i]->m_iScreenSection = C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_LEFT + j;
							quadrantsAllocated[j] = true;
							break;
						}
					}
				}
			}
		}
		// If there's an unoccupied quadrant, record which one so we can clear it to black when rendering
		for( int i = 0; i < XUSER_MAX_COUNT; i++ )
		{
			if( quadrantsAllocated[i] == false )
			{
				unoccupiedQuadrant = i;
			}
		}
	}
}

// Add a temporary player so that the viewports get re-arranged, and add the player to the game session
bool Minecraft::addLocalPlayer(int idx)
{
	//int iLocalPlayerC=app.GetLocalPlayerCount();
	if( m_pendingLocalConnections[idx] != nullptr )
	{
		// 4J Stu - Should we ever be in a state where this happens?
		assert(false);
		m_pendingLocalConnections[idx]->close();
	}
	m_connectionFailed[idx] = false;
	m_pendingLocalConnections[idx] = nullptr;

	bool success=g_NetworkManager.AddLocalPlayerByUserIndex(idx);

	if(success)
	{
		app.DebugPrintf("Adding temp local player on pad %d\n", idx);
		localplayers[idx] = shared_ptr<MultiplayerLocalPlayer>(new MultiplayerLocalPlayer(this, level, user, nullptr));
		localgameModes[idx] = nullptr;

		updatePlayerViewportAssignments();

#ifdef _XBOX
		// tell the xui scenes a splitscreen player joined
		XUIMessage xuiMsg;
		CustomMessage_Splitscreenplayer_Struct myMsgData;
		CustomMessage_Splitscreenplayer( &xuiMsg, &myMsgData, true);

		// send the message
		for(int i=0;i<XUSER_MAX_COUNT;i++)
		{
			//if((i!=idx) && (localplayers[i]!=nullptr))
			{
				XuiBroadcastMessage( CXuiSceneBase::GetPlayerBaseScene(i), &xuiMsg );
			}
		}
#endif
	}
	else
	{
		app.DebugPrintf("g_NetworkManager.AddLocalPlayerByUserIndex failed\n");
#ifdef _DURANGO
		ProfileManager.RemoveGamepadFromGame(idx);
#endif
	}

	return success;
}

void Minecraft::addPendingLocalConnection(int idx, ClientConnection *connection)
{
	m_pendingLocalConnections[idx] = connection;
}

shared_ptr<MultiplayerLocalPlayer> Minecraft::createExtraLocalPlayer(int idx, const wstring& name, int iPad, int iDimension, ClientConnection *clientConnection /*= nullptr*/,MultiPlayerLevel *levelpassedin)
{
	if( clientConnection == nullptr) return nullptr;

	if( clientConnection == m_pendingLocalConnections[idx] )
	{
		int tempScreenSection = C4JRender::VIEWPORT_TYPE_FULLSCREEN;
		if( localplayers[idx] != nullptr && localgameModes[idx] == nullptr )
		{
			// A temp player displaying a connecting screen
			tempScreenSection = localplayers[idx]->m_iScreenSection;
		}
		wstring prevname = user->name;
		user->name = name;

		// Don't need this any more
		m_pendingLocalConnections[idx] = nullptr;

		// Add the connection to the level which will now take responsibility for ticking it
		// 4J-PB - can't use the dimension from localplayers[idx], since there may be no localplayers at this point
		//MultiPlayerLevel *mpLevel = (MultiPlayerLevel *)getLevel( localplayers[idx]->dimension );

		MultiPlayerLevel *mpLevel;

		if(levelpassedin)
		{
			level=levelpassedin;
			mpLevel=levelpassedin;
		}
		else
		{
			level=getLevel( iDimension );
			mpLevel = getLevel( iDimension );
			mpLevel->addClientConnection( clientConnection );
		}

		if( app.GetTutorialMode() )
		{
			localgameModes[idx] = new FullTutorialMode(idx, this, clientConnection);
		}
		// check if we're in the trial version
		else if(ProfileManager.IsFullVersion()==false)
		{
			localgameModes[idx] = new TrialMode(idx, this, clientConnection);
		}
		else
		{
			localgameModes[idx] = new ConsoleGameMode(idx, this, clientConnection);
		}

		// 4J-PB - can't do this here because they use a render context, but this is running from a thread.
		// Moved the creation of these into the main thread, before level launch
		//localitemInHandRenderers[idx] = new ItemInHandRenderer(this);
		localplayers[idx] = localgameModes[idx]->createPlayer(level);

		PlayerUID playerXUIDOffline = INVALID_XUID;
		PlayerUID playerXUIDOnline = INVALID_XUID;
		ProfileManager.GetXUID(idx,&playerXUIDOffline,false);
		ProfileManager.GetXUID(idx,&playerXUIDOnline,true);
#ifdef _WINDOWS64
		// Compatibility rule for Win64 id migration
		// host keeps legacy host XUID, non-host uses persistent uid.dat XUID.
		INetworkPlayer *localNetworkPlayer = g_NetworkManager.GetLocalPlayerByUserIndex(idx);
		if(localNetworkPlayer != nullptr && localNetworkPlayer->IsHost())
		{
			playerXUIDOffline = Win64Xuid::GetLegacyEmbeddedHostXuid();
		}
		else
		{
			playerXUIDOffline = Win64Xuid::ResolvePersistentXuid();
		}
#endif
		localplayers[idx]->setXuid(playerXUIDOffline);
		localplayers[idx]->setOnlineXuid(playerXUIDOnline);
		localplayers[idx]->setIsGuest(ProfileManager.IsGuest(idx));

		localplayers[idx]->m_displayName = ProfileManager.GetDisplayName(idx);

		localplayers[idx]->m_iScreenSection = tempScreenSection;

		if( levelpassedin == nullptr)	level->addEntity(localplayers[idx]);	// Don't add if we're passing the level in, we only do this from the client connection & we'll be handling adding it ourselves

		localplayers[idx]->SetXboxPad(iPad);

		if( localplayers[idx]->input != nullptr ) delete localplayers[idx]->input;
		localplayers[idx]->input = new Input();

		localplayers[idx]->resetPos();

		levelRenderer->setLevel(idx, level);
		localplayers[idx]->level = level;

		user->name = prevname;

		updatePlayerViewportAssignments();

		// Fix for #105852 - TU12: Content: Gameplay: Local splitscreen Players are spawned at incorrect places after re-joining previously saved and loaded "Mass Effect World".
		// Move this check to ClientConnection::handleMovePlayer
//		// 4J-PB - can't call this when this function is called from the qnet thread (GetGameStarted will be false)
//		if(app.GetGameStarted())
//		{
//			ui.CloseUIScenes(idx);
//		}
	}

	return localplayers[idx];
}

// on a respawn of the local player, just store them
void Minecraft::storeExtraLocalPlayer(int idx)
{
	localplayers[idx] = player;

	if( localplayers[idx]->input != nullptr ) delete localplayers[idx]->input;
	localplayers[idx]->input = new Input();

	if(ProfileManager.IsSignedIn(idx))
	{
		localplayers[idx]->name = convStringToWstring( ProfileManager.GetGamertag(idx) );
	}
}

void Minecraft::removeLocalPlayerIdx(int idx)
{
	bool updateXui = true;
	if(localgameModes[idx] != nullptr)
	{
		if( getLevel( localplayers[idx]->dimension )->isClientSide )
		{
			shared_ptr<MultiplayerLocalPlayer> mplp = localplayers[idx];
			( (MultiPlayerLevel *)getLevel( localplayers[idx]->dimension ) )->removeClientConnection(mplp->connection, true);
			delete mplp->connection;
			mplp->connection = nullptr;
			g_NetworkManager.RemoveLocalPlayerByUserIndex(idx);
		}
		getLevel( localplayers[idx]->dimension )->removeEntity(localplayers[idx]);

#ifdef _XBOX
		// 4J Stu - Fix for #12368 - Crash: Game crashes when saving then exiting and selecting to save
		app.TutorialSceneNavigateBack(idx);
#endif

		// 4J Stu - Fix for #13257 - CRASH: Gameplay: Title crashed after exiting the tutorial
		// It doesn't matter if they were in the tutorial already
		playerLeftTutorial( idx );

		delete localgameModes[idx];
		localgameModes[idx] = nullptr;
	}
	else if( m_pendingLocalConnections[idx] != nullptr )
	{
		m_pendingLocalConnections[idx]->sendAndDisconnect(std::make_shared<DisconnectPacket>(DisconnectPacket::eDisconnect_Quitting));;
		delete m_pendingLocalConnections[idx];
		m_pendingLocalConnections[idx] = nullptr;
		g_NetworkManager.RemoveLocalPlayerByUserIndex(idx);
	}
	else
	{
		// Not sure how this works on qnet, but for other platforms, calling RemoveLocalPlayerByUserIndex won't do anything if there isn't a local user to remove
		// Now just updating the UI directly in this case
#ifdef _XBOX
		// 4J Stu - A signout early in the game creation before this player has connected to the game server
		updateXui = false;
#endif
		// 4J Stu - Adding this back in for exactly the reason my comment above suggests it was added in the first place
#if defined(_XBOX_ONE) || defined(__ORBIS__)
		g_NetworkManager.RemoveLocalPlayerByUserIndex(idx);
#endif
	}
	localplayers[idx] = nullptr;

	if( idx == ProfileManager.GetPrimaryPad() )
	{
		// We should never try to remove the Primary player in this way
		assert(false);
		/*
		// If we are removing the primary player then there can't be a valid gamemode left anymore, this
		// pointer will be referring to the one we've just deleted
		gameMode = nullptr;
		// Remove references to player
		player = nullptr;
		cameraTargetPlayer = nullptr;
		EntityRenderDispatcher::instance->cameraEntity = nullptr;
		TileEntityRenderDispatcher::instance->cameraEntity = nullptr;
		*/
	}
	else if( updateXui )
	{
		gameRenderer->DisableUpdateThread();
		levelRenderer->setLevel(idx, nullptr);
		gameRenderer->EnableUpdateThread();
		updatePlayerViewportAssignments();
	}

	// We only create these once ever so don't delete it here
	//delete localitemInHandRenderers[idx];
}

void Minecraft::createPrimaryLocalPlayer(int iPad)
{
	localgameModes[iPad] = gameMode;
	localplayers[iPad] = player;
	//gameRenderer->itemInHandRenderer = localitemInHandRenderers[iPad];
	// Give them the gamertag if they're signed in
	if(ProfileManager.IsSignedIn(ProfileManager.GetPrimaryPad()))
	{
		user->name = convStringToWstring( ProfileManager.GetGamertag(ProfileManager.GetPrimaryPad()) );
	}
}

#ifdef _WINDOWS64
void Minecraft::applyFrameMouseLook()
{
	// Per-frame mouse look: consume mouse deltas every frame instead of waiting
	// for the 20Hz game tick. Apply the same delta to both xRot/yRot AND xRotO/yRotO
	// so the render interpolation instantly reflects the change without waiting for a tick.
	if (level == nullptr) return;

	for (int i = 0; i < XUSER_MAX_COUNT; i++)
	{
		if (localplayers[i] == nullptr) continue;
		int iPad = localplayers[i]->GetXboxPad();
		if (iPad != 0) continue;  // Mouse only applies to pad 0

		if (!g_KBMInput.IsMouseGrabbed()) continue;
		if (localgameModes[iPad] == nullptr) continue;

		float rawDx, rawDy;
		g_KBMInput.ConsumeMouseDelta(rawDx, rawDy);
		if (rawDx == 0.0f && rawDy == 0.0f) continue;

		float mouseSensitivity = static_cast<float>(app.GetGameSettings(iPad, eGameSetting_Sensitivity_InGame)) / 100.0f;
		float mdx = rawDx * mouseSensitivity;
		float mdy = -rawDy * mouseSensitivity;
		if (app.GetGameSettings(iPad, eGameSetting_ControlInvertLook))
			mdy = -mdy;

		// Apply 0.15f scaling (same as Entity::interpolateTurn / Entity::turn)
		float dyaw = mdx * 0.15f;
		float dpitch = -mdy * 0.15f;

		// Apply to both current and old rotation so render interpolation
		// reflects the change immediately (no 50ms tick delay)
		localplayers[i]->yRot += dyaw;
		localplayers[i]->yRotO += dyaw;
		localplayers[i]->xRot += dpitch;
		localplayers[i]->xRotO += dpitch;

		// Clamp pitch
		if (localplayers[i]->xRot < -90.0f) localplayers[i]->xRot = -90.0f;
		if (localplayers[i]->xRot > 90.0f) localplayers[i]->xRot = 90.0f;
		if (localplayers[i]->xRotO < -90.0f) localplayers[i]->xRotO = -90.0f;
		if (localplayers[i]->xRotO > 90.0f) localplayers[i]->xRotO = 90.0f;
	}
}
#endif

void Minecraft::run_middle()
{
	static int64_t lastTime = 0;
	static bool bFirstTimeIntoGame = true;
	static bool bAutosaveTimerSet=false;
	static unsigned int uiAutosaveTimer=0;
	static int iFirstTimeCountdown=60;
	if( lastTime == 0 ) lastTime = System::nanoTime();
	static int frames = 0;

	EnterCriticalSection(&m_setLevelCS);

	if(running)
	{
		if (reloadTextures)
		{
			reloadTextures = false;
			textures->reloadAll();
		}

		//while (running)
		{
			//        try {	// 4J - removed try/catch
			//            if (minecraftApplet != null && !minecraftApplet.isActive()) break;	// 4J - removed
			AABB::resetPool();
			Vec3::resetPool();

			//            if (parent == nullptr && Display.isCloseRequested()) {		// 4J - removed
			//                stop();
			//            }

			

			// 4J-PB - Once we're in the level, check if the players have the level in their banned list and ask if they want to play it
			for( int i = 0; i < XUSER_MAX_COUNT; i++ )
			{
				if( localplayers[i] && (app.GetBanListCheck(i)==false) && !Minecraft::GetInstance()->isTutorial() && ProfileManager.IsSignedInLive(i) && !ProfileManager.IsGuest(i) )
				{
					// If there is a sys ui displayed, we can't display the message box here, so ignore until we can
					if(!ProfileManager.IsSystemUIDisplayed())
					{
						app.SetBanListCheck(i,true);
						// 4J-PB - check if the level is in the banned level list
						// get the unique save name and xuid from whoever is the host
#if defined _XBOX || defined _XBOX_ONE
						INetworkPlayer *pHostPlayer = g_NetworkManager.GetHostPlayer();

#ifdef _XBOX
						PlayerUID xuid=((NetworkPlayerXbox *)pHostPlayer)->GetUID();
#else
						PlayerUID xuid=pHostPlayer->GetUID();
#endif

						if(app.IsInBannedLevelList(i,xuid,app.GetUniqueMapName()))
						{
							// put up a message box asking if the player would like to unban this level
							app.DebugPrintf("This level is banned\n");
							// set the app action to bring up the message box to give them the option to remove from the ban list or exit the level
							app.SetAction(i,eAppAction_LevelInBanLevelList,(void *)TRUE);
						}
#endif
					}
				}
			}

			if(!ProfileManager.IsSystemUIDisplayed() && app.DLCInstallProcessCompleted() && !app.DLCInstallPending() && app.m_dlcManager.NeedsCorruptCheck() )
			{
				app.m_dlcManager.checkForCorruptDLCAndAlert();
			}

			for( int i = 0; i < XUSER_MAX_COUNT; i++ )
			{
#ifdef __ORBIS__
				if ( m_pPsPlusUpsell != nullptr && m_pPsPlusUpsell->hasResponse() && m_pPsPlusUpsell->m_userIndex == i )
				{
					delete m_pPsPlusUpsell;
					m_pPsPlusUpsell = nullptr;

					if ( ProfileManager.HasPlayStationPlus(i) )
					{
						app.DebugPrintf("<Minecraft.cpp> Player_%i is now authorised for PsPlus.\n", i);
						if (!ui.PressStartPlaying(i)) ui.ShowPressStart(i);
					}
					else
					{
						UINT uiIDA[1] = { IDS_OK };
						ui.RequestErrorMessage( IDS_CANTJOIN_TITLE, IDS_NO_PLAYSTATIONPLUS, uiIDA, 1, i);
					}
				}
				else
#endif
					if(localplayers[i])
				{
					// 4J-PB - add these to check for coming out of idle
					if(InputManager.ButtonPressed(i, MINECRAFT_ACTION_JUMP))				localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_JUMP;
					if(InputManager.ButtonPressed(i, MINECRAFT_ACTION_USE))					localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_USE;

					if(InputManager.ButtonPressed(i, MINECRAFT_ACTION_INVENTORY))				localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_INVENTORY;
					if(InputManager.ButtonPressed(i, MINECRAFT_ACTION_ACTION))					localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_ACTION;
					if(InputManager.ButtonPressed(i, MINECRAFT_ACTION_CRAFTING))				localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_CRAFTING;
					if(InputManager.ButtonPressed(i, MINECRAFT_ACTION_PAUSEMENU))
					{
						localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_PAUSEMENU;
						app.DebugPrintf("PAUSE PRESSED - ipad = %d, Storing press\n",i);
					}
#ifdef _DURANGO
					if(InputManager.ButtonPressed(i, ACTION_MENU_GTC_PAUSE))					localplayers[i]->ullButtonsPressed|=1LL<<ACTION_MENU_GTC_PAUSE;
#endif
					if(InputManager.ButtonPressed(i, MINECRAFT_ACTION_DROP))					localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_DROP;

					// 4J-PB - If we're flying, the sneak needs to be held on to go down
					if(localplayers[i]->abilities.flying)
					{
						if(InputManager.ButtonDown(i, MINECRAFT_ACTION_SNEAK_TOGGLE))			localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_SNEAK_TOGGLE;
					}
					else
					{
						if(InputManager.ButtonPressed(i, MINECRAFT_ACTION_SNEAK_TOGGLE))		localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_SNEAK_TOGGLE;
					}
					if(InputManager.ButtonPressed(i, MINECRAFT_ACTION_RENDER_THIRD_PERSON))		localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_RENDER_THIRD_PERSON;
					if(InputManager.ButtonPressed(i, MINECRAFT_ACTION_GAME_INFO))				localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_GAME_INFO;

#ifdef _WINDOWS64
					// Keyboard/mouse button presses for player 0
					if (i == 0)
					{
						if (g_KBMInput.IsKBMActive())
						{
							if(g_KBMInput.IsMouseButtonPressed(KeyboardMouseInput::MOUSE_LEFT))
								localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_ACTION;

							if(g_KBMInput.IsMouseButtonPressed(KeyboardMouseInput::MOUSE_RIGHT))
								localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_USE;


							

							if(g_KBMInput.IsKeyPressed(KeyboardMouseInput::KEY_DROP))
								localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_DROP;

							
							
						}

						if(g_KBMInput.IsKeyPressed(KeyboardMouseInput::KEY_THIRD_PERSON))
							localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_RENDER_THIRD_PERSON;

						if(g_KBMInput.IsKeyPressed(KeyboardMouseInput::KEY_DEBUG_MENU))
						{
							localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_RENDER_DEBUG;
						}
					}
#endif

#if _DEBUG // ndef _FINAL_BUILD // Disable conflicting debug functionality in release builds
					if( app.DebugSettingsOn() && app.GetUseDPadForDebug() )
					{
						localplayers[i]->ullDpad_last = 0;
						localplayers[i]->ullDpad_this = 0;
						localplayers[i]->ullDpad_filtered = 0;
						if(InputManager.ButtonPressed(i, MINECRAFT_ACTION_DPAD_RIGHT))			localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_CHANGE_SKIN;
						if(InputManager.ButtonPressed(i, MINECRAFT_ACTION_DPAD_UP))				localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_FLY_TOGGLE;
						if(InputManager.ButtonPressed(i, MINECRAFT_ACTION_DPAD_DOWN))			localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_RENDER_DEBUG;
						if(InputManager.ButtonPressed(i, MINECRAFT_ACTION_DPAD_LEFT))			localplayers[i]->ullButtonsPressed|=1LL<<MINECRAFT_ACTION_SPAWN_CREEPER;
					}
					else
#endif
					{
						// Movement on DPAD is stored ulimately into ullDpad_filtered - this ignores any diagonals pressed, instead reporting the last single direction - otherwise
						// we get loads of accidental diagonal movements

						localplayers[i]->ullDpad_this = 0;
						int dirCount = 0;

#ifndef __PSVITA__
						if(InputManager.ButtonDown(i, MINECRAFT_ACTION_DPAD_LEFT))					{ localplayers[i]->ullDpad_this|=1LL<<MINECRAFT_ACTION_DPAD_LEFT;		dirCount++; }
						if(InputManager.ButtonDown(i, MINECRAFT_ACTION_DPAD_RIGHT))					{ localplayers[i]->ullDpad_this|=1LL<<MINECRAFT_ACTION_DPAD_RIGHT;		dirCount++; }
						if(InputManager.ButtonDown(i, MINECRAFT_ACTION_DPAD_UP))					{ localplayers[i]->ullDpad_this|=1LL<<MINECRAFT_ACTION_DPAD_UP;			dirCount++; }
						if(InputManager.ButtonDown(i, MINECRAFT_ACTION_DPAD_DOWN))					{ localplayers[i]->ullDpad_this|=1LL<<MINECRAFT_ACTION_DPAD_DOWN;		dirCount++; }
#endif

						if( dirCount <= 1 )
						{
							localplayers[i]->ullDpad_last = localplayers[i]->ullDpad_this;
							localplayers[i]->ullDpad_filtered = localplayers[i]->ullDpad_this;
						}
						else
						{
							localplayers[i]->ullDpad_filtered = localplayers[i]->ullDpad_last;
						}
					}

					// for the opacity timer
					if(InputManager.ButtonPressed(i, MINECRAFT_ACTION_LEFT_SCROLL) || InputManager.ButtonPressed(i, MINECRAFT_ACTION_RIGHT_SCROLL))
						//InputManager.ButtonPressed(i, MINECRAFT_ACTION_USE) || InputManager.ButtonPressed(i, MINECRAFT_ACTION_ACTION))
					{
						app.SetOpacityTimer(i);
					}
				}
				else
				{
					// 4J Stu - This doesn't make any sense with the way we handle XboxOne users
#ifndef _DURANGO
					// did we just get input from a player who doesn't exist? They'll be wanting to join the game then
#ifdef _WINDOWS64
					// The 4J toggle system is unreliable here: UIController::handleInput() calls
					// ButtonPressed for every ACTION_MENU_* mapped button (which covers all physical
					// buttons) before run_middle() runs. Bypass it with raw XInput and own edge detection.
					// A latch counter keeps startJustPressed active for ~120 frames after the rising edge
					// so the detection window is large enough to be caught reliably.
					static WORD s_prevXButtons[XUSER_MAX_COUNT] = {};
					static int  s_startPressLatch[XUSER_MAX_COUNT] = {};
					XINPUT_STATE xstate_join;
					memset(&xstate_join, 0, sizeof(xstate_join));
					WORD xCurButtons = 0;
					if (XInputGetState(i, &xstate_join) == ERROR_SUCCESS)
					{
						xCurButtons = xstate_join.Gamepad.wButtons;
						if ((xCurButtons & XINPUT_GAMEPAD_START) != 0 && (s_prevXButtons[i] & XINPUT_GAMEPAD_START) == 0)
							s_startPressLatch[i] = 120; // rising edge: latch for ~120 frames (~2s at 60fps)
						else if (s_startPressLatch[i] > 0)
							s_startPressLatch[i]--;
						s_prevXButtons[i] = xCurButtons;
					}
					bool startJustPressed = s_startPressLatch[i] > 0;
#else
					bool tryJoin = !pause && !ui.IsIgnorePlayerJoinMenuDisplayed(ProfileManager.GetPrimaryPad()) && g_NetworkManager.SessionHasSpace() && RenderManager.IsHiDef() && InputManager.ButtonPressed(i);
#endif
#ifdef __ORBIS__
					// Check for remote play
					tryJoin = tryJoin && InputManager.IsLocalMultiplayerAvailable();

					// 4J Stu - Check that content restriction information has been received
					if( !g_NetworkManager.IsLocalGame() )
					{
						tryJoin = tryJoin && ProfileManager.GetChatAndContentRestrictions(i,true,nullptr,nullptr,nullptr);
					}
#endif
					
#endif // _DURANGO
				}
			}

#ifdef _DURANGO
			// did we just get input from a player who doesn't exist? They'll be wanting to join the game then
			if(!pause && !ui.IsIgnorePlayerJoinMenuDisplayed(ProfileManager.GetPrimaryPad()) && g_NetworkManager.SessionHasSpace() && RenderManager.IsHiDef() )
			{
				int firstEmptyUser = 0;
				for( int i = 0; i < XUSER_MAX_COUNT; i++ )
				{
					if(localplayers[i] == nullptr)
					{
						firstEmptyUser = i;
						break;
					}
				}

				// For durango, check for unmapped controllers
				for(unsigned int iPad = XUSER_MAX_COUNT; iPad < (XUSER_MAX_COUNT + InputManager.MAX_GAMEPADS); ++iPad)
				{
					bool isPadLocked = InputManager.IsPadLocked(iPad), isPadConnected = InputManager.IsPadConnected(iPad), buttonPressed = InputManager.ButtonPressed(iPad);
					if (isPadLocked || !isPadConnected || !buttonPressed) continue;

					if(!ui.PressStartPlaying(firstEmptyUser))
					{
						ui.ShowPressStart(firstEmptyUser);
					}
					else
					{
						// did we just get input from a player who doesn't exist? They'll be wanting to join the game then
						if(InputManager.ButtonPressed(iPad, MINECRAFT_ACTION_PAUSEMENU))
						{
							// bring up the sign in dialog
							app.DebugPrintf("Bringing up the sign in ui\n");
							ProfileManager.RequestSignInUI(false, g_NetworkManager.IsLocalGame(), true, false,true,&Minecraft::InGame_SignInReturned, this,iPad);

							// 4J Stu - If we are joining a pad here, then we don't want to try and join any others
							break;
						}
					}
				}
			}
#endif

			if (pause && level != nullptr)
			{
				float lastA = timer->a;
				timer->advanceTime();
				timer->a = lastA;
			}
			else
			{
				timer->advanceTime();
			}

			//int64_t beforeTickTime = System::nanoTime();
			for (int i = 0; i < timer->ticks; i++)
			{
				bool bLastTimerTick = ( i == ( timer->ticks - 1 ) );
				// 4J-PB - the tick here can run more than once, and this is a problem for our input, which would see the a key press twice with the same time - let's tick the inputmanager again
				if(i!=0)
				{
					InputManager.Tick();
					app.HandleButtonPresses();
				}

				ticks++;
				//            try {		// 4J - try/catch removed
				bool bFirst = true;
				for( int idx = 0; idx < XUSER_MAX_COUNT; idx++ )
				{
					// 4J - If we are waiting for this connection to do something, then tick it here.
					// This replaces many of the original Java scenes which would tick the connection while showing that scene
					if( m_pendingLocalConnections[idx] != nullptr )
					{
						m_pendingLocalConnections[idx]->tick();
					}

					// reset the player inactive tick
					if(localplayers[idx]!=nullptr)
					{
						// any input received?
						if((localplayers[idx]->ullButtonsPressed!=0) || InputManager.GetJoypadStick_LX(idx,false)!=0.0f ||
							InputManager.GetJoypadStick_LY(idx,false)!=0.0f || InputManager.GetJoypadStick_RX(idx,false)!=0.0f ||
							InputManager.GetJoypadStick_RY(idx,false)!=0.0f )
						{
							localplayers[idx]->ResetInactiveTicks();
						}
						else
						{
							localplayers[idx]->IncrementInactiveTicks();
						}

						if(localplayers[idx]->GetInactiveTicks()>200)
						{
							if(!localplayers[idx]->isIdle() && localplayers[idx]->onGround)
							{
								localplayers[idx]->setIsIdle(true);
							}
						}
						else
						{
							if(localplayers[idx]->isIdle())
							{
								localplayers[idx]->setIsIdle(false);
							}
						}
					}

					if( setLocalPlayerIdx(idx) )
					{
						tick(bFirst, bLastTimerTick);
						bFirst = false;
						// clear the stored button downs since the tick for this player will now have actioned them
						player->ullButtonsPressed=0LL;
					}
				}


				setLocalPlayerIdx(ProfileManager.GetPrimaryPad());

				// 4J - added - now do the equivalent of level::animateTick, but taking into account the positions of all our players

				for( int l = 0; l < levels.length; l++ )
				{
					if( levels[l] )
					{
						levels[l]->animateTickDoWork();
					}
				}

				//            } catch (LevelConflictException e) {
				//                this.level = null;
				//                setLevel(null);
				//                setScreen(new LevelConflictScreen());
				//            }
// 				SparseLightStorage::tick();	// 4J added
// 				CompressedTileStorage::tick();	// 4J added
// 				SparseDataStorage::tick();		// 4J added
			}
			//int64_t tickDuraction = System::nanoTime() - beforeTickTime;
			MemSect(31);
			checkGlError(L"Pre render");
			MemSect(0);


			// if (pause) timer.a = 1;

			PIXBeginNamedEvent(0,"Sound engine update");
			soundEngine->tick((shared_ptr<Mob> *)localplayers, timer->a);
			PIXEndNamedEvent();

			PIXBeginNamedEvent(0,"Light update");

			glEnable(GL_TEXTURE_2D);

			PIXEndNamedEvent();

			//        if (!Keyboard::isKeyDown(Keyboard.KEY_F7)) Display.update();		// 4J - removed

			// 4J-PB - changing this to be per player
			//if (player != nullptr && player->isInWall()) options->thirdPersonView = false;
			if (player != nullptr && player->isInWall()) player->SetThirdPersonView(0);

			if (!noRender)
			{
				bool bFirst = true;
				int iPrimaryPad=ProfileManager.GetPrimaryPad();
				for( int i = 0; i < XUSER_MAX_COUNT; i++ )
				{
					if( setLocalPlayerIdx(i) )
					{
						PIXBeginNamedEvent(0,"Game render player idx %d",i);
						RenderManager.StateSetViewport(static_cast<C4JRender::eViewportType>(player->m_iScreenSection));
						//gameRenderer->render(timer->a, bFirst);
						bFirst = false;
						PIXEndNamedEvent();

						if(i==iPrimaryPad)
						{
#ifdef __ORBIS__
							// PS4 does much of the screen-capturing for every frame, to simplify the synchronisation when we actually want a capture. This call tells it the point in the frame to do it.
							RenderManager.InternalScreenCapture();
#endif
							// check to see if we need to capture a screenshot for the save game thumbnail
							switch(app.GetXuiAction(i))
							{
							case eAppAction_ExitWorldCapturedThumbnail:
							case eAppAction_SaveGameCapturedThumbnail:
							case eAppAction_AutosaveSaveGameCapturedThumbnail:
								// capture the save thumbnail
								app.CaptureSaveThumbnail();
								break;
							}
						}
					}
				}
				setLocalPlayerIdx(iPrimaryPad);
				RenderManager.StateSetViewport(C4JRender::VIEWPORT_TYPE_FULLSCREEN);

#ifdef _XBOX
				// Do we need to capture a screenshot for a social post?
				for( int i = 0; i < XUSER_MAX_COUNT; i++ )
				{
					if(app.GetXuiAction(i)==eAppAction_SocialPostScreenshot)
					{
						app.CaptureScreenshot(i);
					}
				}
#endif
			}
			glFlush();

			/*	4J - removed
			if (!Display::isActive())
			{
			if (fullscreen)
			{
			this->toggleFullScreen();
			}
			Sleep(10);
			}
			*/

#if PACKET_ENABLE_STAT_TRACKING
			Packet::updatePacketStatsPIX();
#endif

			if (options->renderDebug)
			{
				//renderFpsMeter(tickDuraction);

#if DEBUG_RENDER_SHOWS_PACKETS
				// To show data for only one packet type
				//Packet::renderPacketStats(31);

				// To show data for all packet types selected as being renderable in the Packet:static_ctor call to Packet::map
				Packet::renderAllPacketStats();
#else
				// To show the size of the QNet queue in bytes and messages
				g_NetworkManager.renderQueueMeter();
#endif
			}
			else
			{
				lastTimer = System::nanoTime();
			}

			achievementPopup->render();

			PIXBeginNamedEvent(0,"Sleeping");
			Sleep(0);	// 4J - was Thread.yield()
			PIXEndNamedEvent();

			//        if (Keyboard::isKeyDown(Keyboard::KEY_F7)) Display.update();	// 4J - removed condition
			PIXBeginNamedEvent(0,"Display update");
			Display::update();
			PIXEndNamedEvent();

			//        checkScreenshot();	// 4J - removed

			/* 4J - removed
			if (parent != nullptr && !fullscreen)
			{
			if (parent.getWidth() != width || parent.getHeight() != height)
			{
			width = parent.getWidth();
			height = parent.getHeight();
			if (width <= 0) width = 1;
			if (height <= 0) height = 1;

			resize(width, height);
			}
			}
			*/
			MemSect(31);
			checkGlError(L"Post render");
			MemSect(0);
			frames++;
			//pause = !isClientSide() && screen != nullptr && screen->isPauseScreen();
			//pause = g_NetworkManager.IsLocalGame() && g_NetworkManager.GetPlayerCount() == 1 && app.IsPauseMenuDisplayed(ProfileManager.GetPrimaryPad());
			pause = app.IsAppPaused();

#ifndef _CONTENT_PACKAGE
			while (System::nanoTime() >= lastTime + 1000000000)
			{
				MemSect(31);
				fpsString = std::to_wstring(frames) + L" fps (" + std::to_wstring(Chunk::updates) + L" chunk updates)";
				MemSect(0);
				Chunk::updates = 0;
				lastTime += 1000000000;
				frames = 0;
			}
#endif
			/*
			} catch (LevelConflictException e) {
			this.level = null;
			setLevel(null);
			setScreen(new LevelConflictScreen());
			} catch (OutOfMemoryError e) {
			emergencySave();
			setScreen(new OutOfMemoryScreen());
			System.gc();
			}
			*/
		}
		/*
		} catch (StopGameException e) {
		} catch (Throwable e) {
		emergencySave();
		e.printStackTrace();
		crash(new CrashReport("Unexpected error", e));
		} finally {
		destroy();
		}
		*/
	}
	LeaveCriticalSection(&m_setLevelCS);
}

void Minecraft::run_end()
{
	destroy();
}

void Minecraft::emergencySave()
{
	// 4J - lots of try/catches removed here, and garbage collector things
	levelRenderer->clear();
	AABB::clearPool();
	Vec3::clearPool();
	setLevel(nullptr);
}

void Minecraft::renderFpsMeter(int64_t tickTime)
{
	int nsPer60Fps = 1000000000l / 60;
	if (lastTimer == -1)
	{
		lastTimer = System::nanoTime();
	}
	int64_t now = System::nanoTime();
	Minecraft::tickTimes[(Minecraft::frameTimePos) & (Minecraft::frameTimes_length - 1)] = tickTime;
	Minecraft::frameTimes[(Minecraft::frameTimePos++) & (Minecraft::frameTimes_length - 1)] = now - lastTimer;
	lastTimer = now;

	glClear(GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glEnable(GL_COLOR_MATERIAL);
	glLoadIdentity();
	glOrtho(0, static_cast<float>(width), static_cast<float>(height), 0, 1000, 3000);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -2000);

	glLineWidth(1);
	glDisable(GL_TEXTURE_2D);
	Tesselator *t = Tesselator::getInstance();
	t->begin(GL_QUADS);
	int hh1 = (int) (nsPer60Fps / 200000);
	t->color(0x20000000);
	t->vertex(static_cast<float>(0), static_cast<float>(height - hh1), static_cast<float>(0));
	t->vertex(static_cast<float>(0), static_cast<float>(height), static_cast<float>(0));
	t->vertex(static_cast<float>(Minecraft::frameTimes_length), static_cast<float>(height), static_cast<float>(0));
	t->vertex(static_cast<float>(Minecraft::frameTimes_length), static_cast<float>(height - hh1), static_cast<float>(0));

	t->color(0x20200000);
	t->vertex(static_cast<float>(0), static_cast<float>(height - hh1 * 2), static_cast<float>(0));
	t->vertex(static_cast<float>(0), static_cast<float>(height - hh1), static_cast<float>(0));
	t->vertex(static_cast<float>(Minecraft::frameTimes_length), static_cast<float>(height - hh1), static_cast<float>(0));
	t->vertex(static_cast<float>(Minecraft::frameTimes_length), static_cast<float>(height - hh1 * 2), static_cast<float>(0));

	t->end();
	int64_t totalTime = 0;
	for (int i = 0; i < Minecraft::frameTimes_length; i++)
	{
		totalTime += Minecraft::frameTimes[i];
	}
	int hh = static_cast<int>(totalTime / 200000 / Minecraft::frameTimes_length);
	t->begin(GL_QUADS);
	t->color(0x20400000);
	t->vertex(static_cast<float>(0), static_cast<float>(height - hh), static_cast<float>(0));
	t->vertex(static_cast<float>(0), static_cast<float>(height), static_cast<float>(0));
	t->vertex(static_cast<float>(Minecraft::frameTimes_length), static_cast<float>(height), static_cast<float>(0));
	t->vertex(static_cast<float>(Minecraft::frameTimes_length), static_cast<float>(height - hh), static_cast<float>(0));
	t->end();
	t->begin(GL_LINES);
	for (int i = 0; i < Minecraft::frameTimes_length; i++)
	{
		int col = ((i - Minecraft::frameTimePos) & (Minecraft::frameTimes_length - 1)) * 255 / Minecraft::frameTimes_length;
		int cc = col * col / 255;
		cc = cc * cc / 255;
		int cc2 = cc * cc / 255;
		cc2 = cc2 * cc2 / 255;
		if (Minecraft::frameTimes[i] > nsPer60Fps)
		{
			t->color(0xff000000 + cc * 65536);
		}
		else
		{
			t->color(0xff000000 + cc * 256);
		}

		int64_t time = Minecraft::frameTimes[i] / 200000;
		int64_t time2 = Minecraft::tickTimes[i] / 200000;

		t->vertex((float)(i + 0.5f), (float)( height - time + 0.5f), static_cast<float>(0));
		t->vertex((float)(i + 0.5f), (float)( height + 0.5f), static_cast<float>(0));

		// if (Minecraft.frameTimes[i]>nsPer60Fps) {
		t->color(0xff000000 + cc * 65536 + cc * 256 + cc * 1);
		// } else {
		// t.color(0xff808080 + cc/2 * 256);
		// }
		t->vertex((float)(i + 0.5f), (float)( height - time + 0.5f), static_cast<float>(0));
		t->vertex((float)(i + 0.5f), (float)( height - (time - time2) + 0.5f), static_cast<float>(0));
	}
	t->end();

	glEnable(GL_TEXTURE_2D);
}

void Minecraft::stop()
{
	running = false;
	// keepPolling = false;
}

void Minecraft::pauseGame()
{
	if (screen != nullptr) return;

	//    setScreen(new PauseScreen());	// 4J - TODO put back in
}

void Minecraft::resize(int width, int height)
{
	if (width <= 0) width = 1;
	if (height <= 0) height = 1;
	this->width = width;
	this->height = height;

	if (screen != nullptr)
	{
		ScreenSizeCalculator ssc(options, width, height);
		int screenWidth = ssc.getWidth();
		int screenHeight = ssc.getHeight();
		//        screen->init(this, screenWidth, screenHeight);	// 4J - TODO - put back in
	}
}

void Minecraft::verify()
{
	/* 4J - TODO
	new Thread() {
	public void run() {
	try {
	HttpURLConnection huc = (HttpURLConnection) new URL("https://login.minecraft.net/session?name=" + user.name + "&session=" + user.sessionId).openConnection();
	huc.connect();
	if (huc.getResponseCode() == 400) {
	warezTime = System.currentTimeMillis();
	}
	huc.disconnect();
	} catch (Exception e) {
	e.printStackTrace();
	}
	}
	}.start();
	*/
}




void Minecraft::levelTickUpdateFunc(void* pParam)
{
	Level* pLevel = static_cast<Level *>(pParam);
	pLevel->tick();
}

void Minecraft::levelTickThreadInitFunc()
{
	AABB::CreateNewThreadStorage();
	Vec3::CreateNewThreadStorage();
	IntCache::CreateNewThreadStorage();
	Compression::UseDefaultThreadStorage();
}


// 4J - added bFirst parameter, which is true for the first active viewport in splitscreen
// 4J - added bUpdateTextures, which is true if the actual renderer textures are to be updated - this will be true for the last time this tick runs with bFirst true
void Minecraft::tick(bool bFirst, bool bUpdateTextures)
{
	int iPad=player->GetXboxPad();
	//OutputDebugString("Minecraft::tick\n");

	//4J-PB - only tick this player's stats
	stats[iPad]->tick(iPad);

	// Tick the opacity timer (to display the interface at default opacity for a certain time if the user has been navigating it)
	app.TickOpacityTimer(iPad);

	// 4J added
	if( bFirst ) levelRenderer->destroyedTileManager->tick();

	//gui->tick();
	//gameRenderer->pick(1);
#if 0
	// 4J - removed - we don't use ChunkCache anymore
	if (player != nullptr)
	{
		ChunkSource *cs = level->getChunkSource();
		if (dynamic_cast<ChunkCache *>(cs) != nullptr)
		{
			ChunkCache *spcc = (ChunkCache *)cs;

			// 4J - there was also Mth::floors on these ints but that seems superfluous
			int xt = ((int) player->x) >> 4;
			int zt = ((int) player->z) >> 4;
			spcc->centerOn(xt, zt);
		}
	}
#endif

	// soundEngine.playMusicTick();

	if (!pause && level != nullptr) gameMode->tick();
	MemSect(31);
	glBindTexture(GL_TEXTURE_2D, textures->loadTexture(TN_TERRAIN)); //L"/terrain.png"));
	MemSect(0);
	if( bFirst )
	{
		PIXBeginNamedEvent(0,"Texture tick");
		if (!pause) textures->tick(bUpdateTextures);
		PIXEndNamedEvent();
	}

	/*
	* if (serverConnection != null && !(screen instanceof ErrorScreen)) {
	* if (!serverConnection.isConnected()) {
	* progressRenderer.progressStart("Connecting..");
	* progressRenderer.progressStagePercentage(0); } else {
	* serverConnection.tick(); serverConnection.sendPosition(player); } }
	*/
	

	if (screen != nullptr)
	{
		player->missTime = 10000;
		player->lastClickTick[0] = ticks + 10000;
		player->lastClickTick[1] = ticks + 10000;
	}

	if (screen != nullptr)
	{
		screen->updateEvents();
		if (screen != nullptr)
		{
			screen->particles->tick();
			screen->tick();
		}
	}
	
}

void Minecraft::reloadSound()
{
	//    System.out.println("FORCING RELOAD!");		// 4J - removed
	soundEngine = new SoundEngine();
	soundEngine->init(options);
	bgLoader->forceReload();
}

bool Minecraft::isClientSide()
{
	return level != nullptr && level->isClientSide;
}

void Minecraft::selectLevel(ConsoleSaveFile *saveFile, const wstring& levelId, const wstring& levelName, LevelSettings *levelSettings)
{
	}

bool Minecraft::saveSlot(int slot, const wstring& name)
{
	return false;
}

bool Minecraft::loadSlot(const wstring& userName, int slot)
{
	return false;
}

void Minecraft::releaseLevel(int message)
{
	//this->level = nullptr;
	setLevel(nullptr, message);
}

// 4J Stu - This code was within setLevel, but I moved it out so that I can call it at a better
// time when exiting from an online game
void Minecraft::forceStatsSave(int idx)
{
	//4J Gordon: Force a stats save
	stats[idx]->save(idx, true);

	//4J Gordon: If the player is signed in, save the leaderboards
	if( ProfileManager.IsSignedInLive(idx) )
	{
		int tempLockedProfile = ProfileManager.GetLockedProfile();
		ProfileManager.SetLockedProfile(idx);
		stats[idx]->saveLeaderboards();
		ProfileManager.SetLockedProfile(tempLockedProfile);
	}
}

// 4J Added
MultiPlayerLevel *Minecraft::getLevel(int dimension)
{
	if (dimension == -1) return levels[1];
	else if(dimension == 1) return levels[2];
	else return levels[0];
}

// 4J Stu - Removed as redundant with default values in params.
//void Minecraft::setLevel(Level *level, bool doForceStatsSave /*= true*/)
//{
//	setLevel(level, -1, nullptr, doForceStatsSave);
//}

// Also causing ambiguous call for some reason
// as it is matching shared_ptr<Player> from the func below with bool from this one
//void Minecraft::setLevel(Level *level, const wstring& message, bool doForceStatsSave /*= true*/)
//{
//	setLevel(level, message, nullptr, doForceStatsSave);
//}

void Minecraft::forceaddLevel(MultiPlayerLevel *level)
{
	int dimId = level->dimension->id;
	if (dimId == -1) levels[1] = level;
	else if(dimId == 1) levels[2] = level;
	else levels[0] = level;
}

void Minecraft::setLevel(MultiPlayerLevel *level, int message /*=-1*/, shared_ptr<Player> forceInsertPlayer /*=nullptr*/, bool doForceStatsSave /*=true*/, bool bPrimaryPlayerSignedOut /*=false*/)
{
	EnterCriticalSection(&m_setLevelCS);
	bool playerAdded = false;
	this->cameraTargetPlayer = nullptr;

	// 4J-PB - since we now play music in the menu, just let it keep playing
	//soundEngine->playStreaming(L"", 0, 0, 0, 0, 0);

	// 4J - stop update thread from processing this level, which blocks until it is safe to move on - will be re-enabled if we set the level to be non-nullptr
	gameRenderer->DisableUpdateThread();


	for(unsigned int i = 0; i < levels.length; ++i)
	{
		// 4J We only need to save out in multiplayer is we are setting the level to nullptr
		// If we ever go back to making single player only then this will not work properly!
		if (levels[i] != nullptr && level == nullptr)
		{
			// 4J Stu - This is really only relevant for single player (ie not what we do at the moment)
			if((doForceStatsSave==true) && player!=nullptr)
				forceStatsSave(player->GetXboxPad() );

			// 4J Stu - Added these for the case when we exit a level so we are setting the level to nullptr
			// The level renderer needs to have it's stored level set to nullptr so that it doesn't break next time we set one
			if (levelRenderer != nullptr)
			{
				for(DWORD p = 0; p < XUSER_MAX_COUNT; ++p)
				{
					levelRenderer->setLevel(p, nullptr);
				}
			}
			if (particleEngine != nullptr) particleEngine->setLevel(nullptr);
		}
	}
	// 4J If we are setting the level to nullptr then we are exiting, so delete the levels
	if( level == nullptr )
	{
		if(levels[0]!=nullptr)
		{
			delete levels[0];
			levels[0] = nullptr;

			// Both level share the same savedDataStorage
			if(levels[1]!=nullptr) levels[1]->savedDataStorage = nullptr;
		}
		if(levels[1]!=nullptr)
		{
			delete levels[1];
			levels[1] = nullptr;
		}
		if(levels[2]!=nullptr)
		{
			delete levels[2];
			levels[2] = nullptr;
		}

		// Delete all the player objects
		for(unsigned int idx = 0; idx < XUSER_MAX_COUNT; ++idx)
		{
			shared_ptr<MultiplayerLocalPlayer> mplp = localplayers[idx];
			if(mplp != nullptr && mplp->connection != nullptr )
			{
				delete mplp->connection;
				mplp->connection = nullptr;
			}

			if( localgameModes[idx] != nullptr )
			{
				delete localgameModes[idx];
				localgameModes[idx] = nullptr;
			}

			if( m_pendingLocalConnections[idx] != nullptr )
			{
				delete m_pendingLocalConnections[idx];
				m_pendingLocalConnections[idx] = nullptr;
			}

			localplayers[idx] = nullptr;
		}
		// If we are removing the primary player then there can't be a valid gamemode left anymore, this
		// pointer will be referring to the one we've just deleted
		gameMode = nullptr;
		// Remove references to player
		player = nullptr;
		cameraTargetPlayer = nullptr;
	}
	this->level = level;

	if (level != nullptr)
	{
		int dimId = level->dimension->id;
		if (dimId == -1) levels[1] = level;
		else if(dimId == 1) levels[2] = level;
		else levels[0] = level;

		// If no player has been set, then this is the first level to be set this game, so set up
		// a primary player & initialise some other things
		if (player == nullptr)
		{
			int iPrimaryPlayer = ProfileManager.GetPrimaryPad();

			player = gameMode->createPlayer(level);

			PlayerUID playerXUIDOffline = INVALID_XUID;
			PlayerUID playerXUIDOnline = INVALID_XUID;
			ProfileManager.GetXUID(iPrimaryPlayer,&playerXUIDOffline,false);
			ProfileManager.GetXUID(iPrimaryPlayer,&playerXUIDOnline,true);
#ifdef __PSVITA__
			if(CGameNetworkManager::usingAdhocMode() && playerXUIDOnline.getOnlineID()[0] == 0)
			{
				// player doesn't have an online UID, set it from the player name
				playerXUIDOnline.setForAdhoc();
			}
#endif
#ifdef _WINDOWS64
			// On Windows, the implementation has been changed to use a per-client pseudo XUID based on `uid.dat`.
			// To maintain player data compatibility with existing worlds, the world host (the first player) will use the previous embedded pseudo XUID.
			INetworkPlayer *localNetworkPlayer = g_NetworkManager.GetLocalPlayerByUserIndex(iPrimaryPlayer);
			if(localNetworkPlayer != nullptr && localNetworkPlayer->IsHost())
			{
				playerXUIDOffline = Win64Xuid::GetLegacyEmbeddedHostXuid();
			}
			else
			{
				playerXUIDOffline = Win64Xuid::ResolvePersistentXuid();
			}
#endif
			player->setXuid(playerXUIDOffline);
			player->setOnlineXuid(playerXUIDOnline);

			player->m_displayName = ProfileManager.GetDisplayName(iPrimaryPlayer);



			player->resetPos();
			gameMode->initPlayer(player);

			player->SetXboxPad(iPrimaryPlayer);

			for(int i=0;i<XUSER_MAX_COUNT;i++)
			{
				m_pendingLocalConnections[i] = nullptr;
				if( i != iPrimaryPlayer ) localgameModes[i] = nullptr;
			}
		}

		if (player != nullptr)
		{
			player->resetPos();
			// gameMode.initPlayer(player);
			if (level != nullptr)
			{
				level->addEntity(player);
				playerAdded = true;
			}
		}

		if(player->input != nullptr) delete player->input;
		player->input = new Input();

		if (levelRenderer != nullptr) levelRenderer->setLevel(player->GetXboxPad(), level);
		if (particleEngine != nullptr) particleEngine->setLevel(level);

#if 0
		// 4J - removed - we don't use ChunkCache anymore
		ChunkSource *cs = level->getChunkSource();
		if (dynamic_cast<ChunkCache *>(cs) != nullptr)
		{
			ChunkCache *spcc = (ChunkCache *)cs;

			// 4J - these had a Mth::floor which seems unrequired
			int xt = ((int) player->x) >> 4;
			int zt = ((int) player->z) >> 4;

			spcc->centerOn(xt, zt);
		}
#endif
		gameMode->adjustPlayer(player);

		for(int i=0;i<XUSER_MAX_COUNT;i++)
		{
			m_pendingLocalConnections[i] = nullptr;
		}
		updatePlayerViewportAssignments();

		this->cameraTargetPlayer = player;

		// 4J - allow update thread to start processing the level now both it & the player should be ok
		gameRenderer->EnableUpdateThread();
	}
	else
	{
		levelSource->clearAll();
		player = nullptr;

		// Clear all players if the new level is nullptr
		for(int i=0;i<XUSER_MAX_COUNT;i++)
		{
			if( m_pendingLocalConnections[i] != nullptr ) m_pendingLocalConnections[i]->close();
			m_pendingLocalConnections[i] = nullptr;
			localplayers[i] = nullptr;
			localgameModes[i] = nullptr;
		}
	}

	//    System.gc();	// 4J - removed
	// 4J removed
	//this->lastTickTime = 0;
	LeaveCriticalSection(&m_setLevelCS);
}

void Minecraft::prepareLevel(int title)
{
	int r = 128;
	if (gameMode->isCutScene()) r = 64;
	int pp = 0;
	int max = r * 2 / 16 + 1;
	max = max * max;
	ChunkSource *cs = level->getChunkSource();

	Pos *spawnPos = level->getSharedSpawnPos();
	if (player != nullptr)
	{
		spawnPos->x = static_cast<int>(player->x);
		spawnPos->z = static_cast<int>(player->z);
	}

#if 0
	// 4J - removed - we don't use ChunkCache anymore
	if (dynamic_cast<ChunkCache *>(cs)!=nullptr)
	{
		ChunkCache *spcc = (ChunkCache *) cs;

		spcc->centerOn(spawnPos->x >> 4, spawnPos->z >> 4);
	}
#endif

	
	delete spawnPos;

}

wstring Minecraft::gatherStats1()
{
	//return levelRenderer->gatherStats1();
	return L"Time to autosave: " + std::to_wstring( app.SecondsToAutosave() ) + L"s";
}

wstring Minecraft::gatherStats2()
{
	return g_NetworkManager.GatherStats();
	//return levelRenderer->gatherStats2();
}

wstring Minecraft::gatherStats3()
{
	return g_NetworkManager.GatherRTTStats();
	//return L"P: " + particleEngine->countParticles() + L". T: " + level->gatherStats();
}

wstring Minecraft::gatherStats4()
{
	return level->gatherChunkSourceStats();
}

void Minecraft::start(const wstring& name, const wstring& sid)
{
	startAndConnectTo(name, sid, L"");
}

void Minecraft::startAndConnectTo(const wstring& name, const wstring& sid, const wstring& url)
{
	const bool fullScreen = false;
	const wstring userName = name;

	/* 4J - removed window handling things here
	final Frame frame = new Frame("Minecraft");
	Canvas canvas = new Canvas();
	frame.setLayout(new BorderLayout());

	frame.add(canvas, BorderLayout.CENTER);

	// OverlayLayout oll = new OverlayLayout(frame);
	// oll.addLayoutComponent(canvas, BorderLayout.CENTER);
	// oll.addLayoutComponent(new JLabel("TEST"), BorderLayout.EAST);

	canvas.setPreferredSize(new Dimension(854, 480));
	frame.pack();
	frame.setLocationRelativeTo(null);
	*/

	Minecraft *minecraft;
	// 4J - was new Minecraft(frame, canvas, NULL, 854, 480, fullScreen);
	// Logical width is proportional to the real screen aspect ratio so that
	// the ortho projection and HUD layout match the viewport without stretching.
	
	constexpr int logicalH = 0;
	const int logicalW = 0;

	minecraft = new Minecraft(nullptr, nullptr, nullptr, logicalW, logicalH, fullScreen);
	/* - 4J - removed
	{
	@Override
	public void onCrash(CrashReport crashReport) {
	frame.removeAll();
	frame.add(new CrashInfoPanel(crashReport), BorderLayout.CENTER);
	frame.validate();
	}
	}; */

	/* 4J - removed
	final Thread thread = new Thread(minecraft, "Minecraft main thread");
	thread.setPriority(Thread.MAX_PRIORITY);
	*/
	minecraft->serverDomain = L"www.minecraft.net";

	// 4J Stu - We never want the player to be DemoUser, we always want them to have their gamertag displayed
	//if (ProfileManager.IsFullVersion())
	{
		if (userName != L"" && sid != L"")	// 4J - username & side were compared with nullptr rather than empty strings
		{
			minecraft->user = new User(userName, sid);
		}
		else
		{
			minecraft->user = new User(L"Player" + std::to_wstring(System::currentTimeMillis() % 1000), L"");
		}
	}
	//else
	//{
	//	minecraft->user = new DemoUser();
	//}

	/* 4J - TODO
	if (url != nullptr)
	{
	String[] tokens = url.split(":");
	minecraft.connectTo(tokens[0], Integer.parseInt(tokens[1]));
	}
	*/

	/* 4J - removed
	frame.setVisible(true);
	frame.addWindowListener(new WindowAdapter() {
	public void windowClosing(WindowEvent arg0) {
	minecraft.stop();
	try {
	thread.join();
	} catch (InterruptedException e) {
	e.printStackTrace();
	}
	System.exit(0);
	}
	});
	*/
	// 4J - TODO - consider whether we need to actually create a thread here
	minecraft->run();
}

ClientConnection *Minecraft::getConnection(int iPad)
{
	return localplayers[iPad]->connection;
}

// 4J-PB - so we can access this from within our xbox game loop
Minecraft *Minecraft::GetInstance()
{
	return m_instance;
}

bool useLomp = false;

int g_iMainThreadId;

void Minecraft::main()
{
	wstring name;
	wstring sessionId;

	//g_iMainThreadId = GetCurrentThreadId();

	useLomp = true;

	MinecraftWorld_RunStaticCtors();
	//EntityRenderDispatcher::staticCtor();
	//TileEntityRenderDispatcher::staticCtor();
	User::staticCtor();
	Tutorial::staticCtor();
	ColourTable::staticCtor();
	app.loadDefaultGameRules();

#ifdef _LARGE_WORLDS
	//LevelRenderer::staticCtor();
#endif

	// 4J Stu - This block generates XML for the game rules schema
#if 0
	for(unsigned int i = 0; i < Item::items.length; ++i)
	{
		if(Item::items[i] != nullptr)
		{
			app.DebugPrintf("<xs:enumeration value=\"%d\"><xs:annotation><xs:documentation>%ls</xs:documentation></xs:annotation></xs:enumeration>\n", i, app.GetString( Item::items[i]->getDescriptionId() ));
		}
	}

	app.DebugPrintf("\n\n\n\n\n");

	for(unsigned int i = 0; i < 256; ++i)
	{
		if(Tile::tiles[i] != nullptr)
		{
			app.DebugPrintf("<xs:enumeration value=\"%d\"><xs:annotation><xs:documentation>%ls</xs:documentation></xs:annotation></xs:enumeration>\n", i, app.GetString( Tile::tiles[i]->getDescriptionId() ));
		}
	}
	__debugbreak();
#endif

	// 4J-PB - Can't call this for the first 5 seconds of a game - MS rule
	//if (ProfileManager.IsFullVersion())
	{
		name = L"Player" + std::to_wstring(System::currentTimeMillis() % 1000);
		sessionId = L"-";
		/* 4J - TODO - get a session ID from somewhere?
		if (args.length > 0) name = args[0];
		sessionId = "-";
		if (args.length > 1) sessionId = args[1];
		*/
	}


	// On PS4, we call Minecraft::Start from another thread, as this has been timed taking ~2.5 seconds and we need to do some basic
	// rendering stuff so that we don't break the TRCs on SubmitDone calls
#ifndef __ORBIS__
	Minecraft::start(name, sessionId);
#endif
}

bool Minecraft::renderNames()
{
	if (m_instance == nullptr || !m_instance->options->hideGui)
	{
		return true;
	}
	return false;
}

bool Minecraft::useFancyGraphics()
{
	return (m_instance != nullptr && m_instance->options->fancyGraphics);
}

bool Minecraft::useAmbientOcclusion()
{
	return (m_instance != nullptr && m_instance->options->ambientOcclusion != Options::AO_OFF);
}

bool Minecraft::renderDebug()
{
	return (m_instance != nullptr && m_instance->options->renderDebug);
}

bool Minecraft::handleClientSideCommand(const wstring& chatMessage)
{
	return false;
}

int Minecraft::maxSupportedTextureSize()
{
	// 4J Force value
	return 1024;

	//for (int texSize = 16384; texSize > 0; texSize >>= 1) {
	//	GL11.glTexImage2D(GL11.GL_PROXY_TEXTURE_2D, 0, GL11.GL_RGBA, texSize, texSize, 0, GL11.GL_RGBA, GL11.GL_UNSIGNED_BYTE, (ByteBuffer) null);
	//	final int width = GL11.glGetTexLevelParameteri(GL11.GL_PROXY_TEXTURE_2D, 0, GL11.GL_TEXTURE_WIDTH);
	//	if (width != 0) {
	//		return texSize;
	//	}
	//}
	//return -1;
}

void Minecraft::delayTextureReload()
{
	reloadTextures = true;
}

int64_t Minecraft::currentTimeMillis()
{
	return System::currentTimeMillis();//(Sys.getTime() * 1000) / Sys.getTimerResolution();
}

/*void Minecraft::handleMouseDown(int button, bool down)
{
if (gameMode->instaBuild) return;
if (!down) missTime = 0;
if (button == 0 && missTime > 0) return;

if (down && hitResult != nullptr && hitResult->type == HitResult::TILE && button == 0)
{
int x = hitResult->x;
int y = hitResult->y;
int z = hitResult->z;
gameMode->continueDestroyBlock(x, y, z, hitResult->f);
particleEngine->crack(x, y, z, hitResult->f);
}
else
{
gameMode->stopDestroyBlock();
}
}

void Minecraft::handleMouseClick(int button)
{
if (button == 0 && missTime > 0) return;
if (button == 0)
{
app.DebugPrintf("handleMouseClick - Player %d is swinging\n",player->GetXboxPad());
player->swing();
}

bool mayUse = true;

//	* if (button == 1) { ItemInstance item =
//	* player.inventory.getSelected(); if (item != null) { if
//	* (gameMode.useItem(player, item)) {
//	* gameRenderer.itemInHandRenderer.itemUsed(); return; } } }

// 4J-PB - Adding a special case in here for sleeping in a bed in a multiplayer game - we need to wake up, and we don't have the inbedchatscreen with a button

if(button==1 && (player->isSleeping() && level != nullptr && level->isClientSide))
{
shared_ptr<MultiplayerLocalPlayer> mplp = dynamic_pointer_cast<MultiplayerLocalPlayer>( player );

if(mplp) mplp->StopSleeping();

// 4J - TODO
//if (minecraft.player instanceof MultiplayerLocalPlayer)
//{
//    ClientConnection connection = ((MultiplayerLocalPlayer) minecraft.player).connection;
//    connection.send(new PlayerCommandPacket(minecraft.player, PlayerCommandPacket.STOP_SLEEPING));
//}
}

if (hitResult == nullptr)
{
if (button == 0 && !(dynamic_cast<CreativeMode *>(gameMode) != nullptr)) missTime = 10;
}
else if (hitResult->type == HitResult::ENTITY)
{
if (button == 0)
{
gameMode->attack(player, hitResult->entity);
}
if (button == 1)
{
gameMode->interact(player, hitResult->entity);
}
}
else if (hitResult->type == HitResult::TILE)
{
int x = hitResult->x;
int y = hitResult->y;
int z = hitResult->z;
int face = hitResult->f;

//	* if (button != 0) { if (hitResult.f == 0) y--; if (hitResult.f ==
//	* 1) y++; if (hitResult.f == 2) z--; if (hitResult.f == 3) z++; if
//	* (hitResult.f == 4) x--; if (hitResult.f == 5) x++; }

// if (isClientSide())
// {
// return;
// }

if (button == 0)
{
gameMode->startDestroyBlock(x, y, z, hitResult->f);
}
else
{
shared_ptr<ItemInstance> item = player->inventory->getSelected();
int oldCount = item != nullptr ? item->count : 0;
if (gameMode->useItemOn(player, level, item, x, y, z, face))
{
mayUse = false;
app.DebugPrintf("Player %d is swinging\n",player->GetXboxPad());
player->swing();
}
if (item == nullptr)
{
return;
}

if (item->count == 0)
{
player->inventory->items[player->inventory->selected] = nullptr;
}
else if (item->count != oldCount)
{
gameRenderer->itemInHandRenderer->itemPlaced();
}
}
}

if (mayUse && button == 1)
{
shared_ptr<ItemInstance> item = player->inventory->getSelected();
if (item != nullptr)
{
if (gameMode->useItem(player, level, item))
{
gameRenderer->itemInHandRenderer->itemUsed();
}
}
}
}
*/

// 4J-PB
Screen * Minecraft::getScreen()
{
	return screen;
}

bool Minecraft::isTutorial()
{
	return m_inFullTutorialBits > 0;

	/*if( gameMode != nullptr && gameMode->isTutorial() )
	{
	return true;
	}
	else
	{
	return false;
	}*/
}

void Minecraft::playerStartedTutorial(int iPad)
{
	// If the app doesn't think we are in a tutorial mode then just ignore this add
	if( app.GetTutorialMode() ) m_inFullTutorialBits = m_inFullTutorialBits | ( 1 << iPad );
}

void Minecraft::playerLeftTutorial(int iPad)
{
	// 4J Stu - Fix for bug that was flooding Sentient with LevelStart events
	// If the tutorial bits are already 0 then don't need to update anything
	if(m_inFullTutorialBits == 0)
	{
		app.SetTutorialMode( false );
		return;
	}

	m_inFullTutorialBits = m_inFullTutorialBits & ~( 1 << iPad );
	if(m_inFullTutorialBits == 0)
	{
		app.SetTutorialMode( false );

		// 4J Stu -This telemetry event means something different on XboxOne, so we don't call it for simple state changes like this
#ifndef _XBOX_ONE
		for(DWORD idx = 0; idx < XUSER_MAX_COUNT; ++idx)
		{
			if(localplayers[idx] != nullptr)
			{
				TelemetryManager->RecordLevelStart(idx, eSen_FriendOrMatch_Playing_With_Invited_Friends, eSen_CompeteOrCoop_Coop_and_Competitive, level->difficulty, app.GetLocalPlayerCount(), g_NetworkManager.GetOnlinePlayerCount());
			}
		}
#endif
	}
}

#ifdef _DURANGO
void Minecraft::inGameSignInCheckAllPrivilegesCallback(LPVOID lpParam, bool hasPrivileges, int iPad)
{
	Minecraft* pClass = (Minecraft*)lpParam;

	if(!hasPrivileges)
	{
		ProfileManager.RemoveGamepadFromGame(iPad);
	}
	else
	{
		if( !g_NetworkManager.SessionHasSpace() )
		{
			UINT uiIDA[1];
			uiIDA[0]=IDS_OK;
			ui.RequestErrorMessage(IDS_MULTIPLAYER_FULL_TITLE, IDS_MULTIPLAYER_FULL_TEXT, uiIDA, 1);
			ProfileManager.RemoveGamepadFromGame(iPad);
		}
		else if( ProfileManager.IsSignedInLive(iPad) && ProfileManager.AllowedToPlayMultiplayer(iPad) )
		{
			// create the local player for the iPad
			shared_ptr<Player> player = pClass->localplayers[iPad];
			if( player == nullptr)
			{
				if( pClass->level->isClientSide )
				{
					pClass->addLocalPlayer(iPad);
				}
				else
				{
					// create the local player for the iPad
					shared_ptr<Player> player = pClass->localplayers[iPad];
					if( player == nullptr)
					{
						player = pClass->createExtraLocalPlayer(iPad, (convStringToWstring( ProfileManager.GetGamertag(iPad) )).c_str(), iPad, pClass->level->dimension->id);
					}
				}
			}
		}
	}
}
#endif

#ifdef _XBOX_ONE
int Minecraft::InGame_SignInReturned(void *pParam,bool bContinue, int iPad, int iController)
#else
int Minecraft::InGame_SignInReturned(void *pParam,bool bContinue, int iPad)
#endif
{
	Minecraft* pMinecraftClass = static_cast<Minecraft *>(pParam);

	if(g_NetworkManager.IsInSession())
	{
		// 4J Stu - There seems to be a bug in the signin ui call that enables guest sign in. We never allow this within game, so make sure that it's disabled
		// Fix for #66516 - TCR #124: MPS Guest Support ; #001: BAS Game Stability: TU8: The game crashes when second Guest signs-in on console which takes part in Xbox LIVE multiplayer session.
		app.DebugPrintf("Disabling Guest Signin\n");
		XEnableGuestSignin(FALSE);
	}

	// If sign in succeded, we're in game and this player isn't already playing, continue
	if(bContinue==true && g_NetworkManager.IsInSession() && pMinecraftClass->localplayers[iPad] == nullptr)
	{
		// It's possible that the player has not signed in - they can back out or choose no for the converttoguest
		if(ProfileManager.IsSignedIn(iPad))
		{
#ifdef _DURANGO
			if(!g_NetworkManager.IsLocalGame() && ProfileManager.IsSignedInLive(iPad) && ProfileManager.AllowedToPlayMultiplayer(iPad))
			{
				ProfileManager.CheckMultiplayerPrivileges(iPad, true, &inGameSignInCheckAllPrivilegesCallback, pMinecraftClass);
			}
			else
#endif
			if( !g_NetworkManager.SessionHasSpace() )
			{
				UINT uiIDA[1];
				uiIDA[0]=IDS_OK;
#ifdef _DURANGO
				ProfileManager.RemoveGamepadFromGame(iPad);
#endif
			}
			// if this is a local game then profiles just need to be signed in
			else if( g_NetworkManager.IsLocalGame() || (ProfileManager.IsSignedInLive(iPad) && ProfileManager.AllowedToPlayMultiplayer(iPad)) )
			{
#ifdef __ORBIS__
				bool contentRestricted = false;
				ProfileManager.GetChatAndContentRestrictions(iPad,false,nullptr,&contentRestricted,nullptr); // TODO!

				if (!g_NetworkManager.IsLocalGame() && contentRestricted)
				{
					ui.RequestContentRestrictedMessageBox(IDS_NO_MULTIPLAYER_PRIVILEGE_TITLE, IDS_CONTENT_RESTRICTION, iPad);
				}
				else if(!g_NetworkManager.IsLocalGame() && !ProfileManager.HasPlayStationPlus(iPad))
				{
					pMinecraftClass->m_pPsPlusUpsell = new PsPlusUpsellWrapper(iPad);
					pMinecraftClass->m_pPsPlusUpsell->displayUpsell();
				}
				else
#endif
					if( pMinecraftClass->level->isClientSide )
					{
					pMinecraftClass->addLocalPlayer(iPad);
				}
				else
				{
					// create the local player for the iPad
					shared_ptr<Player> player = pMinecraftClass->localplayers[iPad];
					if( player == nullptr)
					{
						player = pMinecraftClass->createExtraLocalPlayer(iPad, (convStringToWstring( ProfileManager.GetGamertag(iPad) )).c_str(), iPad, pMinecraftClass->level->dimension->id);
					}
				}
			}
			else if( ProfileManager.IsSignedInLive(ProfileManager.GetPrimaryPad()) && !ProfileManager.AllowedToPlayMultiplayer(iPad) )
			{
				// 4J Stu - Don't allow converting to guests as we don't allow any guest sign-in while in the game
				// Fix for #66516 - TCR #124: MPS Guest Support ; #001: BAS Game Stability: TU8: The game crashes when second Guest signs-in on console which takes part in Xbox LIVE multiplayer session.
				//ProfileManager.RequestConvertOfflineToGuestUI( &Minecraft::InGame_SignInReturned, pMinecraftClass,iPad);
				UINT uiIDA[1];
				uiIDA[0]=IDS_CONFIRM_OK;
#ifdef _DURANGO
				ProfileManager.RemoveGamepadFromGame(iPad);
#endif
			}
		}
	}
	return 0;
}

void Minecraft::tickAllConnections()
{
	int oldIdx = getLocalPlayerIdx();
	for(unsigned int i = 0; i < XUSER_MAX_COUNT; i++ )
	{
		shared_ptr<MultiplayerLocalPlayer> mplp = localplayers[i];
		if( mplp && mplp->connection)
		{
			setLocalPlayerIdx(i);
			mplp->connection->tick();
		}
	}
	setLocalPlayerIdx(oldIdx);
}

bool Minecraft::addPendingClientTextureRequest(const wstring &textureName)
{
    auto it = find(m_pendingTextureRequests.begin(), m_pendingTextureRequests.end(), textureName);
    if( it == m_pendingTextureRequests.end() )
	{
		m_pendingTextureRequests.push_back(textureName);
		return true;
	}
	return false;
}

void Minecraft::handleClientTextureReceived(const wstring &textureName)
{
    auto it = find(m_pendingTextureRequests.begin(), m_pendingTextureRequests.end(), textureName);
    if( it != m_pendingTextureRequests.end() )
	{
		m_pendingTextureRequests.erase(it);
	}
}

unsigned int Minecraft::getCurrentTexturePackId()
{
	return skins->getSelected()->getId();
}

ColourTable *Minecraft::getColourTable()
{
	TexturePack *selected = skins->getSelected();

	ColourTable *colours = selected->getColourTable();

	if(colours == nullptr)
	{
		colours = skins->getDefault()->getColourTable();
	}

	return colours;
}

#if defined __ORBIS__
int Minecraft::MustSignInReturnedPSN(void *pParam, int iPad, C4JStorage::EMessageResult result)
{
	Minecraft* pMinecraft = (Minecraft *)pParam;

	if(result == C4JStorage::EMessage_ResultAccept)
	{
		SQRNetworkManager_Orbis::AttemptPSNSignIn(&Minecraft::InGame_SignInReturned, pMinecraft, false, iPad);
	}

	return 0;
}
#endif

