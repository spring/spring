#include "stdafx.h"
#include ".\gameserver.h"
#include "net.h"
#include "player.h"
#include "team.h"
#include "infoconsole.h"
#include "game.h"
#include "gamesetup.h"
#include "scripthandler.h"

CGameServer* gameServer=0;

extern string stupidGlobalMapname;
extern int stupidGlobalMapId;
extern bool globalQuit;

static DWORD WINAPI GameServerThreadProc(LPVOID lpParameter)
{
	((CGameServer*)lpParameter)->UpdateLoop();
	return 0;
}


CGameServer::CGameServer(void)
{
	makeMemDump=true;
	serverframenum=0;
	timeLeft=0;
	gameEndDetected=false;
	gameEndTime=0;
	outstandingSyncFrame=-1;
	lastSyncRequest=0;
	quitServer=false;
	gameClientUpdated=false;
	maxTimeLeft=2;

	int port=8452;
	if(gameSetup)
		port=gameSetup->hostport;

	serverNet=new CNet();
	serverNet->InitServer(port);
	net->InitClient("localhost",port,true);
	serverNet->InitNewConn(&net->connections[0].addr,true,0);
	serverNet->connections[0].localConnection=&net->connections[0];
	net->connections[0].localConnection=&serverNet->connections[0];
	net->onlyLocal=true;

	netbuf[0]=NETMSG_SCRIPT;
	netbuf[1]=CScriptHandler::Instance()->chosenName.size()+3;
	for(unsigned int b=0;b<CScriptHandler::Instance()->chosenName.size();b++)
		netbuf[b+2]=CScriptHandler::Instance()->chosenName.at(b);
	netbuf[CScriptHandler::Instance()->chosenName.size()+2]=0;
	serverNet->SendData(netbuf,CScriptHandler::Instance()->chosenName.size()+3);

	outbuf[0]=NETMSG_MAPNAME;
	outbuf[1]=stupidGlobalMapname.size()+7;
	*(int*)&outbuf[2]=stupidGlobalMapId;
	for(unsigned int a=0;a<stupidGlobalMapname.size();a++)
		outbuf[a+6]=stupidGlobalMapname.at(a);
	outbuf[stupidGlobalMapname.size()+6]=0;
	serverNet->SendData(outbuf,stupidGlobalMapname.size()+7);

	QueryPerformanceCounter(&lastframe);
	QueryPerformanceFrequency(&timeSpeed);

	exeChecksum=game->CreateExeChecksum();

	gameServerMutex=CreateMutex(0,false,"SpringGameServerMutex");
#ifndef SYNCIFY		//syncify doesnt really support multithreading...
	thisThread=CreateThread(0,0,GameServerThreadProc,this,0,0);
#endif
}

CGameServer::~CGameServer(void)
{
	CloseHandle(gameServerMutex);
	delete serverNet;
	serverNet=0;
}

