// Game.cpp: implementation of the CGame class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)


#include "StdAfx.h"
#include "Game.h"
#include "SyncTracer.h"
#include "myGL.h"
#include <GL/glu.h>			// Header File For The GLu32 Library
#include <time.h>
#include <stdlib.h>
#include "glList.h"
#ifdef _WIN32
#include "winerror.h"
#endif
#include "float.h"
#include "glFont.h"
#include "InfoConsole.h"
#include "Sound.h"
#include "Camera.h"
#include "BaseSky.h"
#include "Net.h"
#include "Ground.h"
#include "BaseGroundDrawer.h"
#include "GameHelper.h"
#include "GuiKeyReader.h"
#include "Script.h"
#include "SmokeProjectile.h"
#include "ScriptHandler.h"
#include "MouseHandler.h"
#include "BaseWater.h"
#include "UnitHandler.h"
#include "QuadField.h"
#include "ReadMap.h"
#include "GuiHandler.h"
#include "SelectedUnits.h"
#include "Team.h"
#include "LosHandler.h"
#include "TimeProfiler.h"
#include "Bitmap.h"
#include "MiniMap.h"
#include "GroupHandler.h"
#include "MapDamage.h"
#include "UnitParser.h"
#include "BaseTreeDrawer.h"
#include "RegHandler.h"
#include "UnitDefHandler.h"
#include "Unit.h"
#include "TooltipConsole.h"
#include "GeometricObjects.h"
#include "FileHandler.h"
//#include "HpiHandler.h"
#include "VFSHandler.h"
#include "ResourceBar.h"
#include "3DOParser.h"
#include "FartextureHandler.h"
#include "CobEngine.h"
#include "FeatureHandler.h"
#ifndef NO_AVI
#include "AVIGenerator.h"
#endif
#include "UnitTracker.h"
#include "Wind.h"
#include "MetalMap.h"
#include "RadarHandler.h"
#include "MoveInfo.h"
#include "PathManager.h"
#include "DamageArrayHandler.h"
#include "CategoryHandler.h"
#include "ShadowHandler.h"
#ifdef DIRECT_CONTROL_ALLOWED
#include "CobFile.h"
#include "myMath.h"
#include "Weapon.h"
#include "MoveType.h"
#endif
//#include "PhysicsEngine.h"
#include "LoadSaveHandler.h"
#include "NewGuiDefine.h"
#include "GameSetup.h"
#include "ShareBox.h"
#include "GameServer.h"
#include "EndGameBox.h"
#include "InMapDraw.h"
#include "CameraController.h"
#include "GlobalAIHandler.h"
#include "GroundDecalHandler.h"
#include "ArchiveScanner.h"
#include "GameVersion.h"
#include <boost/filesystem/path.hpp>

#ifdef NEW_GUI
#include "GUIcontroller.h"
#include "Game.h"
#endif

GLfloat LightDiffuseLand[]=	{ 0.8f, 0.8f, 0.8f, 1.0f };
GLfloat LightAmbientLand[]=	{ 0.2f, 0.2f, 0.2f, 1.0f };
GLfloat FogLand[]=			{ 0.7f,	0.7f, 0.8f, 0	 }; 
GLfloat FogWhite[]=			{ 1.0f,	1.0f, 1.0f, 0	 }; 
GLfloat FogBlack[]=			{ 0.0f,	0.0f, 0.0f, 0	 }; 

extern bool globalQuit;
CGame* game=0;
extern bool keys[256];
extern bool fullscreen;
extern string stupidGlobalMapname;
extern int stupidGlobalMapId;

CGame::CGame(bool server,std::string mapname)
{
	stupidGlobalMapname=mapname;

	time(&starttime);
	lastTick=clock();
	consumeSpeed=1;
	leastQue=0;
	que=0;
	timeLeft=0;

	oldframenum=0;
	if(server)
		gs->players[0]->readyToStart=true;
	gs->players[0]->active=true;

	if(net->playbackDemo)
		gu->myPlayerNum=MAX_PLAYERS-1;

	for(int a=0;a<8;++a)
		camMove[a]=false;
	for(int a=0;a<4;++a)
		camRot[a]=false;

	fps=0;
	thisFps=0;
	totalGameTime=0;

	debugging=false;
	trackingUnit=false;

	bOneStep=false;
	creatingVideo=false;

	playing=false;
	allReady=false;
	hideInterface=false;
	gameOver=false;
	showClock=!!regHandler.GetInt("ShowClock",0);
	showPlayerInfo=!!regHandler.GetInt("ShowPlayerInfo",0);
	gamePausable=true;
	noSpectatorChat=false;
 

	inbufpos=0;
	inbuflength=0;

	chatting=false;
	userInput="";
	userPrompt="";

#ifdef DIRECT_CONTROL_ALLOWED
	oldPitch=0;
	oldHeading=0;
	oldStatus=255;
#endif

	if(server){		//create game server before loading stuff to avoid timeouts
		game=this;		//should avoid gameserver crashing from using non initialized game pointer
		gameServer=new CGameServer;
	}

	ENTER_UNSYNCED;
	camera=new CCamera();
	cam2=new CCamera();
	sound=new CSound();
	mouse=new CMouseHandler();
	tooltip=new CTooltipConsole();
	
	ENTER_SYNCED;
	damageArrayHandler=new CDamageArrayHandler();
	unitDefHandler=new CUnitDefHandler();;

	ENTER_MIXED;
	if(!server) net->Update();	//prevent timing out during load
	helper=new CGameHelper(this);
//	physicsEngine = new CPhysicsEngine();
	ENTER_UNSYNCED;
	shadowHandler=new CShadowHandler();

	ENTER_SYNCED;
	ground=new CGround();
	CReadMap::Instance();
	moveinfo=new CMoveInfo();
	groundDecals=new CGroundDecalHandler();
	ENTER_MIXED;
	if(!server) net->Update();	//prevent timing out during load
	ENTER_UNSYNCED;
	guihandler=new CGuiHandler();
	minimap=new CMiniMap();
	ENTER_MIXED;
	ph=new CProjectileHandler();
	ENTER_UNSYNCED;
	inMapDrawer=new CInMapDraw();
	geometricObjects=new CGeometricObjects();
	ENTER_SYNCED;
	qf=new CQuadField();
	ENTER_MIXED;
	featureHandler=new CFeatureHandler();
	ENTER_SYNCED;
	mapDamage=new CMapDamage();
	loshandler=new CLosHandler();
	radarhandler=new CRadarHandler(false);
	if(!server) net->Update();	//prevent timing out during load
	featureHandler->LoadFeaturesFromMap(readmap->ifs,CScriptHandler::Instance()->chosenScript->loadGame);
	if(!server) net->Update();	//prevent timing out during load
	pathManager = new CPathManager();
	if(!server) net->Update();	//prevent timing out during load
	ENTER_MIXED;
	uh=new CUnitHandler();
	fartextureHandler = new CFartextureHandler();
	if(!server) net->Update();	//prevent timing out during load
	unit3doparser = new C3DOParser();
	ENTER_UNSYNCED;
	sky=CBaseSky::GetSky();
#ifndef NEW_GUI
	resourceBar=new CResourceBar();
#else
	guikeys=new CGuiKeyReader("");
#endif
	if(!server) net->Update();	//prevent timing out during load
	guikeys=new CGuiKeyReader("uikeys.txt");
	water=CBaseWater::GetWater();
	grouphandler=new CGroupHandler(gu->myTeam);
	globalAI=new CGlobalAIHandler();

	ENTER_MIXED;
	if(!shadowHandler->drawShadows){
		for(int a=0;a<3;++a){
			LightAmbientLand[a]=uh->unitAmbientColor[a];
			LightDiffuseLand[a]=uh->unitSunColor[a];
		}
		glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbientLand);		// Setup The Ambient Light
		glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuseLand);		// Setup The Diffuse Light
		glLightfv(GL_LIGHT1, GL_SPECULAR, LightAmbientLand);		// Setup The Diffuse Light
		glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,0);
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,0);
	}
	for(int a=0;a<16;++a)
		trackPos[a]=float3(0,0,0);

	userInput="";
	showList=0;

	info->AddLine("TA Spring linux %s",VERSION_STRING);

	if(!server){
		netbuf[0]=NETMSG_EXECHECKSUM;
		*((unsigned int*)&netbuf[1])=CreateExeChecksum();
		net->SendData(netbuf,5);
	}

	maxUserSpeed=3;
	minUserSpeed=0.3;

	CPlayer* p=gs->players[gu->myPlayerNum];
	if(!gameSetup)
		p->playerName=regHandler.GetString("name","");
	netbuf[0]=NETMSG_PLAYERNAME;
	netbuf[1]=p->playerName.size()+4;
	netbuf[2]=gu->myPlayerNum;
	for(unsigned int a=0;a<p->playerName.size();a++)
		netbuf[a+3]=p->playerName.at(a);
	netbuf[p->playerName.size()+3]=0;
	net->SendData(netbuf,netbuf[1]);		//sending your playername to the server indicates that you are finished loading

	QueryPerformanceFrequency(&timeSpeed);
	QueryPerformanceCounter(&lastModGameTimeMeasure);
	QueryPerformanceCounter(&lastframe);
	QueryPerformanceCounter(&lastUpdate);
	QueryPerformanceCounter(&lastMoveUpdate);

	glFogfv(GL_FOG_COLOR,FogLand);
	glFogf(GL_FOG_START,0);
	glFogf(GL_FOG_END,gu->viewRange*0.98);
	glFogf(GL_FOG_DENSITY,1.0f);
	glFogi(GL_FOG_MODE,GL_LINEAR);
	glEnable(GL_FOG);
	glClearColor(FogLand[0],FogLand[1],FogLand[2],0);
