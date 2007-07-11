#include "Demo.h"

#include <assert.h>
#include <time.h>
#ifdef _WIN32
#include <io.h> // for _mktemp
#endif

#include "Platform/FileSystem.h"
#include "Sync/Syncify.h"
#include "LogOutput.h"
#include "Game/GameSetup.h"
#include "Game/GameVersion.h"

#include "Game/Team.h"
#include "Lua/LuaGaia.h" // FIXME: this is even worse
#include "Lua/LuaRules.h"
#include "Platform/ConfigHandler.h"

#ifdef __GNUC__
#define __time64_t time_t
#define _time64(x) time(x)
#define _localtime64(x) localtime(x)
#endif


static std::string MakeDemoStartScript(const char *startScript, const int ssLen)
{
	std::string script;
	// find the last non-zero character
	int last = ssLen-1;
	while (last >= 0)  {
		if (startScript[last] != 0) break;
		last --;
	}
	time_t currtime;
	time(&currtime);
	tm* gmttime = gmtime(&currtime);
	char buff[500];
	sprintf(buff, "%d-%d-%d %2d:%2d:%2d GMT", gmttime->tm_year+1900, gmttime->tm_mon, gmttime->tm_mday,
			gmttime->tm_hour, gmttime->tm_min, gmttime->tm_sec);
	std::string datetime;
	script.insert (script.begin(), startScript, startScript + last + 1);
	script += "\n[VERSION]\n{\n\tGameVersion=" VERSION_STRING ";\n";
	script += "\tDateTime=" + std::string(buff) + ";\n";
	sprintf(buff, "%u", (unsigned)currtime);
	script += "\tUnixTime=" + std::string(buff) + ";\n}\n";
	return script;
}

CDemoRecorder::CDemoRecorder()
{
	// We want this folder to exist
	if (!filesystem.CreateDirectory("demos"))
		return;
	
	char buf[500] = "demos/XXXXXX";
#ifndef _WIN32
	mkstemp(buf);
#else
	_mktemp(buf);
#endif

	
	demoName = wantedName = buf;
	recordDemo=SAFE_NEW std::ofstream(filesystem.LocateFile(demoName, FileSystem::WRITE).c_str(), std::ios::out|std::ios::binary);

	if (gameSetup)
	{
		// add a TDF section containing the game version to the startup script
		std::string scriptText = MakeDemoStartScript (gameSetup->gameSetupText, gameSetup->gameSetupTextLength);
	
		char c=1;
		recordDemo->write(&c,1);
		int len = scriptText.length();
		recordDemo->write((char*)&len, sizeof(int));
		recordDemo->write(scriptText.c_str(), scriptText.length());
	}
	else
	{
		char c=0;
		recordDemo->write(&c,1);
	}
}

CDemoRecorder::~CDemoRecorder()
{
	delete recordDemo;
	
	if (demoName != wantedName)
	{
		rename(demoName.c_str(), wantedName.c_str());
	}
	else
	{
		remove("demos/unnamed.sdf");
		rename(demoName.c_str(), "demos/unnamed.sdf");
	}
}

void CDemoRecorder::SaveToDemo(const unsigned char* buf,const unsigned length)
{
	recordDemo->write((char*)&gu->modGameTime,sizeof(float));
	recordDemo->write((char*)&length,sizeof(unsigned));
	recordDemo->write((char*)buf,length);
	recordDemo->flush();
}

void CDemoRecorder::SetName(const std::string& mapname)
{
	struct tm *newtime;
	__time64_t long_time;
	_time64(&long_time);                /* Get time as long integer. */
	newtime = _localtime64(&long_time); /* Convert to local time. */

	char buf[500];
	sprintf(buf,"%02i%02i%02i",newtime->tm_year%100,newtime->tm_mon+1,newtime->tm_mday);
	std::string name=std::string(buf)+"-"+mapname.substr(0,mapname.find_first_of("."));
	name+=std::string("-")+VERSION_STRING;
	
	sprintf(buf,"demos/%s.sdf",name.c_str());
	CFileHandler ifs(buf);
	if(ifs.FileExists()){
		for(int a=0;a<9999;++a){
			sprintf(buf,"demos/%s-%i.sdf",name.c_str(),a);
			CFileHandler ifs(buf);
			if(!ifs.FileExists())
				break;
		}
	}
	wantedName = buf;
}

CDemoReader::CDemoReader(const std::string& filename)
{
	std::string firstTry = filename;
	firstTry = "demos/" + firstTry;
	playbackDemo=SAFE_NEW CFileHandler(firstTry);
	if (!playbackDemo->FileExists()) {
		delete playbackDemo;
		playbackDemo=SAFE_NEW CFileHandler(filename);
	}

	if(playbackDemo->FileExists()){
		logOutput.Print("Playing demo from %s",filename.c_str());
		char c;
		playbackDemo->Read(&c,1);
		if(c){
			int length;
			playbackDemo->Read(&length,sizeof(int));
			char* buf=SAFE_NEW char[length];
			playbackDemo->Read(buf,length);

			if (!gameSetup)	// dont overwrite existing gamesetup (when hosting a demo)
			{
				gameSetup=SAFE_NEW CGameSetup();
				gameSetup->Init(buf,length);
			}
			delete[] buf;
		}
		else {
			logOutput.Print("Demo file does not contain header data");
			// Didn't get a CGameSetup script
			// FIXME: duplicated in Main.cpp
			const string luaGaiaStr  = configHandler.GetString("LuaGaia",  "1");
			const string luaRulesStr = configHandler.GetString("LuaRules", "1");
			gs->useLuaGaia  = CLuaGaia::SetConfigString(luaGaiaStr);
			gs->useLuaRules = CLuaRules::SetConfigString(luaRulesStr);
			if (gs->useLuaGaia) {
				gs->gaiaTeamID = gs->activeTeams;
				gs->gaiaAllyTeamID = gs->activeAllyTeams;
				gs->activeTeams++;
				gs->activeAllyTeams++;
				CTeam* team = gs->Team(gs->gaiaTeamID);
				team->color[0] = 255;
				team->color[1] = 255;
				team->color[2] = 255;
				team->color[3] = 255;
				team->gaia = true;
				gs->SetAllyTeam(gs->gaiaTeamID, gs->gaiaAllyTeamID);
			}
		}
		
		gu->spectating = true;
		gu->spectatingFullView = true;
		
		playbackDemo->Read(&demoTimeOffset,sizeof(float));
		demoTimeOffset=gu->modGameTime-demoTimeOffset;
		nextDemoRead=gu->modGameTime-0.01f;
	} else {
		// file not found -> exception
		logOutput.Print("Demo file not found: %s",filename.c_str());
		delete playbackDemo;
		playbackDemo=0;
	}
}

unsigned CDemoReader::GetData(unsigned char *buf, const unsigned length)
{
	if(gs->paused)
		return 0;
	
	unsigned ret=0;
	while(nextDemoRead<gu->modGameTime){
		if(playbackDemo->Eof()){
			return 0;
		}
		unsigned l;
		playbackDemo->Read(&l,sizeof(int));
		playbackDemo->Read(reinterpret_cast<void*>(buf+ret),l);
		playbackDemo->Read(&nextDemoRead,sizeof(float));
		nextDemoRead+=demoTimeOffset;
		ret+=l;
		if (ret >= length) {
			logOutput.Print("Overflow in DemoReader");
			break;
		}
		if(playbackDemo->Eof()){
			logOutput.Print("End of demo");
			nextDemoRead=gu->modGameTime+4*gs->speedFactor;
		}
	}
	return ret;
}




