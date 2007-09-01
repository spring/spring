#include "StdAfx.h"
#include "Demo.h"

#include <assert.h>
#include <fstream>
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
#include "demofile.h"

#ifdef __GNUC__
#define __time64_t time_t
#define _time64(x) time(x)
#define _localtime64(x) localtime(x)
#endif


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

	std::string filename = filesystem.LocateFile(demoName, FileSystem::WRITE);
	recordDemo = SAFE_NEW std::ofstream(filename.c_str(), std::ios::out | std::ios::binary);

	memset(&fileHeader, 0, sizeof(DemoFileHeader));
	strcpy(fileHeader.magic, DEMOFILE_MAGIC);
	fileHeader.version = DEMOFILE_VERSION;
	fileHeader.headerSize = sizeof(DemoFileHeader);
	strcpy(fileHeader.versionStr, VERSION_STRING);

	__time64_t currtime;
	_time64(&currtime);
	fileHeader.unixTime = currtime;

	if (gameSetup) {
		// strip trailing null termination characters
		int length = gameSetup->gameSetupTextLength;
		while (gameSetup->gameSetupText[length - 1] == '\0')
			--length;

		fileHeader.scriptPtr = fileHeader.headerSize;
		fileHeader.scriptSize = length;
		fileHeader.demoStreamPtr = fileHeader.scriptPtr + length;

		recordDemo->seekp(fileHeader.scriptPtr);
		recordDemo->write(gameSetup->gameSetupText, length);
	} else {
		fileHeader.demoStreamPtr = fileHeader.headerSize;
	}

	fileHeader.winningAllyTeam = -1;

	WriteFileHeader();
	recordDemo->seekp(fileHeader.demoStreamPtr);
}