bool CGameServer::Update(void)
{
	if(lastSyncRequest<gu->gameTime-2){
		lastSyncRequest=gu->gameTime;

		if(game->playing){
			//check if last times sync responses is correct
			if(outstandingSyncFrame>0){
				int correctSync=0;
				for(int a=0;a<MAX_PLAYERS;++a){
					if(gs->players[a]->active){
						if(!syncResponses[a]){		//if the checksum really happens to be 0 we will get lots of falls positives here
							info->AddLine("No sync resonse from %s",gs->players[a]->playerName.c_str());
							continue;
						}
						if(correctSync && correctSync!=syncResponses[a]){
							SendSystemMsg("Sync error for %s %i %X %X",gs->players[a]->playerName.c_str(),outstandingSyncFrame,correctSync,syncResponses[a]);
							continue;
						}
						correctSync=syncResponses[a];		//this assumes the lowest num player is the correct one, should maybe use some sort of majority voting instead
					}
				}
			}
			for(int a=0;a<MAX_PLAYERS;++a)
				syncResponses[a]=0;
			
			//send sync request
			outbuf[0]=NETMSG_SYNCREQUEST;
			(*((int*)&outbuf[1]))=serverframenum;
			serverNet->SendData(outbuf,5);
			outstandingSyncFrame=serverframenum;

			//decide new internal speed
			float maxCpu=0;
			for(int a=0;a<MAX_PLAYERS;++a){
				if(gs->players[a]->cpuUsage>maxCpu && gs->players[a]->active){
					maxCpu=gs->players[a]->cpuUsage;
				}
			}

			float wantedCpu=0.35+(1-gs->speedFactor/gs->userSpeedFactor)*0.5;
			//float speedMod=1+wantedCpu-maxCpu;
			float newSpeed=gs->speedFactor*wantedCpu/maxCpu;
			//info->AddLine("Speed %f %f %f %f",maxCpu,wantedCpu,speedMod,newSpeed);
			newSpeed=(newSpeed+gs->speedFactor)*0.5;
			if(newSpeed>gs->userSpeedFactor)
				newSpeed=gs->userSpeedFactor;
			if(newSpeed<0.1)
				newSpeed=0.1;
			if(newSpeed!=gs->speedFactor){
				outbuf[0]=NETMSG_INTERNAL_SPEED;
				*((float*)&outbuf[1])=newSpeed;
				serverNet->SendData(outbuf,5);
			}
		}
	}

	if(!ServerReadNet()){
		info->AddLine("Server read net wanted quit");
		return false;
	}
	if (game->playing){
		LARGE_INTEGER currentFrame;
		QueryPerformanceCounter(&currentFrame);
		
		double timeElapsed=((double)(currentFrame.QuadPart - lastframe.QuadPart))/timeSpeed.QuadPart;
		if(gameEndDetected)
			gameEndTime+=timeElapsed;
//		info->AddLine("float value is %f",timeElapsed);

		if(gameClientUpdated){
			gameClientUpdated=false;
			maxTimeLeft=2;
		}
		if(timeElapsed>maxTimeLeft)
			timeElapsed=maxTimeLeft;
		maxTimeLeft-=timeElapsed;

		timeLeft+=GAME_SPEED*gs->speedFactor*timeElapsed;
		lastframe=currentFrame;
		
		while((timeLeft>0) && (!gs->paused || game->bOneStep)){
			if(!game->creatingVideo){
				game->bOneStep=false;
				CreateNewFrame(true);
			}
			timeLeft--;
		}
	}
	serverNet->Update();

	CheckForGameEnd();
	return true;
}