#ifdef TRACE_SYNC
	tracefile.NewInterval();
	tracefile.NewInterval();
	tracefile.NewInterval();
	tracefile.NewInterval();
	tracefile.NewInterval();
	tracefile.NewInterval();
	tracefile.NewInterval();
	tracefile.NewInterval();
#endif
	activeController=this;

	chatSound=sound->GetWaveId("beep4.wav");

	if(gameServer)
		gameServer->gameLoading=false;
	UnloadStartPicture();
}

CGame::~CGame()
{
	regHandler.SetInt("ShowClock",showClock);
	regHandler.SetInt("ShowPlayerInfo",showPlayerInfo);

	ENTER_MIXED;
#ifndef NO_AVI
	if(creatingVideo){
		creatingVideo=false;
		aviGenerator->ReleaseEngine();
		delete aviGenerator;
		aviGenerator=0;
	}
#endif
#ifdef TRACE_SYNC
	tracefile << "End game\n";
#endif
	if(gameServer){
		gameServer->quitServer=true;
		gameServer=0;
		Sleep(20);
	}

	globalAI->PreDestroy ();
	delete water;
	delete sky;

	CScriptHandler::UnloadInstance();
	delete resourceBar;
	delete uh;
	delete featureHandler;
	delete geometricObjects;
	delete ph;
	delete globalAI;
	delete grouphandler;
	delete minimap;
	delete pathManager;
	delete groundDecals;
	delete groundDrawer;
	delete ground;
	delete inMapDrawer;
	delete net;
	delete radarhandler;
	delete loshandler;
	delete mapDamage;
	delete qf;
	delete tooltip;
	delete guikeys;
	delete sound;
	delete guihandler;
	delete mouse;
	delete helper;
	delete shadowHandler;
	delete moveinfo;
	delete unitDefHandler;
	delete damageArrayHandler;
	delete hpiHandler;
	delete archiveScanner;
	delete unit3doparser;
	delete fartextureHandler;
	CCategoryHandler::RemoveInstance();
	delete camera;
	delete cam2;
	delete info;
}
//called when the key is pressed by the user (can be called several times due to key repeat)
int CGame::KeyPressed(unsigned char k,bool isRepeat)
{
	if(!isRepeat)
		gs->players[gu->myPlayerNum]->currentStats->keyPresses++;
//	info->AddLine("%i",(int)k);

#ifdef NEW_GUI
	GUIcontroller::KeyDown(k);
#endif

	if(showList){					//are we currently showing a list?
		if(k==VK_UP)
			showList->UpOne();
		if(k==VK_DOWN)
			showList->DownOne();
		if(k==VK_RETURN){
			showList->Select();
			showList=0;
		}
		if(k==27 && playing){
			showList=0;
		}
		return 0;
	}
	if (userWriting){
#ifndef NO_CLIPBOARD
		if ((k=='V') && keys[VK_CONTROL]){
			OpenClipboard(0);
			void* p;
			if((p=GetClipboardData(CF_TEXT))!=0){
				userInput+=(char*)p;
			}
			CloseClipboard();
			return 0;
		}
#endif
		if(k==8){ //backspace
			if(userInput.size()!=0)
				userInput.erase(userInput.size()-1,1);
			return 0;
		}
		if(k==VK_RETURN){
			userWriting=false;
			keys[k] = false;		//prevent game start when server chats
			return 0;
		}
		if(k==27 && chatting){
			userWriting=false;
			chatting=false;
			inMapDrawer->wantLabel=false;
			userInput="";
		}
		return 0;
	}

	std::deque<CInputReceiver*>::iterator ri;
	for(ri=inputReceivers.begin();ri!=inputReceivers.end();++ri){
		if((*ri)->KeyPressed(k))
			return 0;
	}
	std::string s=guikeys->TranslateKey(k);

	if(s=="drawinmap"){
		inMapDrawer->keyPressed=true;
	}

	if(s=="showhealthbars"){
		uh->showHealthBars=!uh->showHealthBars;
	}

	if (s=="pause"){
		netbuf[0]=NETMSG_PAUSE;
		netbuf[1]=!gs->paused;
		netbuf[2]=gu->myPlayerNum;
		net->SendData(netbuf,3);
		QueryPerformanceCounter(&lastframe);
	}
	if (s=="singlestep"){
		bOneStep=true;
	}
	if (s=="chat"){
		userWriting=true;
		userPrompt="Say: ";
		chatting=true;
		if(k!=VK_RETURN)
			ignoreNextChar=true;
	}
	if (s=="debug")
		gu->drawdebug=!gu->drawdebug;

	if (s=="track")
		trackingUnit=!trackingUnit;

	if (s=="nosound")
		sound->noSound=!sound->noSound;
	
	if(s=="savegame"){
		CLoadSaveHandler ls;
		ls.SaveGame("Test.ssf");
	}

#ifndef NO_AVI
	if (s=="createvideo"){
		if(creatingVideo){
			creatingVideo=false;
			aviGenerator->ReleaseEngine();
			delete aviGenerator;
			aviGenerator=0;
//			info->AddLine("Finished avi");
		} else {
			creatingVideo=true;
			string name;
			for(int a=0;a<99;++a){
				char t[50];
				itoa(a,t,10);
				name=string("video")+t+".avi";
				CFileHandler ifs(name);
				if(!ifs.FileExists())
					break;
			}
			int x=gu->screenx;
			x-=gu->screenx%4;

			int y=gu->screeny;
			y-=gu->screeny%4;

			BITMAPINFOHEADER bih;
			bih.biSize=sizeof(BITMAPINFOHEADER);
			bih.biWidth=x;
			bih.biHeight=y;
			bih.biPlanes=1;
			bih.biBitCount=24;
			bih.biCompression=BI_RGB;
			bih.biSizeImage=0;
			bih.biXPelsPerMeter=1000;
			bih.biYPelsPerMeter=1000;
			bih.biClrUsed=0;
			bih.biClrImportant=0;

			aviGenerator=new CAVIGenerator();
			aviGenerator->SetFileName(name.c_str());
			aviGenerator->SetRate(30);
			aviGenerator->SetBitmapHeader(&bih);
			HRESULT hr=aviGenerator->InitEngine();
			if(hr!=AVIERR_OK){
				creatingVideo=false;
			} else {
				info->AddLine("Recording avi to %s size %i %i",name.c_str(),x,y);
			}
		}
	}
#endif

	if (s=="updatefov")
		groundDrawer->updateFov=!groundDrawer->updateFov;

	if (s=="drawtrees")
		treeDrawer->drawTrees=!treeDrawer->drawTrees;

	if (s=="dynamicsky")
		sky->dynamicSky=!sky->dynamicSky;

	if (s=="hideinterface")
		hideInterface=!hideInterface;

	if (s=="increaseviewradius"){
		groundDrawer->viewRadius+=2;
		(*info) << "ViewRadius is now " << groundDrawer->viewRadius << "\n";
	}

	if (s=="decreaseviewradius"){
		groundDrawer->viewRadius-=2;
		(*info) << "ViewRadius is now " << groundDrawer->viewRadius << "\n";
	}

	if (s=="moretrees"){
		groundDrawer->baseTreeDistance+=0.2f;
		(*info) << "Base tree distance " << groundDrawer->baseTreeDistance*2*SQUARE_SIZE*TREE_SQUARE_SIZE << "\n";
	}

	if (s=="lesstrees"){
		groundDrawer->baseTreeDistance-=0.2f;
		(*info) << "Base tree distance " << groundDrawer->baseTreeDistance*2*SQUARE_SIZE*TREE_SQUARE_SIZE << "\n";
	}

	if (s=="moreclouds"){
		sky->cloudDensity*=0.95f;
		(*info) << "Cloud density " << 1/sky->cloudDensity << "\n";
	}

	if (s=="lessclouds"){
		sky->cloudDensity*=1.05f;
		(*info) << "Cloud density " << 1/sky->cloudDensity << "\n";
	}

	if (s=="speedup"){
		float speed=gs->userSpeedFactor;
		if(speed<1){
			speed/=0.8;
			if(speed>0.99)
				speed=1;
		}else 
			speed+=0.5;
		if(!net->playbackDemo){
			netbuf[0]=NETMSG_USER_SPEED;
			*((float*)&netbuf[1])=speed;
			net->SendData(netbuf,5);
		} else {
			gs->speedFactor=speed;
			gs->userSpeedFactor=speed;
			(*info) << "Speed " << gs->speedFactor << "\n";
		}
	}

	if (s=="slowdown"){
		float speed=gs->userSpeedFactor;
		if(speed<=1){
			speed*=0.8;
			if(speed<0.1)
				speed=0.1;
		}else 
			speed-=0.5;
		if(!net->playbackDemo){
			netbuf[0]=NETMSG_USER_SPEED;
			*((float*)&netbuf[1])=speed;
			net->SendData(netbuf,5);
		} else {
			gs->speedFactor=speed;
			gs->userSpeedFactor=speed;
			(*info) << "Speed " << gs->speedFactor << "\n";
		}
	}

#ifdef DIRECT_CONTROL_ALLOWED
	if(s=="controlunit"){
		Command c;
		c.id=CMD_STOP;
		c.options=0;
		selectedUnits.GiveCommand(c,false);		//force it to update selection and clear order que

		netbuf[0]=NETMSG_DIRECT_CONTROL;
		netbuf[1]=gu->myPlayerNum;
		net->SendData(netbuf,2);
	}
#endif
	if (s=="showshadowmap"){
		shadowHandler->showShadowMap=!shadowHandler->showShadowMap;
	}

	if (s=="showstandard"){
		groundDrawer->SetExtraTexture(0,0,false);
	}

	if (s=="showelevation"){
		groundDrawer->SetExtraTexture(readmap->heightLineMap,readmap->heightLinePal,true);
	}
	if (s=="yardmap1"){
//		groundDrawer->SetExtraTexture(readmap->yardmapLevels[0],readmap->yardmapPal,true);
	}
	if (s=="lastmsgpos"){
		mouse->currentCamController->SetPos(info->lastMsgPos);
		mouse->inStateTransit=true;
		mouse->transitSpeed=0.5;
	}
	if (s=="showmetalmap"){
		groundDrawer->SetMetalTexture(readmap->metalMap->metalMap,readmap->metalMap->extractionMap,readmap->metalMap->metalPal,false);
	}
	if (s=="showpathmap" && gs->cheatEnabled){
		groundDrawer->SetPathMapTexture();
	}

	if (s=="yardmap4"){
//		groundDrawer->SetExtraTexture(readmap->yardmapLevels[3],readmap->yardmapPal,true);
	}
/*	if (s=="showsupply"){
		groundDrawer->SetExtraTexture(supplyhandler->supplyLevel[gu->myTeam],supplyhandler->supplyPal);
	}*/
/*	if (s=="showteam"){
		groundDrawer->SetExtraTexture(readmap->teammap,cityhandler->teampal);
	}*/
	if (s=="togglelos"){
		groundDrawer->ToggleLosTexture();
	}
	if(s=="mousestate"){
	  mouse->ToggleState(keys[VK_SHIFT] || keys[VK_CONTROL]);
	}

	if (s=="sharedialog"){
		if(!inputReceivers.empty() && dynamic_cast<CShareBox*>(inputReceivers.front())==0 && !gu->spectating)
			new CShareBox();
	}
	if (s=="quit"){
		if(keys[VK_SHIFT]){
			info->AddLine("User exited");
			globalQuit=true;
		} else
			info->AddLine("Use shift-esc to quit");
	}
	if(s=="group1")
		grouphandler->GroupCommand(1);
	if(s=="group2")
		grouphandler->GroupCommand(2);
	if(s=="group3")
		grouphandler->GroupCommand(3);
	if(s=="group4")
		grouphandler->GroupCommand(4);
	if(s=="group5")
		grouphandler->GroupCommand(5);
	if(s=="group6")
		grouphandler->GroupCommand(6);
	if(s=="group7")
		grouphandler->GroupCommand(7);
	if(s=="group8")
		grouphandler->GroupCommand(8);
	if(s=="group9")
		grouphandler->GroupCommand(9);
	if(s=="group0")
		grouphandler->GroupCommand(0);

	if (s=="screenshot"){
		int x=gu->screenx;
		if(gu->screenx%4)
			gu->screenx+=4-gu->screenx%4;
		unsigned char* buf=new unsigned char[gu->screenx*gu->screeny*4];
		glReadPixels(0,0,gu->screenx,gu->screeny,GL_RGBA,GL_UNSIGNED_BYTE,buf);
		CBitmap b(buf,gu->screenx,gu->screeny);
		b.ReverseYAxis();
		char t[50];
		for(int a=0;a<9999;++a){
			sprintf(t,"screen%03i.jpg",a);
			CFileHandler ifs(t);
			if(!ifs.FileExists())
				break;
		}
		b.Save(t);
		delete[] buf;
		gu->screenx=x;
	}

	if (s=="moveforward")
		camMove[0]=true;

	if (s=="moveback")
		camMove[1]=true;

	if (s=="moveleft")
		camMove[2]=true;

	if (s=="moveright")
		camMove[3]=true;

	if (s=="moveup")
		camMove[4]=true;

	if (s=="movedown")
		camMove[5]=true;

	if (s=="movefast")
		camMove[6]=true;

	if (s=="moveslow")
		camMove[7]=true;

	return 0;
}