CDemoRecorder::~CDemoRecorder()
{
	fileHeader.demoStreamSize = (int)recordDemo->tellp() - fileHeader.demoStreamPtr;

	WritePlayerStats();
	WriteTeamStats();
	WriteFileHeader();

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
	DemoStreamChunkHeader chunkHeader;

	chunkHeader.modGameTime = gu->modGameTime;
	chunkHeader.length = length;
	chunkHeader.swab();
	recordDemo->write((char*)&chunkHeader, sizeof(chunkHeader));
	recordDemo->write((char*)buf, length);
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

void CDemoRecorder::SetGameID(const unsigned char* buf)
{
	memcpy(&fileHeader.gameID, buf, sizeof(fileHeader.gameID));
	WriteFileHeader();
}

void CDemoRecorder::SetTime(int gameTime, int wallclockTime)
{
	fileHeader.gameTime = gameTime;
	fileHeader.wallclockTime = wallclockTime;
}

void CDemoRecorder::InitializeStats(int numPlayers, int numTeams, int winningAllyTeam)
{
	fileHeader.numberOfPlayers = numPlayers;
	fileHeader.numberOfTeams = numTeams;
	fileHeader.teamStatPeriod = CTeam::statsPeriod;
	fileHeader.winningAllyTeam = winningAllyTeam;

	playerStats.resize(numPlayers);
	teamStats.resize(numTeams);
}

/** @brief Set (overwrite) the CPlayer::Statistics for player playerNum */
void CDemoRecorder::SetPlayerStats(int playerNum, const CPlayer::Statistics& stats)
{
	assert((unsigned)playerNum < playerStats.size());

	playerStats[playerNum] = stats;
}

/** @brief Set (overwrite) the CTeam::Statistics history for team teamNum */
void CDemoRecorder::SetTeamStats(int teamNum, const std::list< CTeam::Statistics >& stats)
{
	assert((unsigned)teamNum < teamStats.size());

	teamStats[teamNum].clear();
	teamStats[teamNum].reserve(stats.size());

	for (std::list< CTeam::Statistics >::const_iterator it = stats.begin(); it != stats.end(); ++it)
		teamStats[teamNum].push_back(*it);
}

/** @brief Write DemoFileHeader
Write the DemoFileHeader at the start of the file and restores the original
position in the file afterwards. */
void CDemoRecorder::WriteFileHeader()
{
	std::ofstream::pos_type pos = recordDemo->tellp();

	recordDemo->seekp(0);

	fileHeader.swab(); // to little endian
	recordDemo->write((char*)&fileHeader, sizeof(fileHeader));
	fileHeader.swab(); // back to host endian

	recordDemo->seekp(pos);
}

/** @brief Write the CPlayer::Statistics at the current position in the file. */
void CDemoRecorder::WritePlayerStats()
{
	if (fileHeader.numberOfPlayers == 0)
		return;

	fileHeader.playerStatPtr = recordDemo->tellp();

	for (std::vector< CPlayer::Statistics >::iterator it = playerStats.begin(); it != playerStats.end(); ++it) {
		CPlayer::Statistics& stats = *it;
		stats.swab();
		recordDemo->write((char*)&stats, sizeof(CPlayer::Statistics));
	}
	playerStats.clear();

	fileHeader.playerStatSize = (int)recordDemo->tellp() - fileHeader.playerStatPtr;

	int wanted = sizeof(CPlayer::Statistics) * fileHeader.numberOfPlayers;
	if (fileHeader.playerStatSize != wanted)
		logOutput.Print("Invalid playerStatSize in CDemoRecorder::WritePlayerStats (%d instead of %d)", fileHeader.playerStatSize, wanted);
}

/** @brief Write the CTeam::Statistics at the current position in the file. */
void CDemoRecorder::WriteTeamStats()
{
	if (fileHeader.numberOfTeams == 0)
		return;

	fileHeader.teamStatPtr = recordDemo->tellp();

	for (std::vector< std::vector< CTeam::Statistics > >::iterator it = teamStats.begin(); it != teamStats.end(); ++it) {
		for (std::vector< CTeam::Statistics >::iterator it2 = it->begin(); it2 != it->end(); ++it2) {
			CTeam::Statistics& stats = *it2;
			stats.swab();
			recordDemo->write((char*)&stats, sizeof(CTeam::Statistics));
		}
	}
	teamStats.clear();

	fileHeader.teamStatSize = (int)recordDemo->tellp() - fileHeader.teamStatPtr;

	int wanted = sizeof(CTeam::Statistics) *
		(1 + fileHeader.numberOfTeams * (fileHeader.gameTime / fileHeader.teamStatPeriod));
	if (fileHeader.teamStatSize != wanted)
		logOutput.Print("Invalid teamStatSize in CDemoRecorder::WriteTeamStats (%d instead of %d)", fileHeader.teamStatSize, wanted);
}

/////////////////////////////////////
// CDemoReader implementation

CDemoReader::CDemoReader(const std::string& filename)
{
	std::string firstTry = "demos/" + filename;

	playbackDemo = SAFE_NEW CFileHandler(firstTry);

	if (!playbackDemo->FileExists()) {
		delete playbackDemo;
		playbackDemo = SAFE_NEW CFileHandler(filename);
	}

	if (!playbackDemo->FileExists()) {
		// file not found -> exception
		logOutput.Print("Demo file not found: %s",filename.c_str());
		delete playbackDemo;
		playbackDemo = NULL;
		return;
	}

	DemoFileHeader fileHeader;

	playbackDemo->Read((void*)&fileHeader, sizeof(fileHeader));
	fileHeader.swab();

	if (memcmp(fileHeader.magic, DEMOFILE_MAGIC, sizeof(fileHeader.magic)) ||
			fileHeader.version != DEMOFILE_VERSION ||
			fileHeader.headerSize != sizeof(fileHeader) ||
			strcmp(fileHeader.versionStr, VERSION_STRING)) {
		logOutput.Print("Demofile corrupt or created by a different version of Spring: %s", filename.c_str());
		delete playbackDemo;
		playbackDemo = NULL;
		return;
	}

	logOutput.Print("Playing demo from %s",filename.c_str());
	if (fileHeader.scriptPtr != 0) {
		char* buf = SAFE_NEW char[fileHeader.scriptSize];
		playbackDemo->Seek(fileHeader.scriptPtr);
		playbackDemo->Read(buf, fileHeader.scriptSize);

		if (!gameSetup) { // dont overwrite existing gamesetup (when hosting a demo)
			gameSetup = SAFE_NEW CGameSetup();
			gameSetup->Init(buf, fileHeader.scriptSize);
		}
		delete[] buf;

	} else {

		logOutput.Print("Demo file does not contain fileHeader data");
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

	gu->spectating           = true;
	gu->spectatingFullView   = true;
	gu->spectatingFullSelect = true;

	playbackDemo->Seek(fileHeader.demoStreamPtr);
	playbackDemo->Read((void*)&chunkHeader, sizeof(chunkHeader));
	chunkHeader.swab();

	demoTimeOffset = gu->modGameTime - chunkHeader.modGameTime;
	nextDemoRead = gu->modGameTime - 0.01f;
	bytesRemaining = fileHeader.demoStreamSize - sizeof(chunkHeader);
}

unsigned CDemoReader::GetData(unsigned char *buf, const unsigned length)
{
	if(gs->paused)
		return 0;

	if (bytesRemaining <= 0 || playbackDemo->Eof())
		return 0;

	unsigned ret = 0;
	while (nextDemoRead < gu->modGameTime) {
		if (ret + chunkHeader.length >= length) {
			logOutput.Print("Overflow in DemoReader");
			break;
		}

		playbackDemo->Read((void*)(buf + ret), chunkHeader.length);
		ret += chunkHeader.length;
		bytesRemaining -= chunkHeader.length;

		playbackDemo->Read((void*)&chunkHeader, sizeof(chunkHeader));
		chunkHeader.swab();
		nextDemoRead = chunkHeader.modGameTime + demoTimeOffset;
		bytesRemaining -= sizeof(chunkHeader);

		if (bytesRemaining <= 0 || playbackDemo->Eof()) {
			logOutput.Print("End of demo");
			break;
		}
	}
	return ret;
}
