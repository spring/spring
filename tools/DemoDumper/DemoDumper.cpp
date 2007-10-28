// DemoDumper.cpp : Defines the entry point for the console application.
//

#include <cstdio>
#include <cctype>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <string>
#include <sstream>
#include <exception>
#include <algorithm>
#include "../../rts/System/demofile.h"
using std::endl;


//#define DEMO_DUMPER_USE_WIN_API

#ifdef DEMO_DUMPER_USE_WIN_API
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
#endif

#ifdef _MSC_VER
  #ifdef DEMO_DUMPER_USE_WIN_API
    #pragma comment(linker, "/SUBSYSTEM:WINDOWS")
  #else
    #pragma comment(linker, "/SUBSYSTEM:CONSOLE")
  #endif
#endif


#include <boost/shared_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/static_assert.hpp>
#include <boost/version.hpp>


BOOST_STATIC_ASSERT(BOOST_VERSION >= 103400);
#include <boost/filesystem.hpp>
#include <boost/filesystem/cerrno.hpp>
#include <boost/filesystem/fstream.hpp>
namespace fs = boost::filesystem;


/******************************************************************************/

struct scoped_message : public std::wstringstream {
	~scoped_message(){
#ifdef DEMO_DUMPER_USE_WIN_API
		MessageBoxW(NULL, this->str().c_str(), L"Spring - DemoDumper", MB_APPLMODAL);
#else
		std::wcout<<this->str()<<endl;
#endif
	}
};


struct demofile_exception : public std::exception {
	fs::wpath filename;
	std::wstring message;
	demofile_exception(const fs::wpath& fname, const std::wstring& msg) :
		filename(fname), message(msg) {}
};


static int run(int argc, wchar_t** argv);
static void read_demofile(scoped_message& message, const fs::wpath& source_file);


/******************************************************************************/

#if defined(DEMO_DUMPER_USE_WIN_API)
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	int argc = 0;
	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if(argv == 0){
		return 0;
	}

	int ret = run(argc, argv);
	LocalFree(argv);

	return ret;
}
#elif defined(_MSC_VER) && (defined(UNICODE) || defined(_UNICODE))
int wmain(int argc, wchar_t** argv)
{
	return run(argc, argv);
}
#else
int main(int argc, char** argv){
/*	//Untested
	mbstate_t mbstate;
	memset((void*)&mbstate, 0, sizeof(mbstate));

	wchar_t** argvW = new wchar_t*[argc];
	for(int i = 0; i < argc; i++){
		const char *indirect_string = argv[i];
		std::size_t num_chars = mbsrtowcs(NULL, &indirect_string, INT_MAX, &mbstate);
		if(num_chars == -1){
			return 1;
		}

		argvW[i] = new wchar_t[num_chars+1];
		num_chars = mbsrtowcs(argvW[i], &indirect_string, num_chars+1, &mbstate);
		if(num_chars == -1){
			return 1;
		}
	}

	int ret = run(argc, argvW);

	for(int i = 0; i < argc; i++){
		delete [] argvW[i];
	}
	delete [] argvW;

	return ret;
*/
}
#endif


static int run(int argc, wchar_t** argv)
{
	scoped_message message;
	try{
		if(argc == 0){
			message<<L"Error: Zero arguments."<<endl;
			return 1;
		}else if(argc != 2){
			message<<L"Usage: "<<fs::system_complete(argv[0]).leaf()<<L" <filename>"<<endl<<endl;
			return 1;
		}


		const fs::wpath source_file = fs::system_complete(argv[1]);
		fs::wpath target_dir = fs::system_complete(argv[0]).branch_path();

		if(!fs::exists(source_file)){
			message<<source_file<<endl<<L"File not found."<<endl;
			return 1;
		}

		read_demofile(message, source_file);

	}
	catch (demofile_exception& error) {
		message<<"Cannot read demofile: "<<error.filename<<endl;
		message<<L": "<<error.message<<endl;
	}
	catch(fs::filesystem_error& error){
		message<<L"Filesystem error in: "<<error.what()<<endl;
	}
	catch(std::exception& error){
		message<<L"Found an exception with: "<<error.what()<<endl;
	}
	catch(...){
		message<<L"Found an unknown exception."<<endl;
	}

	return 0;
}


struct PlayerStatistics {
	/// how many pixels the mouse has traversed in total
	int mousePixels;
	int mouseClicks;
	int keyPresses;

	int numCommands;
	/// total amount of units affected by commands
	/// (divide by numCommands for average units/command)
	int unitCommands;
};


struct TeamStatistics
{
	float metalUsed,     energyUsed;
	float metalProduced, energyProduced;
	float metalExcess,   energyExcess;
	float metalReceived, energyReceived; //received from allies
	float metalSent,     energySent;     //sent to allies

	float damageDealt,   damageReceived; // Damage taken and dealt to enemy units

	int unitsProduced;
	int unitsDied;
	int unitsReceived;
	int unitsSent;
	int unitsCaptured;				//units captured from enemy by us
	int unitsOutCaptured;			//units captured from us by enemy
	int unitsKilled;	//how many enemy units have been killed by this teams units
};