//Called when a key is released by the user
int CGame::KeyReleased(unsigned char k)				
{
//	keys[k] = false;

#ifdef NEW_GUI
	GUIcontroller::KeyUp(k);
#endif


	if ((userWriting) && (((k>=' ') && (k<='Z')) || (k==8) || (k==190) )){
		return 0;
	}

	std::deque<CInputReceiver*>::iterator ri;
	for(ri=inputReceivers.begin();ri!=inputReceivers.end();++ri){
		if((*ri)->KeyReleased(k))
			return 0;
	}

	std::string s=guikeys->TranslateKey(k);

	if(s=="drawinmap"){
		inMapDrawer->keyPressed=false;
	}

	if (s=="moveforward")
		camMove[0]=false;

	if (s=="moveback")
		camMove[1]=false;

	if (s=="moveleft")
		camMove[2]=false;

	if (s=="moveright")
		camMove[3]=false;

	if (s=="moveup")
		camMove[4]=false;

	if (s=="movedown")
		camMove[5]=false;

	if (s=="movefast")
		camMove[6]=false;

	if (s=="moveslow")
		camMove[7]=false;

	return 0;
}

bool CGame::Update()
{
	mouse->EmptyMsgQueUpdate();
	if(CScriptHandler::Instance()->chosenScript){
		script=CScriptHandler::Instance()->chosenScript;
	}

	thisFps++;

	LARGE_INTEGER timeNow;
	QueryPerformanceCounter(&timeNow);
	LARGE_INTEGER difTime;
	difTime.QuadPart=timeNow.QuadPart-lastModGameTimeMeasure.QuadPart;
	double dif=double(difTime.QuadPart)/double(timeSpeed.QuadPart);
	gu->modGameTime+=dif*gs->speedFactor;
	gu->gameTime+=dif;
	if(playing && !gameOver)
		totalGameTime+=dif;
	lastModGameTimeMeasure=timeNow;

	time(&fpstimer);
	if 	(difftime(fpstimer,starttime)!=0){		//do once every second
		fps=thisFps;
		thisFps=0;

		consumeSpeed=((float)(GAME_SPEED+leastQue-2));
		leastQue=10000;
		if(!gameServer)
			timeLeft=0;

		starttime=fpstimer;
		oldframenum=gs->frameNum;

#ifdef TRACE_SYNC
		tracefile.DeleteInterval();
		tracefile.NewInterval();
#endif
	}

	UpdateUI();
	net->Update();

	if(gameServer)
		gameServer->gameClientUpdated=true;

#if defined(SYNCIFY) || 1		//syncify doesnt support multithreading ...
	gameServer->Update();
#endif

	if(creatingVideo && playing && gameServer){
		gameServer->CreateNewFrame();
	}

	//listen to network
	if(net->connected){
		if(!ClientReadNet()){
			info->AddLine("Client read net wanted quit");
			return false;
		}
	}
	
	if(gameServer && serverNet->waitOnCon && allReady && (keys[13] || script->onlySinglePlayer || gameSetup)){
		chatting=false;
		userWriting=false;
		gameServer->StartGame();
	}

	return true;
}

bool CGame::Draw()
{
	ASSERT_UNSYNCED_MODE;
//	(*info) << mouse->lastx << "\n";
	if(!gs->paused && gs->frameNum>1 && !creatingVideo){
		LARGE_INTEGER startDraw;
		QueryPerformanceCounter(&startDraw);
		gu->timeOffset = ((double)(startDraw.QuadPart - lastUpdate.QuadPart))/timeSpeed.QuadPart*GAME_SPEED*gs->speedFactor;
	} else  {
		gu->timeOffset=0;
		QueryPerformanceCounter(&lastUpdate);
	}
	int a;
	std::string tempstring;

	glClearColor(FogLand[0],FogLand[1],FogLand[2],0);
				
	sky->Update();
//	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	//set camera
	mouse->UpdateCam();

	if(trackingUnit)
		unitTracker.SetCam();

	if(playing && (hideInterface || script->wantCameraControl))
		script->SetCamera();

	
	if(shadowHandler->drawShadows)
		shadowHandler->CreateShadows();

	camera->Update(false);

	water->UpdateWater(this);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer

	sky->Draw();
	groundDrawer->UpdateTextures();
	groundDrawer->Draw();

//	if (playing){
		selectedUnits.Draw();
		uh->Draw(false);
		featureHandler->Draw();
		if(gu->drawdebug && gs->cheatEnabled)
			pathManager->Draw();
		//transparent stuff
		glEnable(GL_BLEND);
		glDepthFunc(GL_LEQUAL);
		water->Draw();
		if(treeDrawer->drawTrees)
			treeDrawer->DrawGrass();
		uh->DrawCloakedUnits();
		ph->Draw(false);
		sky->DrawSun();
		if(keys[VK_SHIFT])
			selectedUnits.DrawCommands();

		mouse->Draw();
		
		#ifndef NEW_GUI
		guihandler->DrawMapStuff();
		#endif
		inMapDrawer->Draw();

		glLoadIdentity();
		glDisable(GL_DEPTH_TEST );
//	}
	//reset fov
	glMatrixMode(GL_PROJECTION);	
	glLoadIdentity();			
	gluOrtho2D(0,1,0,1);
	glMatrixMode(GL_MODELVIEW);

	if(shadowHandler->drawShadows && shadowHandler->showShadowMap)
		shadowHandler->DrawShadowTex();

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLoadIdentity();

	if(mouse->locked){
		glColor4f(1,1,1,0.5);
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_LINES);
		glVertex2f(0.5-10.0/gu->screenx,0.5);
		glVertex2f(0.5+10.0/gu->screenx,0.5);
		glVertex2f(0.5,0.5-10.0/gu->screeny);
		glVertex2f(0.5,0.5+10.0/gu->screeny);
		glEnd();
	}