bool CGameServer::ServerReadNet()
{
	static int lastMsg[MAX_PLAYERS],thisMsg=0;

	for(int a=0;a<MAX_PLAYERS;a++){
		int inbufpos=0;
		int inbuflength=0;
		if(gs->players[a]->active){
			if((inbuflength=serverNet->GetData(inbuf,NETWORK_BUFFER_SIZE,a))==-1){
				PUSH_CODE_MODE;		//this could lead to some nasty errors if the other thread switches code mode...
				ENTER_MIXED;
				gs->players[a]->active=false;
				POP_CODE_MODE;
				inbuflength=0;
				info->AddLine("Lost connection to "+gs->players[a]->playerName);
			}
		}
		//		(*info) << serverNet->numConnected << "\n";

		while(inbufpos<inbuflength){
			thisMsg=inbuf[inbufpos];
			switch (inbuf[inbufpos]){

			case NETMSG_ATTEMPTCONNECT: //handled directly in CNet
				inbufpos+=3;
				break;

			case NETMSG_HELLO: 
				inbufpos++;
				break;

			case NETMSG_NEWFRAME: 
				inbufpos+=5;
				break;

			case NETMSG_PAUSE:
				if(inbuf[inbufpos+2]!=a){
					info->AddLine("Server: Warning got pause msg from %i claiming to be from %i",a,inbuf[inbufpos+2]);
				} else {
					timeLeft=0;
					serverNet->SendData(&inbuf[inbufpos],3);
				}
				inbufpos+=3;
				break;

			case NETMSG_INTERNAL_SPEED: 
				info->AddLine("Server shouldnt get internal speed msgs?");
				inbufpos+=5;
				break;

			case NETMSG_USER_SPEED: 
				if(gs->speedFactor==gs->userSpeedFactor || gs->speedFactor>*((float*)&inbuf[inbufpos+1])){
					unsigned char t[5];
					t[0]=NETMSG_INTERNAL_SPEED;
					*((float*)&t[1])=*((float*)&inbuf[inbufpos+1]);
					serverNet->SendData(t,5);
				}
				serverNet->SendData(&inbuf[inbufpos],5);
				inbufpos+=5;
				break;

			case NETMSG_CPU_USAGE: 
				ENTER_MIXED;
				gs->players[a]->cpuUsage=*((float*)&inbuf[inbufpos+1]);
				ENTER_UNSYNCED;
				inbufpos+=5;
				break;

			case NETMSG_EXECHECKSUM: 
				if(exeChecksum!=*((unsigned int*)&inbuf[inbufpos+1])){
					SendSystemMsg("Wrong exe checksum from %i got %X instead of %X",a,*((unsigned int*)&inbuf[inbufpos+1]),exeChecksum);
				}
				inbufpos+=5;
				break;

			case NETMSG_MAPNAME:
				inbufpos+=inbuf[inbufpos+1];				
				break;

			case NETMSG_QUIT:
				ENTER_MIXED;
				gs->players[a]->active=false;
				ENTER_UNSYNCED;
				serverNet->connections[a].active=false;
				inbufpos++;
				SendSystemMsg("Player %s quit",gs->players[a]->playerName.c_str());
				break;

			case NETMSG_PLAYERNAME:
				if(inbuf[inbufpos+2]!=a && a!=0){
					SendSystemMsg("Server: Warning got playername msg from %i claiming to be from %i",a,inbuf[inbufpos+2]);
				} else {
					ENTER_MIXED;
					gs->players[inbuf[inbufpos+2]]->playerName=(char*)(&inbuf[inbufpos+3]);
					gs->players[inbuf[inbufpos+2]]->readyToStart=true;
					gs->players[inbuf[inbufpos+2]]->active=true;
					ENTER_UNSYNCED;

					SendSystemMsg("Player %s joined as %i",&inbuf[inbufpos+3],inbuf[inbufpos+2]);
					serverNet->SendData(&inbuf[inbufpos],inbuf[inbufpos+1]);
				}
				inbufpos+=inbuf[inbufpos+1];
				break;

			case NETMSG_CHAT:
				if(inbuf[inbufpos+2]!=a){
					SendSystemMsg("Server: Warning got chat msg from %i claiming to be from %i",a,inbuf[inbufpos+2]);
				} else {
					serverNet->SendData(&inbuf[inbufpos],inbuf[inbufpos+1]);
				}
				inbufpos+=inbuf[inbufpos+1];	
				break;

			case NETMSG_SYSTEMMSG:
				if(inbuf[inbufpos+2]!=a){
					info->AddLine("Server: Warning got system msg from %i claiming to be from %i",a,inbuf[inbufpos+2]);
				} else {
					serverNet->SendData(&inbuf[inbufpos],inbuf[inbufpos+1]);
				}
				inbufpos+=inbuf[inbufpos+1];	
				break;

			case NETMSG_STARTPOS:
				if(inbuf[inbufpos+1]!=gs->players[a]->team && a!=0){
					SendSystemMsg("Server: Warning got select msg from %i claiming to be from team %i",a,inbuf[inbufpos+1]);
				} else {
					serverNet->SendData(&inbuf[inbufpos],15);
				}
				inbufpos+=15;
				break;

			case NETMSG_COMMAND:
				if(inbuf[inbufpos+3]!=a){
					SendSystemMsg("Server: Warning got command msg from %i claiming to be from %i",a,inbuf[inbufpos+3]);
				} else {
					serverNet->SendData(&inbuf[inbufpos],*((short int*)&inbuf[inbufpos+1]));
				}
				inbufpos+=*((short int*)&inbuf[inbufpos+1]);
				break;

			case NETMSG_SELECT:
				if(inbuf[inbufpos+3]!=a){
					SendSystemMsg("Server: Warning got select msg from %i claiming to be from %i",a,inbuf[inbufpos+3]);
				} else {
					serverNet->SendData(&inbuf[inbufpos],*((short int*)&inbuf[inbufpos+1]));
				}
				inbufpos+=*((short int*)&inbuf[inbufpos+1]);
				break;

			case NETMSG_AICOMMAND:
				if(inbuf[inbufpos+3]!=a){
					SendSystemMsg("Server: Warning got aicommand msg from %i claiming to be from %i",a,inbuf[inbufpos+3]);
				} else {
					serverNet->SendData(&inbuf[inbufpos],*((short int*)&inbuf[inbufpos+1]));
				}
				inbufpos+=*((short int*)&inbuf[inbufpos+1]);
				break;

			case NETMSG_SYNCRESPONSE:{
				if(inbuf[inbufpos+1]!=a){
					SendSystemMsg("Server: Warning got syncresponse msg from %i claiming to be from %i",a,inbuf[inbufpos+1]);
				} else {
					if(outstandingSyncFrame==*(int*)&inbuf[inbufpos+6])
						syncResponses[inbuf[inbufpos+1]]=*(int*)&inbuf[inbufpos+2];
					else
						info->AddLine("Delayed sync respone from %s",gs->players[inbuf[inbufpos+1]]->playerName.c_str());
				}
				inbufpos+=10;}
				break;

			case NETMSG_SHARE:
				if(inbuf[inbufpos+1]!=a){
					SendSystemMsg("Server: Warning got share msg from %i claiming to be from %i",a,inbuf[inbufpos+1]);
				} else {
					serverNet->SendData(&inbuf[inbufpos],12);
				}
				inbufpos+=12;
				break;

			case NETMSG_SETSHARE:
				if(inbuf[inbufpos+1]!=gs->players[a]->team){
					SendSystemMsg("Server: Warning got setshare msg from player %i claiming to be from team %i",a,inbuf[inbufpos+1]);
				} else {
					serverNet->SendData(&inbuf[inbufpos],10);
				}
				inbufpos+=10;
				break;

			case NETMSG_PLAYERSTAT:
				if(inbuf[inbufpos+1]!=a){
					SendSystemMsg("Server: Warning got stat msg from %i claiming to be from %i",a,inbuf[inbufpos+1]);
				} else {
					serverNet->SendData(&inbuf[inbufpos],sizeof(CPlayer::Statistics)+2);
				}
				inbufpos+=sizeof(CPlayer::Statistics)+2;
				break;

			case NETMSG_MAPDRAW:
				serverNet->SendData(&inbuf[inbufpos],inbuf[inbufpos+1]);
				inbufpos+=inbuf[inbufpos+1];				
				break;

#ifdef DIRECT_CONTROL_ALLOWED
			case NETMSG_DIRECT_CONTROL:
				if(inbuf[inbufpos+1]!=a){
					SendSystemMsg("Server: Warning got direct control msg from %i claiming to be from %i",a,inbuf[inbufpos+1]);
				} else {
					serverNet->SendData(&inbuf[inbufpos],2);
				}
				inbufpos+=2;
				break;

			case NETMSG_DC_UPDATE:
				if(inbuf[inbufpos+1]!=a){
					SendSystemMsg("Server: Warning got dc update msg from %i claiming to be from %i",a,inbuf[inbufpos+1]);
				} else {
					serverNet->SendData(&inbuf[inbufpos],7);
				}
				inbufpos+=7;
				break;
#endif
			default:
				char txt[200];
				sprintf(txt,"Unknown net msg in server %d from %d pos %d last %d",(int)inbuf[inbufpos],a,inbufpos,lastMsg[a]);
				info->AddLine(txt);
				//MessageBox(0,txt,"Network error",0);
				inbufpos++;
				break;
			}
			lastMsg[a]=thisMsg;
		}
		if(inbufpos!=inbuflength){
			char txt[200];
			sprintf(txt,"Wrong packet length got %d from %d instead of %d",inbufpos,a,inbuflength);
			info->AddLine(txt);
			MessageBox(0,txt,"Network error",0);
		}
	}
	return true;
}

