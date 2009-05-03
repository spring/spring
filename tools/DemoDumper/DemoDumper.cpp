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
#include "demofile.h"
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
#include <boost/cerrno.hpp>
#include <boost/filesystem/fstream.hpp>
namespace fs = boost::filesystem;


#if defined(WIN32) && (defined(UNICODE) || defined(_UNICODE))

	typedef fs::wpath Path;

	typedef wchar_t Char;
	typedef std::wstring String;
	typedef std::wstringstream StringStream;
	std::wostream& cout = std::wcout;

	// Untested with MSVC (works fine with GCC).
	#define _(s) L ## s

#else

	// fs::wpath segfaults on Linux when doing something simple like:
	//		Path p = fs::current_path<Path>();
	// (this is used by fs::system_complete() and others ...)
	typedef fs::path Path;

	// And converting stuff back and forth between wide chars and narrow chars
	// is a PITA so we just define String to normal string too.
	// (Accented chars are for nubs anyway, in particular in filenames ;-))
	typedef char Char;
	typedef std::string String;
	typedef std::stringstream StringStream;
	std::ostream& cout = std::cout;

	#define _(s) s

#endif


/******************************************************************************/

struct ScopedMessage : public StringStream {
	~ScopedMessage(){
#ifdef DEMO_DUMPER_USE_WIN_API
		MessageBoxW(NULL, this->str().c_str(), _("Spring - DemoDumper"), MB_APPLMODAL);
#else
		cout<<this->str()<<endl;
#endif
	}
};


struct DemofileException : public std::exception {
	Path filename;
	String message;
	DemofileException(const Path& fname, const String& msg) :
		filename(fname), message(msg) {}
	~DemofileException() throw() {}
};


static int run(int argc, const Char* const* argv);
static void read_demofile(ScopedMessage& message, const Path& source_file);


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
#elif defined(WIN32) && (defined(UNICODE) || defined(_UNICODE))
int wmain(int argc, const wchar_t* const* argv)
{
	return run(argc, argv);
}
#else
int main(int argc, const char* const* argv)
{
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
	return run(argc, argv);
}
#endif


static int run(int argc, const Char* const* argv)
{
	ScopedMessage message;
	try{
		if(argc == 0){
			message<<_("Error: Zero arguments.")<<endl;
			return 1;
		}else if(argc != 2){
			message<<_("Usage: ")<<fs::system_complete(argv[0]).leaf()<<_(" <filename>")<<endl<<endl;
			return 1;
		}

		const Path source_file = fs::system_complete(argv[1]);

		if(!fs::exists(source_file)){
			message<<source_file<<endl<<_("File not found.")<<endl;
			return 1;
		}

		read_demofile(message, source_file);

	}
	catch (DemofileException& error) {
		message<<_("Cannot read demofile: ")<<error.filename;
		message<<_(": ")<<error.message<<endl;
	}
	catch(fs::filesystem_error& error){
		message<<_("Filesystem error in: ")<<error.what()<<endl;
	}
	catch(std::exception& error){
		message<<_("Found an exception with: ")<<error.what()<<endl;
	}
	catch(...){
		message<<_("Found an unknown exception.")<<endl;
	}

	return 0;
}