#ifdef DIRECT_CONTROL_ALLOWED
	if(gu->directControl)
		DrawDirectControlHud();
#endif

	glEnable(GL_TEXTURE_2D);

#ifndef NEW_GUI
	if(!hideInterface && !inputReceivers.empty()){
		std::deque<CInputReceiver*>::iterator ri;
		for(ri=--inputReceivers.end();ri!=inputReceivers.begin();--ri){
			(*ri)->Draw();
		}
		(*ri)->Draw();
	}
#endif

	glEnable(GL_TEXTURE_2D);

	if(gu->drawdebug){
		glPushMatrix();
		glColor4f(1,1,0.5f,0.8f);
		glTranslatef(0.03f,0.02f,0.0f);
		glScalef(0.03f,0.04f,0.1f);

		//skriv ut fps etc
		font->glPrint("FPS %3.0d Frame %d Units %d Part %d(%d)",fps,gs->frameNum,uh->activeUnits.size(),ph->ps.size(),ph->currentParticles);
		glPopMatrix();
		
		if(playing){
			glPushMatrix();
			glTranslatef(0.03f,0.08f,0.0f);
			glScalef(0.02f,0.025f,0.1f);
			font->glPrint("xpos: %5.0f ypos: %5.0f zpos: %5.0f speed %2.2f",camera->pos.x,camera->pos.y,camera->pos.z,gs->speedFactor);
			glPopMatrix();
		}
	}

	if(gameSetup && !playing){
		allReady=gameSetup->Draw();
	} else if( gameServer && serverNet->waitOnCon){				//servern väntar på anslutningar
		glPushMatrix();
		glColor4f(1,1,1,1);
		glTranslatef(0.1f,0.6f,0.0f);
		glScalef(0.03f,0.04f,0.1f);
		allReady=true;
		for(a=0;a<MAX_PLAYERS;a++)
			if(gs->players[a]->active && !gs->players[a]->readyToStart)
				allReady=false;
		if(allReady)
			font->glPrint("Waiting for connections. Press return to start");
		glPopMatrix();
	}
	
	if(userWriting){
		glColor4f(1,1,1,1);
		glTranslatef(0.1f,0.75f,0.0f);
		glScalef(0.03f,0.04f,0.1f);
		tempstring=userPrompt;
		tempstring+=userInput;
		font->glPrint("%s",tempstring.c_str());
		glLoadIdentity();
	}	

	if(showClock){
		glColor4f(1,1,1,1);
		glTranslatef(0.94f,0.01f,0.0f);
		glScalef(0.015f,0.015f,0.1f);
		font->glPrint("%02i:%02i",gs->frameNum/60/30,(gs->frameNum/30)%60);
		glLoadIdentity();
	}
	if(showPlayerInfo){
		glTranslatef(0.72f,0.01f,0.0f);
		glScalef(0.015f,0.015f,0.1f);
		for(int a=0;a<MAX_PLAYERS;++a){
			if(gs->players[a]->active){
				glTranslatef(0,1.2f,0.0f);
				glColor4ubv(gs->teams[gs->players[a]->team]->color);
				font->glPrint("%20s %3.0f%% %3i",gs->players[a]->playerName.c_str(),gs->players[a]->cpuUsage*100,gs->players[a]->ping);
			}
		}
		glLoadIdentity();
	}
	if(!hideInterface)
		info->Draw();

	if(showList)
		showList->Draw();

#ifdef NEW_GUI
	GUIcontroller::Draw();
#endif

	mouse->DrawCursor();

//	float tf[]={1,1,1,1,1,1,1,1,1};
//	glVertexPointer(3,GL_FLOAT,0,tf);
//	glDrawArrays(GL_TRIANGLES,0,3);

	glEnable(GL_DEPTH_TEST );
	glLoadIdentity();

	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);
	gu->lastFrameTime = (double)(start.QuadPart - lastMoveUpdate.QuadPart)/timeSpeed.QuadPart;
	lastMoveUpdate=start;

#ifndef NO_AVI
	if(creatingVideo){
		gu->lastFrameTime=1.0/GAME_SPEED;
		LPBITMAPINFOHEADER ih;
		ih=aviGenerator->GetBitmapHeader();
		unsigned char* buf=new unsigned char[ih->biWidth*ih->biHeight*3];
		glReadPixels(0,0,ih->biWidth,ih->biHeight,GL_BGR_EXT,GL_UNSIGNED_BYTE,buf);

		aviGenerator->AddFrame(buf);

		delete buf;
//		info->AddLine("Saved avi frame size %i %i",ih->biWidth,ih->biHeight);
	}
#endif

	return true;
}

void CGame::StartPlaying()
{
	playing=true;
	lastTick=clock();
	QueryPerformanceCounter(&lastframe);
	ENTER_MIXED;
	gu->myTeam=gs->players[gu->myPlayerNum]->team;
	gu->myAllyTeam=gs->team2allyteam[gu->myTeam];
	grouphandler->team=gu->myTeam;
	ENTER_SYNCED;
}

void CGame::SimFrame()
{
	ASSERT_SYNCED_MODE;
//	info->AddLine("New frame %i %i %i",gs->frameNum,gs->randInt(),uh->CreateChecksum());
	_clearfp();
	_control87(0,_EM_ZERODIVIDE);	//make sure any fpu errors generate an exception immidiatly instead of creating nans (easier to debug)
	_control87(0,_EM_INVALID);
#ifdef TRACE_SYNC
	uh->CreateChecksum();
	tracefile << "New frame:" << gs->frameNum << " " << gs->randSeed << "\n";
#endif

	script->Update();
	gs->frameNum++;

	ENTER_UNSYNCED;
	info->Update();
	geometricObjects->Update();
	if(!(gs->frameNum & 7))
		sound->Update();
	treeDrawer->Update();
	globalAI->Update();
	grouphandler->Update();
	profiler.Update();
#ifdef DIRECT_CONTROL_ALLOWED
	if(gu->directControl){
		unsigned char status=0;
		if(camMove[0]) status|=1;
		if(camMove[1]) status|=2;
		if(camMove[2]) status|=4;
		if(camMove[3]) status|=8;
		if(mouse->buttons[0].pressed) status|=16;
		if(mouse->buttons[1].pressed) status|=32;
		shortint2 hp=GetHAndPFromVector(camera->forward);

		if(hp.x!=oldHeading || hp.y!=oldPitch || oldStatus!=status){
			oldHeading=hp.x;
			oldPitch=hp.y;
			oldStatus=status;

			netbuf[0]=NETMSG_DC_UPDATE;
			netbuf[1]=gu->myPlayerNum;
			netbuf[2]=status;
			*((short int*)&netbuf[3])=hp.x;
			*((short int*)&netbuf[5])=hp.y;
			net->SendData(netbuf,7);
		}
	}
#endif
	ENTER_SYNCED;
START_TIME_PROFILE

	helper->Update();
	mapDamage->Update();
	pathManager->Update();
	uh->Update();
	ph->Update();
	featureHandler->Update();
	GCobEngine.Tick(33);
	wind.Update();
	loshandler->Update();

	if(!(gs->frameNum&31)){
		for(int a=0;a<gs->activeTeams;++a)
			gs->teams[a]->Update();
	}
//	CPathFinder::Instance()->Update();	
	
START_TIME_PROFILE
	ground->CheckCol(ph);	
	ph->CheckUnitCol();
END_TIME_PROFILE("Collisions");

	LARGE_INTEGER stopPhysics;
	QueryPerformanceCounter(&stopPhysics);

END_TIME_PROFILE("Sim time")

	lastUpdate=stopPhysics;

#ifdef DIRECT_CONTROL_ALLOWED

	for(int a=0;a<MAX_PLAYERS;++a){
		if(!gs->players[a]->active || !gs->players[a]->playerControlledUnit)
			continue;

		CUnit* unit=gs->players[a]->playerControlledUnit;
		DirectControlStruct* dc=&gs->players[a]->myControl;
		
		std::vector<long> args;
		args.push_back(0);
		unit->cob->Call(COBFN_AimFromPrimary/*/COBFN_QueryPrimary+weaponNum/**/,args);
		float3 relPos=unit->localmodel->GetPiecePos(args[0]);
		float3 pos=unit->pos+unit->frontdir*relPos.z+unit->updir*relPos.y+unit->rightdir*relPos.x;
		pos+=UpVector*7;

		CUnit* hit;
		float dist=helper->TraceRayTeam(pos,dc->viewDir,unit->maxRange,hit,1,unit,gs->team2allyteam[gs->players[a]->team]);
		dc->target=hit;

		if(hit){
			dc->targetDist=dist;
			dc->targetPos=hit->pos;
			if(!dc->mouse2){
				unit->AttackUnit(hit,true);
				/*					for(std::vector<CWeapon*>::iterator wi=unit->weapons.begin();wi!=unit->weapons.end();++wi)
				if((*wi)->targetType!=Target_Unit || (*wi)->targetUnit!=hit)
				(*wi)->AttackUnit(hit,true);							
				*/
			}
		} else {
			if(dist>unit->maxRange*0.95)
				dist=unit->maxRange*0.95;

			dc->targetDist=dist;
			dc->targetPos=pos+dc->viewDir*dc->targetDist;

			if(!dc->mouse2){
				unit->AttackGround(dc->targetPos,true);
				for(std::vector<CWeapon*>::iterator wi=unit->weapons.begin();wi!=unit->weapons.end();++wi){
					float d=dc->targetDist;
					if(d>(*wi)->range*0.95)
						d=(*wi)->range*0.95;
					float3 p=pos+dc->viewDir*d;
					(*wi)->AttackGround(p,true);
				}
			}
		}
	}

#endif
	_clearfp();
	_control87(_MCW_EM ,_EM_ZERODIVIDE);
	_control87(_MCW_EM ,_EM_INVALID);
}