void CGameServer::StartGame(void)
{
	WaitForSingleObject(gameServerMutex,INFINITE);
	serverNet->StopListening();
	for(int a=0;a<MAX_PLAYERS;a++){
		if(!gs->players[a]->active)
			continue;
		outbuf[0]=NETMSG_PLAYERNAME;
		outbuf[1]=gs->players[a]->playerName.size()+4;
		outbuf[2]=a;
		for(unsigned int b=0;b<gs->players[a]->playerName.size();b++)
			outbuf[b+3]=gs->players[a]->playerName.at(b);
		outbuf[gs->players[a]->playerName.size()+3]=0;
		serverNet->SendData(outbuf,gs->players[a]->playerName.size()+4);
	}
	if(gameSetup){
		for(int a=0;a<gs->activeTeams;a++){
			outbuf[0]=NETMSG_STARTPOS;
			outbuf[1]=a;
			outbuf[2]=1;
			*(float*)&outbuf[3]=gs->teams[a]->startPos.x;
			*(float*)&outbuf[7]=gs->teams[a]->startPos.y;
			*(float*)&outbuf[11]=gs->teams[a]->startPos.z;
			serverNet->SendData(outbuf,15);
		}
	}
	outbuf[0]=NETMSG_STARTPLAYING;
	serverNet->SendData(outbuf,1);
	timeLeft=0;
	ReleaseMutex(gameServerMutex);
}