struct PlayerStatistics
{
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


StringStream& operator<<(StringStream& str, const DemoFileHeader& header)
{
	char idbuf[128];
	const unsigned char* p = header.gameID;

	sprintf(idbuf,
		"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		p[ 0], p[ 1], p[ 2], p[ 3], p[ 4], p[ 5], p[ 6], p[ 7],
		p[ 8], p[ 9], p[10], p[11], p[12], p[13], p[14], p[15]);

	str<<_("Magic: ")<<header.magic<<endl;
	str<<_("Version: ")<<header.version<<endl;
	str<<_("HeaderSize: ")<<header.headerSize<<endl;
	str<<_("VersionString: ")<<header.versionString<<endl;
	str<<_("GameID: ")<<idbuf<<endl;
	str<<_("UnixTime: ")<<header.unixTime<<endl;
	str<<_("ScriptSize: ")<<header.scriptSize<<endl;
	str<<_("DemoStreamSize: ")<<header.demoStreamSize<<endl;
	str<<_("GameTime: ")<<header.gameTime<<endl;
	str<<_("WallclockTime: ")<<header.wallclockTime<<endl;
	str<<_("NumPlayers: ")<<header.numPlayers<<endl;
	str<<_("PlayerStatSize: ")<<header.playerStatSize<<endl;
	str<<_("PlayerStatElemSize: ")<<header.playerStatElemSize<<endl;
	str<<_("NumTeams: ")<<header.numTeams<<endl;
	str<<_("TeamStatSize: ")<<header.teamStatSize<<endl;
	str<<_("TeamStatElemSize: ")<<header.teamStatElemSize<<endl;
	str<<_("TeamStatPeriod: ")<<header.teamStatPeriod<<endl;
	str<<_("WinningAllyTeam: ")<<header.winningAllyTeam<<endl;
	return str;
}


StringStream& operator<<(StringStream& str, const PlayerStatistics& header)
{
	str<<_("MousePixels: ")<<header.mousePixels<<endl;
	str<<_("MouseClicks: ")<<header.mouseClicks<<endl;
	str<<_("KeyPresses: ")<<header.keyPresses<<endl;
	str<<_("NumCommands: ")<<header.numCommands<<endl;
	str<<_("UnitCommands: ")<<header.unitCommands<<endl;
	return str;
}


StringStream& operator<<(StringStream& str, const TeamStatistics& header)
{
	str<<_("MetalUsed: ")<<header.metalUsed<<endl;
	str<<_("EnergyUsed: ")<<header.energyUsed<<endl;
	str<<_("MetalProduced: ")<<header.metalProduced<<endl;
	str<<_("EnergyProduced: ")<<header.energyProduced<<endl;
	str<<_("MetalExcess: ")<<header.metalExcess<<endl;
	str<<_("EnergyExcess: ")<<header.energyExcess<<endl;
	str<<_("MetalReceived: ")<<header.metalReceived<<endl;
	str<<_("EnergyReceived: ")<<header.energyReceived<<endl;
	str<<_("MetalSent: ")<<header.metalSent<<endl;
	str<<_("EnergySent: ")<<header.energySent<<endl;
	str<<_("DamageDealt: ")<<header.damageDealt<<endl;
	str<<_("DamageReceived: ")<<header.damageReceived<<endl;
	str<<_("UnitsProduced: ")<<header.unitsProduced<<endl;
	str<<_("UnitsDied: ")<<header.unitsDied<<endl;
	str<<_("UnitsReceived: ")<<header.unitsReceived<<endl;
	str<<_("UnitsSent: ")<<header.unitsSent<<endl;
	str<<_("UnitsCaptured: ")<<header.unitsCaptured<<endl;
	str<<_("UnitsOutCaptured: ")<<header.unitsOutCaptured<<endl;
	str<<_("UnitsKilled: ")<<header.unitsKilled<<endl;
	return str;
}


static void read_demofile(ScopedMessage& message, const Path& source_file)
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
		throw DemofileException(source_file, _("Corrupt demofile."));
	}

	// Write out the DemoFileHeader.
	message<<fileHeader<<endl;

	// Seek to the player statistics (by skipping over earlier chunks)
	file.seekg(fileHeader.headerSize + fileHeader.scriptSize + fileHeader.demoStreamSize);

	// Loop through all players and read and output the statistics for each.
	for (int playerNum = 0; playerNum < fileHeader.numPlayers; ++playerNum) {
		file.read((char*)&playerStats, sizeof(playerStats));
		message<<_("-- Player statistics for player ")<<playerNum<<_(" --")<<endl;
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
			message<<_("-- Team statistics for team ")<<teamNum<<_(", game second ")<<time<<_(" --")<<endl;
			message<<teamStats<<endl;
			time += fileHeader.teamStatPeriod;
		}
	}

	// Clean up.
	delete[] numStatsPerTeam;
}