bool CGame::ClientReadNet()
{
	static int lastMsg=0,thisMsg=0;
	int a;
/*	if(gameServer){
		inbufpos=0;
		inbuflength=0;
	}*/

	if(inbufpos>inbuflength)
		info->AddLine("To much data read");
	if(inbufpos==inbuflength){
		inbufpos=0;
		inbuflength=0;
	}
	if(inbufpos>NETWORK_BUFFER_SIZE*0.5){
		for(a=inbufpos;a<inbuflength;a++)
			inbuf[a-inbufpos]=inbuf[a];
		inbuflength-=inbufpos;
		inbufpos=0;
	}

	if((a=net->GetData(&inbuf[inbuflength],NETWORK_BUFFER_SIZE-inbuflength,0))==-1){
		return gameOver;
	}
	inbuflength+=a;

	if(!gameServer/* && !net->onlyLocal*/){
		LARGE_INTEGER currentFrame;
		QueryPerformanceCounter(&currentFrame);
		
		if(timeLeft>1)
			timeLeft--;
		timeLeft+=consumeSpeed*((float)(currentFrame.QuadPart - lastframe.QuadPart)/timeSpeed.QuadPart);
		lastframe=currentFrame;
		
		que=0;
		for(int i2=inbufpos;i2<inbuflength;){
			switch (inbuf[i2]){
			case NETMSG_SETPLAYERNUM:
#ifdef DIRECT_CONTROL_ALLOWED
			case NETMSG_DIRECT_CONTROL:
#endif
				i2+=2;
				break;
#ifdef DIRECT_CONTROL_ALLOWED
			case NETMSG_DC_UPDATE:
				i2+=7;
				break;
#endif
			case NETMSG_PLAYERNAME:
			case NETMSG_CHAT:
			case NETMSG_SYSTEMMSG:
			case NETMSG_MAPNAME:
			case NETMSG_SCRIPT:
			case NETMSG_MAPDRAW:
				i2+=inbuf[i2+1];
				break;
			case NETMSG_COMMAND:
			case NETMSG_SELECT:
			case NETMSG_AICOMMAND:
				i2+=(*((short int*)&inbuf[i2+1]));
				break;
			case NETMSG_NEWFRAME:
				que++;
			case NETMSG_RANDSEED:
			case NETMSG_INTERNAL_SPEED:
			case NETMSG_USER_SPEED:
			case NETMSG_CPU_USAGE:
			case NETMSG_SYNCREQUEST:
			case NETMSG_SYNCERROR:
				i2+=5;
				break;
			case NETMSG_HELLO:
			case NETMSG_QUIT:
			case NETMSG_STARTPLAYING:
			case NETMSG_MEMDUMP:
			case NETMSG_SENDPLAYERSTAT:
			case NETMSG_GAMEOVER:
				i2++;
				break;
			case NETMSG_SHARE:
				i2+=12;
				break;
			case NETMSG_SETSHARE:
			case NETMSG_PLAYERINFO:
				i2+=10;
				break;
			case NETMSG_PLAYERSTAT:
				i2+=sizeof(CPlayer::Statistics)+2;
				break;
			case NETMSG_PAUSE:
			case NETMSG_ATTEMPTCONNECT:
			case NETMSG_PLAYERLEFT:
				i2+=3;
				break;
			case NETMSG_STARTPOS:
				i2+=15;
				break;
			default:
				info->AddLine("Unknown net msg in read ahead %i",(int)inbuf[i2]);
				i2++;
			}
		}
		if(que<leastQue)
			leastQue=que;
	}
	PUSH_CODE_MODE;
	ENTER_SYNCED;
	while((inbufpos<inbuflength) && ((timeLeft>0) || gameServer/* || net->onlyLocal*/)){
		thisMsg=inbuf[inbufpos];
		int lastLength=0;
		switch (inbuf[inbufpos]){
		case NETMSG_ATTEMPTCONNECT:
			lastLength=3;
			info->AddLine("Attempted connection to client?");
			break;

		case NETMSG_HELLO:
			lastLength=1;
			break;

		case NETMSG_QUIT:
			info->AddLine("Server exited");
			net->connected=false;
			POP_CODE_MODE;
			return gameOver;
			
		case NETMSG_PLAYERLEFT:{
			int player=inbuf[inbufpos+1];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in player left msg",player);
			} else {
				if(netbuf[inbufpos+2]==1)
					info->AddLine("Player %s left",gs->players[player]->playerName.c_str());
				else
					info->AddLine("Lost connection to %s",gs->players[player]->playerName.c_str());
				gs->players[player]->active=false;
			}
			lastLength=3;
			break;}

		case NETMSG_MEMDUMP:
			MakeMemDump();
			#ifdef TRACE_SYNC
				tracefile.Commit();
			#endif
			lastLength=1;
			break;

		case NETMSG_STARTPLAYING:
			StartPlaying();
			lastLength=1;
			break;
			
		case NETMSG_GAMEOVER:
			ENTER_MIXED;
			gameOver=true;
			#ifdef NEW_GUI
			guicontroller->Event("dialog_endgame_togglehidden");
			#else
			new CEndGameBox();
			#endif
			lastLength=1;
			ENTER_SYNCED;
			break;

		case NETMSG_SENDPLAYERSTAT:
			ENTER_MIXED;
			info->AddLine("Game over");
			netbuf[0]=NETMSG_PLAYERSTAT;
			netbuf[1]=gu->myPlayerNum;
			*(CPlayer::Statistics*)&netbuf[2]=*gs->players[gu->myPlayerNum]->currentStats;
			net->SendData(netbuf,sizeof(CPlayer::Statistics)+2);
			lastLength=1;
			ENTER_SYNCED;
			break;

		case NETMSG_PLAYERSTAT:{
			int player=inbuf[inbufpos+1];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in playerstat msg",player);
				inbufpos+=sizeof(CPlayer::Statistics)+2;
				break;
			}
			*gs->players[player]->currentStats=*(CPlayer::Statistics*)&inbuf[inbufpos+2];
			lastLength=sizeof(CPlayer::Statistics)+2;
			break;}

		case NETMSG_PAUSE:{
			int player=inbuf[inbufpos+2];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in pause msg",player);
			} else {
				gs->paused=!!inbuf[inbufpos+1];
				if(gs->paused){
					info->AddLine("%s paused the game",gs->players[player]->playerName.c_str());
				} else {
					info->AddLine("%s unpaused the game",gs->players[player]->playerName.c_str());
				}
				QueryPerformanceCounter(&lastframe);
				timeLeft=0;
			}
			lastLength=3;
			break;}

		case NETMSG_INTERNAL_SPEED:{
			if(!net->playbackDemo)
				gs->speedFactor=*((float*)&inbuf[inbufpos+1]);
			lastLength=5;
//			info->AddLine("Internal speed set to %.2f",gs->speedFactor);
			break;}

		case NETMSG_USER_SPEED:
			gs->userSpeedFactor=*((float*)&inbuf[inbufpos+1]);
			if(gs->userSpeedFactor>maxUserSpeed)
				gs->userSpeedFactor=maxUserSpeed;
			if(gs->userSpeedFactor<minUserSpeed)
				gs->userSpeedFactor=minUserSpeed;

			lastLength=5;
			info->AddLine("Speed set to %.1f",gs->userSpeedFactor);
			break;

		case NETMSG_CPU_USAGE:
			info->AddLine("Game clients shouldnt get cpu usage msgs?");
			lastLength=5;
			break;

		case NETMSG_PLAYERINFO:{
			int player=inbuf[inbufpos+1];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in playerinfo msg",player);
			} else {
				gs->players[player]->cpuUsage=*(float*)&inbuf[inbufpos+2];
				gs->players[player]->ping=*(int*)&inbuf[inbufpos+6];
			}
			lastLength=10;
			break;}

		case NETMSG_SETPLAYERNUM:
			//info->AddLine("Warning shouldnt receive NETMSG_SETPLAYERNUM in CGame");
			lastLength=2;
			break;
			
		case NETMSG_MAPNAME:
			lastLength=inbuf[inbufpos+1];				
			break;

		case NETMSG_PLAYERNAME:{
			int player=inbuf[inbufpos+2];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in playername msg",player);
			} else {
				gs->players[player]->playerName=(char*)(&inbuf[inbufpos+3]);
				gs->players[player]->readyToStart=true;
				gs->players[player]->active=true;
			}
			lastLength=inbuf[inbufpos+1];				
			break;}
						
		case NETMSG_SCRIPT:
			CScriptHandler::SelectScript((char*)(&inbuf[inbufpos+2]));
			script=CScriptHandler::Instance()->chosenScript;
			info->AddLine("Using script %s",(char*)(&inbuf[inbufpos+2]));
			lastLength=inbuf[inbufpos+1];				
			break;

		case NETMSG_CHAT:{
			int player=inbuf[inbufpos+2];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in chat msg",player);
			} else {
				string s=(char*)(&inbuf[inbufpos+3]);
				HandleChatMsg(s,player);
				inbufpos+=inbuf[inbufpos+1];	
			}
			lastLength=inbuf[inbufpos+1];	
			break;}
			
		case NETMSG_SYSTEMMSG:{
			string s=(char*)(&inbuf[inbufpos+3]);
			info->AddLine(s);
			lastLength=inbuf[inbufpos+1];	
			break;}

		case NETMSG_STARTPOS:{
			int team=inbuf[inbufpos+1];
			if(team>=gs->activeTeams || team<0 || !gameSetup){
				info->AddLine("Got invalid team num %i in startpos msg",team);
			} else {
				if(inbuf[inbufpos+2]!=2)
					gameSetup->readyTeams[team]=!!inbuf[inbufpos+2];
				gs->teams[team]->startPos.x=*(float*)&inbuf[inbufpos+3];
				gs->teams[team]->startPos.y=*(float*)&inbuf[inbufpos+7];
				gs->teams[team]->startPos.z=*(float*)&inbuf[inbufpos+11];
			}
			lastLength=15;
			break;}

		case NETMSG_RANDSEED:
			gs->randSeed=(*((unsigned int*)&netbuf[1]));
			lastLength=5;				
			break;
			
		case NETMSG_NEWFRAME:
			if(!gameServer)
				timeLeft-=1;
			
			netbuf[0]=NETMSG_NEWFRAME;
			(*((int*)&netbuf[1]))=gs->frameNum;
			net->SendData(netbuf,5);
			SimFrame();
			lastLength=5;
			if(creatingVideo && net->playbackDemo){
				POP_CODE_MODE;
				return true;
			}
			break;
			
		case NETMSG_COMMAND:{
			int player=inbuf[inbufpos+3];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in command msg",player);
			} else {
				Command c;
				c.id=*((int*)&inbuf[inbufpos+4]);
				c.options=inbuf[inbufpos+8];
				for(int a=0;a<((*((short int*)&inbuf[inbufpos+1])-9)/4);++a)
					c.params.push_back(*((float*)&inbuf[inbufpos+9+a*4]));
				selectedUnits.NetOrder(c,player);
			}
			lastLength=*((short int*)&inbuf[inbufpos+1]);
			break;}

		case NETMSG_SELECT:{
			int player=inbuf[inbufpos+3];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in netselect msg",player);
			} else {
				vector<int> selected;
				for(int a=0;a<((*((short int*)&inbuf[inbufpos+1])-4)/2);++a){
					int unitid=*((short int*)&inbuf[inbufpos+4+a*2]);
					if(unitid>=MAX_UNITS || unitid<0){
						info->AddLine("Got invalid unitid %i in netselect msg",unitid);
						lastLength=*((short int*)&inbuf[inbufpos+1]);
						break;
					}
					if(uh->units[unitid] && uh->units[unitid]->team==gs->players[player]->team)
						selected.push_back(unitid);
				}
				selectedUnits.NetSelect(selected,player);
			}
			lastLength=*((short int*)&inbuf[inbufpos+1]);
			break;}

		case NETMSG_AICOMMAND:{
			Command c;
			int player=inbuf[inbufpos+3];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in aicommand msg",player);
				lastLength=*((short int*)&inbuf[inbufpos+1]);
				break;
			}
			int unitid=*((short int*)&inbuf[inbufpos+4]);
			if(unitid>=MAX_UNITS || unitid<0){
				info->AddLine("Got invalid unitid %i in aicommand msg",unitid);
				lastLength=*((short int*)&inbuf[inbufpos+1]);
				break;
			}
			//if(uh->units[unitid] && uh->units[unitid]->team!=gs->players[player]->team)		//msgs from global ais can be for other teams
			//	info->AddLine("Warning player %i of team %i tried to send aiorder to unit from team %i",a,gs->players[player]->team,uh->units[unitid]->team);
			c.id=*((int*)&inbuf[inbufpos+6]);
			c.options=inbuf[inbufpos+10];
			for(int a=0;a<((*((short int*)&inbuf[inbufpos+1])-11)/4);++a)
				c.params.push_back(*((float*)&inbuf[inbufpos+11+a*4]));
			selectedUnits.AiOrder(unitid,c);
			lastLength=*((short int*)&inbuf[inbufpos+1]);
			break;}

		case NETMSG_SYNCREQUEST:{
			ENTER_MIXED;
			int frame=*((int*)&inbuf[inbufpos+1]);
			if(frame!=gs->frameNum){
				info->AddLine("Sync request for wrong frame");
			}

			int checksum=uh->CreateChecksum();
			netbuf[0]=NETMSG_SYNCRESPONSE;
			netbuf[1]=gu->myPlayerNum;
			*((int*)&netbuf[2])=checksum;
			*((int*)&netbuf[6])=gs->frameNum;
			net->SendData(netbuf,10);

			netbuf[0]=NETMSG_CPU_USAGE;
			*((float*)&netbuf[1])=profiler.profile["Sim time"].percent;
			net->SendData(netbuf,5);
			lastLength=5;
			ENTER_SYNCED;
			break;}

		case NETMSG_SYNCERROR:
			info->AddLine("Sync error");
			lastLength=5;
			break;

		case NETMSG_SHARE:{
			int player=inbuf[inbufpos+1];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in share msg",player);
				lastLength=12;
				break;
			}
			int team1=gs->players[player]->team;
			int team2=inbuf[inbufpos+2];
			bool shareUnits=!!inbuf[inbufpos+3];
			float metalShare=min(*(float*)&inbuf[inbufpos+4],(float)gs->teams[team1]->metal);
			float energyShare=min(*(float*)&inbuf[inbufpos+8],(float)gs->teams[team1]->energy);
			
			gs->teams[team1]->metal-=metalShare;
			gs->teams[team2]->metal+=metalShare;
			gs->teams[team1]->energy-=energyShare;
			gs->teams[team2]->energy+=energyShare;
	
			if(shareUnits){
				for(vector<int>::iterator ui=selectedUnits.netSelected[player].begin();ui!=selectedUnits.netSelected[player].end();++ui){
					if(uh->units[*ui] && uh->units[*ui]->team==team1 && !uh->units[*ui]->beingBuilt){
						uh->units[*ui]->ChangeTeam(team2,CUnit::ChangeGiven);
					}
				}
				selectedUnits.netSelected[player].clear();
			}
			lastLength=12;
			break;}

		case NETMSG_SETSHARE:{
			int team=inbuf[inbufpos+1];
			if(team>=gs->activeTeams || team<0){
				info->AddLine("Got invalid team num %i in setshare msg",team);
			} else {
				float metalShare=*(float*)&inbuf[inbufpos+2];
				float energyShare=*(float*)&inbuf[inbufpos+6];
				
				gs->teams[team]->metalShare=metalShare;
				gs->teams[team]->energyShare=energyShare;
			}
			lastLength=10;
			break;}

		case NETMSG_MAPDRAW:{
			inMapDrawer->GotNetMsg(&inbuf[inbufpos]);
			lastLength=inbuf[inbufpos+1];
			break;}