std::wstringstream& operator<<(std::wstringstream& str, const DemoFileHeader& header)
{
	char idbuf[128];
	const unsigned char* p = header.gameID;

	sprintf(idbuf,
		"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		p[ 0], p[ 1], p[ 2], p[ 3], p[ 4], p[ 5], p[ 6], p[ 7],
		p[ 8], p[ 9], p[10], p[11], p[12], p[13], p[14], p[15]);

	str<<L"Magic: "<<header.magic<<endl;
	str<<L"Version: "<<header.version<<endl;
	str<<L"HeaderSize: "<<header.headerSize<<endl;
	str<<L"VersionString: "<<header.versionString<<endl;
	str<<L"GameID: "<<idbuf<<endl;
	str<<L"UnixTime: "<<header.unixTime<<endl;
	str<<L"ScriptSize: "<<header.scriptSize<<endl;
	str<<L"DemoStreamSize: "<<header.demoStreamSize<<endl;
	str<<L"GameTime: "<<header.gameTime<<endl;
	str<<L"WallclockTime: "<<header.wallclockTime<<endl;
	str<<L"NumPlayers: "<<header.numPlayers<<endl;
	str<<L"PlayerStatSize: "<<header.playerStatSize<<endl;
	str<<L"PlayerStatElemSize: "<<header.playerStatElemSize<<endl;
	str<<L"NumTeams: "<<header.numTeams<<endl;
	str<<L"TeamStatSize: "<<header.teamStatSize<<endl;
	str<<L"TeamStatElemSize: "<<header.teamStatElemSize<<endl;
	str<<L"TeamStatPeriod: "<<header.teamStatPeriod<<endl;
	str<<L"WinningAllyTeam: "<<header.winningAllyTeam<<endl;
	return str;
}


std::wstringstream& operator<<(std::wstringstream& str, const PlayerStatistics& header)
{
	str<<L"MousePixels: "<<header.mousePixels<<endl;
	str<<L"MouseClicks: "<<header.mouseClicks<<endl;
	str<<L"KeyPresses: "<<header.keyPresses<<endl;
	str<<L"NumCommands: "<<header.numCommands<<endl;
	str<<L"UnitCommands: "<<header.unitCommands<<endl;
	return str;
}


std::wstringstream& operator<<(std::wstringstream& str, const TeamStatistics& header)
{
	str<<L"MetalUsed: "<<header.metalUsed<<endl;
	str<<L"EnergyUsed: "<<header.energyUsed<<endl;
	str<<L"MetalProduced: "<<header.metalProduced<<endl;
	str<<L"EnergyProduced: "<<header.energyProduced<<endl;
	str<<L"MetalExcess: "<<header.metalExcess<<endl;
	str<<L"EnergyExcess: "<<header.energyExcess<<endl;
	str<<L"MetalReceived: "<<header.metalReceived<<endl;
	str<<L"EnergyReceived: "<<header.energyReceived<<endl;
	str<<L"MetalSent: "<<header.metalSent<<endl;
	str<<L"EnergySent: "<<header.energySent<<endl;
	str<<L"DamageDealt: "<<header.damageDealt<<endl;
	str<<L"DamageReceived: "<<header.damageReceived<<endl;
	str<<L"UnitsProduced: "<<header.unitsProduced<<endl;
	str<<L"UnitsDied: "<<header.unitsDied<<endl;
	str<<L"UnitsReceived: "<<header.unitsReceived<<endl;
	str<<L"UnitsSent: "<<header.unitsSent<<endl;
	str<<L"UnitsCaptured: "<<header.unitsCaptured<<endl;
	str<<L"UnitsOutCaptured: "<<header.unitsOutCaptured<<endl;
	str<<L"UnitsKilled: "<<header.unitsKilled<<endl;
	return str;
}


static void read_demofile(scoped_message& message, const fs::wpath& source_file)
{
	// Open the file for binary reading.
	boost::filesystem::ifstream file(source_file, std::ios_base::binary);

	// Allocate some space on the stack for our header and stat items.
	DemoFileHeader fileHeader;
	PlayerStatistics playerStats;
	TeamStatistics teamStats;

	// Start by reading the DemoFileHeader.
	file.read((char*)&fileHeader, sizeof(fileHeader));

	// Check whether the DemoFileHeader contains the right magic,
	// is of the right version and the right size.
	if (memcmp(fileHeader.magic, DEMOFILE_MAGIC, sizeof(fileHeader.magic)) ||
			fileHeader.version != DEMOFILE_VERSION ||
			fileHeader.headerSize != sizeof(DemoFileHeader) ||
			fileHeader.playerStatElemSize != sizeof(PlayerStatistics) ||
			fileHeader.teamStatElemSize != sizeof(TeamStatistics)) {
		throw demofile_exception(source_file, L"Corrupt demofile.");
	}

	// Write out the DemoFileHeader.
	message<<fileHeader<<endl;

	// Seek to the player statistics (by skipping over earlier chunks)
	file.seekg(fileHeader.headerSize + fileHeader.scriptSize + fileHeader.demoStreamSize);

	// Loop through all players and read and output the statistics for each.
	for (int playerNum = 0; playerNum < fileHeader.numPlayers; ++playerNum) {
		file.read((char*)&playerStats, sizeof(playerStats));
		message<<L"-- Player statistics for player "<<playerNum<<L" --"<<endl;
		message<<playerStats<<endl;
	}

	// Team statistics follow player statistics.

	// Read the array containing the number of team stats for each team.
	int* numStatsPerTeam = new int[fileHeader.numTeams];
	file.read((char*)numStatsPerTeam, sizeof(int) * fileHeader.numTeams);

	// Loop through all team stats for each team and read and output them.
	// We keep track of the gametime while reading the stats for a team so we
	// can output it too.
	for (int teamNum = 0; teamNum < fileHeader.numTeams; ++teamNum) {
		int time = 0;
		for (int i = 0; i < numStatsPerTeam[teamNum]; ++i) {
			file.read((char*)&teamStats, sizeof(teamStats));
			message<<L"-- Team statistics for team "<<teamNum<<L", game second "<<time<<L" --"<<endl;
			message<<teamStats<<endl;
			time += fileHeader.teamStatPeriod;
		}
	}

	// Clean up.
	delete[] numStatsPerTeam;
}