void CGameServer::CheckForGameEnd(void)
{
	if(gameEndDetected){
		if(gameEndTime>2 && gameEndTime<500){
			outbuf[0]=NETMSG_GAMEOVER;
			serverNet->SendData(outbuf,1);
			gameEndTime=600;
		}
		return;
	}

	bool active[MAX_TEAMS];

	for(int a=0;a<MAX_TEAMS;++a)
		active[a]=false;

	for(int a=0;a<gs->activeTeams;++a)
		if(!gs->teams[a]->isDead)
			active[gs->team2allyteam[a]]=true;

	int numActive=0;

	for(int a=0;a<MAX_TEAMS;++a)
		if(active[a])
			numActive++;

	if(numActive<=1){
		gameEndDetected=true;
		gameEndTime=0;
		outbuf[0]=NETMSG_SENDPLAYERSTAT;
		serverNet->SendData(outbuf,1);
	}
}

void CGameServer::CreateNewFrame(bool fromServerThread)
{
	if(!fromServerThread)
		WaitForSingleObject(gameServerMutex,INFINITE);
	serverframenum++;
	outbuf[0]=NETMSG_NEWFRAME;
	(*((int*)&outbuf[1]))=serverframenum;
	if(serverNet->SendData(outbuf,5)==-1){
		info->AddLine("Server net couldnt send new frame");
		globalQuit=true;
	}
	if(!fromServerThread)
		ReleaseMutex(gameServerMutex);
}

void CGameServer::UpdateLoop(void)
{
	Sleep(500);		//we might crash if game hasnt finished initializing within this time
	SetThreadPriority(thisThread,THREAD_PRIORITY_ABOVE_NORMAL);		//we want the server to continue running smoothly even if the game client is struggling
	while(!quitServer){
		WaitForSingleObject(gameServerMutex,INFINITE);
		if(!Update()){
			info->AddLine("Game server experienced an error in update");
			globalQuit=true;
		}
		ReleaseMutex(gameServerMutex);
		Sleep(10);
	}
	delete this;
}

void CGameServer::SendSystemMsg(const char* fmt,...)
{
	char text[500];
	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing

	va_start(ap, fmt);									// Parses The String For Variables
	    vsprintf(text, fmt, ap);						// And Converts Symbols To Actual Numbers
	va_end(ap);											// Results Are Stored In Text

	string msg=text;

	outbuf[0]=NETMSG_SYSTEMMSG;
	outbuf[1]=msg.size()+4;
	outbuf[2]=gu->myPlayerNum;
	for(unsigned int a=0;a<msg.size();a++)
		outbuf[a+3]=msg.at(a);
	outbuf[msg.size()+3]=0;
	serverNet->SendData(outbuf,outbuf[1]);
}