#ifdef DIRECT_CONTROL_ALLOWED
		case NETMSG_DIRECT_CONTROL:{
			int player=inbuf[inbufpos+1];

			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in direct control msg",player);
				lastLength=2;
				break;
			}

			if(gs->players[player]->playerControlledUnit){
				CUnit* unit=gs->players[player]->playerControlledUnit;
				//info->AddLine("Player %s released control over unit %i type %s",gs->players[player]->playerName.c_str(),unit->id,unit->unitDef->humanName.c_str());

				unit->directControl=0;
				unit->AttackUnit(0,true);
				gs->players[player]->playerControlledUnit=0;
				ENTER_UNSYNCED;
				if(player==gu->myPlayerNum){
					gu->directControl=0;
					mouse->currentCamController=mouse->camControllers[mouse->currentCamControllerNum];
					mouse->currentCamController->SetPos(camera->pos);
					mouse->inStateTransit=true;
					mouse->transitSpeed=1;
				}
				ENTER_SYNCED;
			} else {
				if(!selectedUnits.netSelected[player].empty() && uh->units[selectedUnits.netSelected[player][0]] && !uh->units[selectedUnits.netSelected[player][0]]->weapons.empty()){
					CUnit* unit=uh->units[selectedUnits.netSelected[player][0]];
					//info->AddLine("Player %s took control over unit %i type %s",gs->players[player]->playerName.c_str(),unit->id,unit->unitDef->humanName.c_str());

					unit->directControl=&gs->players[player]->myControl;
					gs->players[player]->playerControlledUnit=unit;
					ENTER_UNSYNCED;
					if(player==gu->myPlayerNum){
						gu->directControl=unit;
						mouse->currentCamController=mouse->camControllers[0];	//set fps mode
						mouse->inStateTransit=true;
						mouse->transitSpeed=1;
						((CFPSController*)mouse->camControllers[0])->pos=unit->midPos;
						if(!mouse->locked){
							mouse->locked=true;
							mouse->HideMouse();
						}
						selectedUnits.ClearSelected();
					}
					ENTER_SYNCED;
				}
			}
			lastLength=2;
			break;}

		case NETMSG_DC_UPDATE:{
			int player=inbuf[inbufpos+1];
			if(player>=MAX_PLAYERS || player<0){
				info->AddLine("Got invalid player num %i in dc update msg",player);
				lastLength=7;
				break;
			}
			DirectControlStruct* dc=&gs->players[player]->myControl;
			CUnit* unit=gs->players[player]->playerControlledUnit;

			dc->forward=!!(inbuf[inbufpos+2] & 1);
			dc->back=!!(inbuf[inbufpos+2] & 2);
			dc->left=!!(inbuf[inbufpos+2] & 4);
			dc->right=!!(inbuf[inbufpos+2] & 8);
			dc->mouse1=!!(inbuf[inbufpos+2] & 16);
			bool newMouse2=!!(inbuf[inbufpos+2] & 32);
			if(!dc->mouse2 && newMouse2 && unit){
				unit->AttackUnit(0,true);
//				for(std::vector<CWeapon*>::iterator wi=unit->weapons.begin();wi!=unit->weapons.end();++wi)
//					(*wi)->HoldFire();
			}
			dc->mouse2=newMouse2;

			short int h=*((short int*)&inbuf[inbufpos+3]);
			short int p=*((short int*)&inbuf[inbufpos+5]);
			dc->viewDir=GetVectorFromHAndPExact(h,p);

			lastLength=7;
			break;}
#endif

		default:
			char txt[200];
			sprintf(txt,"Unknown net msg in client %d last %d",(int)inbuf[inbufpos],lastMsg);
			info->AddLine(txt);
			//MessageBox(0,txt,"Network error in CGame",0);
			lastLength=1;
			break;
		}
		if(lastLength<=0){
			info->AddLine("Client readnet got packet type %i length %i pos %i??",thisMsg,lastLength,inbufpos);
			lastLength=0;
		}
		inbufpos+=lastLength;
		lastMsg=thisMsg;
	}
	POP_CODE_MODE;
	return true;
}


void CGame::UpdateUI()
{
	ASSERT_UNSYNCED_MODE;
	//move camera if arrow keys pressed
#ifdef DIRECT_CONTROL_ALLOWED
	if(gu->directControl){
		CUnit* owner=gu->directControl;

		std::vector<long> args;
		args.push_back(0);
		owner->cob->Call(COBFN_AimFromPrimary/*/COBFN_QueryPrimary+weaponNum/**/,args);
		float3 relPos=owner->localmodel->GetPiecePos(args[0]);
		float3 pos=owner->pos+owner->frontdir*relPos.z+owner->updir*relPos.y+owner->rightdir*relPos.x;
		pos+=UpVector*7;

		((CFPSController*)mouse->camControllers[0])->pos=pos;
	} else
#endif
	{
		float cameraSpeed=1;
		if (camMove[7]){
			cameraSpeed*=0.1f;
		}
		if (camMove[6]){
			cameraSpeed*=10;
		}
		float3 movement(0,0,0);

		if (camMove[0]){
			movement.y+=gu->lastFrameTime;
			trackingUnit=false;
		}
		if (camMove[1]){
			movement.y-=gu->lastFrameTime;
			trackingUnit=false;
		}
		if (camMove[3]){
			movement.x+=gu->lastFrameTime;
			trackingUnit=false;
		}
		if (camMove[2]){
			movement.x-=gu->lastFrameTime;
			trackingUnit=false;
		}
		movement.z=cameraSpeed;
		mouse->currentCamController->KeyMove(movement);

		movement=float3(0,0,0);

		if ((mouse->lasty<2 && fullscreen)){
			movement.y+=gu->lastFrameTime;
			trackingUnit=false;
		}
		if ((mouse->lasty>gu->screeny-2 && fullscreen)){
			movement.y-=gu->lastFrameTime;
			trackingUnit=false;
		}
		if ((mouse->lastx>gu->screenx-2 && fullscreen)){
			movement.x+=gu->lastFrameTime;
			trackingUnit=false;
		}
		if ((mouse->lastx<2 && fullscreen)){
			movement.x-=gu->lastFrameTime;
			trackingUnit=false;
		}
		movement.z=cameraSpeed;
		mouse->currentCamController->ScreenEdgeMove(movement);

		if(camMove[4])
			mouse->currentCamController->MouseWheelMove(gu->lastFrameTime*200*cameraSpeed);
		if(camMove[5])
			mouse->currentCamController->MouseWheelMove(-gu->lastFrameTime*200*cameraSpeed);
	}

	if(chatting && !userWriting){
		if(userInput.size()>0){
			netbuf[0]=NETMSG_CHAT;
			netbuf[1]=userInput.size()+4;
			netbuf[2]=gu->myPlayerNum;
			for(unsigned int a=0;a<userInput.size();a++)
				netbuf[a+3]=userInput.at(a);
			netbuf[userInput.size()+3]=0;
			net->SendData(netbuf,netbuf[1]);
			if(net->playbackDemo)
				HandleChatMsg(userInput,gu->myPlayerNum);
		}
		chatting=false;
		userInput="";
	}
	if(inMapDrawer->wantLabel && !userWriting){
		inMapDrawer->CreatePoint(inMapDrawer->waitingPoint,userInput);
		inMapDrawer->wantLabel=false;
		userInput="";
		ignoreChar=0;
	}
}


/*
unsigned int crcTable[256];

unsigned int Calculate_CRC(char *first,char *last)
{  int length = ((int) (last - first));  unsigned int crc = 0xFFFFFFFF;
  for(int i=0; i<length; i++)
    {
    crc = (crc>>8) & 0x00FFFFFF ^ crcTable[(crc^first[i]) & 0xFF];
    }  return(crc);}



void InitCRCTable()
{
  unsigned int crc;
  for (int i=0; i<256; i++)
    {
    crc = i;
    for(int j=8; j>0; j--)
      {
      if (crc&1)
        {
        crc = (crc >> 1) ^ 0xEDB88320;
        }
      else
        {
        crc >>= 1;
        }
     }
     crcTable[i] = crc;
   }
  }

  */

void CGame::MakeMemDump(void)
{
	ofstream file;
	if(gameServer) {
		boost::filesystem::path fn("memdump.txt");
		file.open(fn.native_file_string().c_str(),ios::out);
	} else {
		boost::filesystem::path fn("memdumpclient.txt");
		file.open(fn.native_file_string().c_str(),ios::out);
	}
	
	file << "Frame " << gs->frameNum <<"\n";
	list<CUnit*>::iterator usi;
	for(usi=uh->activeUnits.begin();usi!=uh->activeUnits.end();usi++){
		CUnit* u=*usi;
		file << "Unit " << u->id << "\n";
		file << "  xpos " << u->pos.x << " ypos " << u->pos.y << " zpos " << u->pos.z << "\n";
		file << "  heading " << u->heading << " power " << u->power << " experience " << u->experience << "\n";
		file << " health " << u->health << "\n";
	}
	Projectile_List::iterator psi;
	for(psi=ph->ps.begin();psi != ph->ps.end();++psi){
		CProjectile* p=*psi;
		file << "Projectile " << p->radius << "\n";
		file << "  xpos " << p->pos.x << " ypos " << p->pos.y << " zpos " << p->pos.z << "\n";
		file << "  xspeed " << p->speed.x << " yspeed " << p->speed.y << " zspeed " << p->speed.z << "\n";
	}
	for(int a=0;a<gs->activeTeams;++a){
		file << "Losmap for team " << a << "\n";
		for(int y=0;y<gs->mapy/2;++y){
			file << " ";
			for(int x=0;x<gs->mapx/2;++x){
				file << loshandler->losMap[a][y*gs->mapx/2+x] << " ";
			}
			file << "\n";
		}
	}
	file.close();
	info->AddLine("Memdump finished");
}

void CGame::DrawDirectControlHud(void)
{
#ifdef DIRECT_CONTROL_ALLOWED
	CUnit* unit=gu->directControl;
	glPushMatrix();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_TEXTURE_2D);

	glPushMatrix();
	glTranslatef(0.1,0.1,0);
	glScalef(0.25,0.25,0.25);

	if(unit->moveType->useHeading){
		glPushMatrix();
		glRotatef(unit->heading*180.0/32768+180,0,0,1);
		
		glColor4d(0.3,0.9,0.3,0.4);
		glBegin(GL_TRIANGLE_FAN);
		glVertex2d(-0.2,-0.3);
		glVertex2d(-0.2,0.3);
		glVertex2d(0,0.4);
		glVertex2d(0.2,0.3);
		glVertex2d(0.2,-0.3);
		glVertex2d(-0.2,-0.3);
		glEnd();
		glPopMatrix();
	}

	glEnable(GL_DEPTH_TEST);
	glPushMatrix();
	if(unit->moveType->useHeading){
		float scale=0.4/unit->radius;
		glScalef(scale,scale,scale);
		glRotatef(90,1,0,0);
	} else {
		float scale=0.2/unit->radius;
		glScalef(scale,scale,-scale);
		CMatrix44f m(ZeroVector,float3(camera->right.x,camera->up.x,camera->forward.x),float3(camera->right.y,camera->up.y,camera->forward.y),float3(camera->right.z,camera->up.z,camera->forward.z));
		glMultMatrixf(m.m);
	}
	glTranslatef3(-unit->pos-unit->speed*gu->timeOffset);
	uh->SetupForUnitDrawing();
	unit->Draw();
	glPopMatrix();
	glDisable(GL_DEPTH_TEST);
	uh->CleanUpUnitDrawing();
	glDisable(GL_TEXTURE_2D);

	if(unit->moveType->useHeading){
		glPushMatrix();
		glRotatef(GetHeadingFromVector(camera->forward.x,camera->forward.z)*180.0/32768+180,0,0,1);
		glScalef(0.4,0.4,0.3);
		
		glColor4d(0.4,0.4,1.0,0.6);
		glBegin(GL_TRIANGLE_FAN);
		glVertex2d(-0.2,-0.3);
		glVertex2d(-0.2,0.3);
		glVertex2d(0,0.5);
		glVertex2d(0.2,0.3);
		glVertex2d(0.2,-0.3);
		glVertex2d(-0.2,-0.3);
		glEnd();
		glPopMatrix();
	}
	glPopMatrix();

	glEnable(GL_TEXTURE_2D);

	glPushMatrix();
	glTranslatef(0.22,0.05,0);
	glScalef(0.03,0.03,0.03);
	glColor4d(0.2,0.8,0.2,0.6);
	font->glPrint("Health %.0f",unit->health);
	glPopMatrix();

	if(gs->players[gu->myPlayerNum]->myControl.mouse2){
		glPushMatrix();
		glTranslatef(0.02,0.34,0);
		glScalef(0.03,0.03,0.03);
		glColor4d(0.2,0.8,0.2,0.6);
		font->glPrint("Free fire mode");
		glPopMatrix();
	}
	for(int a=0;a<unit->weapons.size();++a){
		glPushMatrix();
		glTranslatef(0.02,0.3-a*0.04,0);
		glScalef(0.03,0.03,0.03);
		CWeapon* w=unit->weapons[a];
		if(w->reloadStatus>gs->frameNum){
			glColor4d(0.8,0.2,0.2,0.6);
			font->glPrint("Weapon %i: Reloading",a+1);
		} else if(!w->angleGood){
			glColor4d(0.6,0.6,0.2,0.6);
			font->glPrint("Weapon %i: Aiming",a+1);
		} else {
			glColor4d(0.2,0.8,0.2,0.6);
			font->glPrint("Weapon %i: Ready",a+1);
		}
		glPopMatrix();
	}

	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);						
	glPushMatrix();
	camera->Update(false);		//draw some stuff in world coordinates
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);

	for(int a=0;a<unit->weapons.size();++a){
		CWeapon* w=unit->weapons[a];
		if(!w){
			info->AddLine("Null weapon in vector?");
			return;
		}
		switch(a){
		case 0:
			glColor4d(0,1,0,0.7);
			break;
		case 1:
			glColor4d(1,0,0,0.7);
			break;
		default:
			glColor4d(0,0,1,0.7);
		}
		if(w->targetType!=Target_None){
			float3 pos=w->targetPos;
			float3 v1=(pos-camera->pos).Normalize();
			float3 v2=(v1.cross(UpVector)).Normalize();
			float3 v3=(v2.cross(v1)).Normalize();
			float radius=10;
			if(w->targetType==Target_Unit)
				radius=w->targetUnit->radius;

			glBegin(GL_LINE_STRIP);
			for(int b=0;b<=80;++b){
				glVertexf3(pos+(v2*sin(b*2*PI/80)+v3*cos(b*2*PI/80))*radius);
			}
			glEnd();

			if(!w->onlyForward){
				float dist=min(w->owner->directControl->targetDist,w->range*0.9);
				pos=w->weaponPos+w->wantedDir*dist;
				v1=(pos-camera->pos).Normalize();
				v2=(v1.cross(UpVector)).Normalize();
				v3=(v2.cross(v1)).Normalize();
				radius=dist/100;

				glBegin(GL_LINE_STRIP);
				for(int b=0;b<=80;++b){
					glVertexf3(pos+(v2*sin(b*2*PI/80)+v3*cos(b*2*PI/80))*radius);
				}
				glEnd();
			}
			glBegin(GL_LINES);
			if(!w->onlyForward){
				glVertexf3(pos);
				glVertexf3(w->targetPos);

				glVertexf3(pos+(v2*sin(PI*0.25)+v3*cos(PI*0.25))*radius);
				glVertexf3(pos+(v2*sin(PI*1.25)+v3*cos(PI*1.25))*radius);

				glVertexf3(pos+(v2*sin(PI*-0.25)+v3*cos(PI*-0.25))*radius);
				glVertexf3(pos+(v2*sin(PI*-1.25)+v3*cos(PI*-1.25))*radius);
			}
			if((w->targetPos-camera->pos).Normalize().dot(camera->forward)<0.7){
				glVertexf3(w->targetPos);
				glVertexf3(camera->pos+camera->forward*100);
			}
			glEnd();
		}
	}
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);						
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);

	glPopMatrix();
#endif
}

void CGame::HandleChatMsg(std::string s,int player)
{
	globalAI->GotChatMsg(s.c_str(),player);

	if(s.find(".cheat")==0 && player==0){
		gs->cheatEnabled=true;
		info->AddLine("Cheating!");
	}
	if(s.find(".nocost")==0 && player==0 && gs->cheatEnabled){
		for(int i=0; i<unitDefHandler->numUnits; i++)
		{
			unitDefHandler->unitDefs[i].metalCost = 1;
			unitDefHandler->unitDefs[i].energyCost = 1;
			unitDefHandler->unitDefs[i].buildTime = 10;
			unitDefHandler->unitDefs[i].metalUpkeep = 0;
			unitDefHandler->unitDefs[i].energyUpkeep = 0;
		}
		unitDefHandler->noCost=true;
		info->AddLine("Cheating!");
	}

	if (s.find(".crash") == 0 && gs->cheatEnabled) {
		int *a=0;
		*a=0;
	}

	if(s==".spectator" && gs->cheatEnabled){
		gs->players[player]->spectator=true;
		if(player==gu->myPlayerNum)
			gu->spectating=true;
	}
	if(s.find(".team")==0 && gs->cheatEnabled){
		int team=atoi(&s.c_str()[s.find(" ")]);
		if(team>=0 && team<gs->activeTeams){
			gs->players[player]->team=team;
			gs->players[player]->spectator=false;
			if(player==gu->myPlayerNum){
				gu->spectating=false;
				gu->myTeam=team;
				gu->myAllyTeam=gs->team2allyteam[gu->myTeam];
				grouphandler->team=gu->myTeam;
			}
		}
	}
	if(s.find(".atm")==0 && gs->cheatEnabled){
		int team=gs->players[player]->team;
		gs->teams[team]->AddMetal(1000);
		gs->teams[team]->AddEnergy(1000);
	}
	if(s.find(".nospectatorchat")==0 && player==0){
		noSpectatorChat=!noSpectatorChat;
		info->AddLine("No spectator chat %i",noSpectatorChat);
	}

	if(s.find(".clock")==0 && player==gu->myPlayerNum){
		showClock=!showClock;
	}
	if(s.find(".info")==0 && player==gu->myPlayerNum){
		showPlayerInfo=!showPlayerInfo;
	}
	if(s.find(".nopause")==0 && player==0){
		gamePausable=!gamePausable;
	}
	if(s.find(".setmaxspeed")==0 && player==0){
		maxUserSpeed=atof(s.substr(12).c_str());
		if(gs->userSpeedFactor>maxUserSpeed)
			gs->userSpeedFactor=maxUserSpeed;
	}
	if(s.find(".setminspeed")==0 && player==0){
		minUserSpeed=atof(s.substr(12).c_str());
		if(gs->userSpeedFactor<minUserSpeed)
			gs->userSpeedFactor=minUserSpeed;
	}

	if(noSpectatorChat && gs->players[player]->spectator && !gu->spectating)
		return;

	if((s[0]=='a' || s[0]=='A') && s[1]==':'){
		if((gs->allies[gs->team2allyteam[gs->players[inbuf[inbufpos+2]]->team]][gu->myAllyTeam] && !gs->players[player]->spectator) || gu->spectating){
			s="<"+gs->players[player]->playerName+"> Allies: "+s.substr(2,255);
			info->AddLine(s);
			sound->PlaySound(chatSound);
		}
	} else if((s[0]=='s' || s[0]=='S') && s[1]==':'){
		if(gu->spectating || gu->myPlayerNum == player){
			s="<"+gs->players[player]->playerName+"> Spectators: "+s.substr(2,255);
			info->AddLine(s);
			sound->PlaySound(chatSound);
		}
	} else {
		s="<"+gs->players[player]->playerName+"> "+s;
		info->AddLine(s);
		sound->PlaySound(chatSound);
	}
}

unsigned  int CGame::CreateExeChecksum(void)
{
	unsigned int ret=0;
#ifdef _WIN32
	CFileHandler f("spring.exe");
	int l=f.FileSize();
	unsigned char* buf=new unsigned char[l];
	f.Read(buf,l);

	for(int a=0;a<l;++a){
		ret+=buf[a];
		ret*=max(1,buf[a]);
	}

	delete[] buf;
#endif
	return ret;
}
