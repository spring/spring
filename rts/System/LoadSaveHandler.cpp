#include "StdAfx.h"
#include "LoadSaveHandler.h"
#include "Rendering/GL/myGL.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/FeatureHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Platform/errorhandler.h"
#include "Platform/FileSystem.h"
#include "creg/Serializer.h"
#include <fstream>
#include "mmgr.h"

extern std::string stupidGlobalMapname;

CLoadSaveHandler::CLoadSaveHandler(void)
{}

CLoadSaveHandler::~CLoadSaveHandler(void)
{}

class CGameStateCollector
{
public:
	CR_DECLARE(CGameStateCollector);

	CGameStateCollector() {}

	void Serialize(creg::ISerializer& s);

	std::string mapName, modName;
};

CR_BIND(CGameStateCollector, );
CR_REG_METADATA(CGameStateCollector, CR_SERIALIZER(Serialize));

static void SerializeString(creg::ISerializer& s, std::string& str)
{
	unsigned short size = (unsigned short)str.length();
	s.Serialize(&size,2);

	if (!s.IsWriting()) 
		str.resize(size);

	s.Serialize(&str[0], size);
}

static void WriteString(std::ostream& s, std::string& str)
{
	for (int a=0;a<str.length();a++) 
		s << str[a];
	s << (char)0;
}
 
static void ReadString(std::istream& s, std::string& str)
{
	char c;
	do {
		s >> c;
		str += c;
	} while (c != 0);
}

void CGameStateCollector::Serialize(creg::ISerializer& s)
{
	// GetClass() works because readmap and uh both have to exist already
	s.SerializeObjectInstance(readmap, readmap->GetClass());
	s.SerializeObjectInstance(qf, qf->GetClass());
	// s.SerializeObjectInstance(fh,
	s.SerializeObjectInstance(loshandler, loshandler->GetClass());
	s.SerializeObjectInstance(radarhandler, radarhandler->GetClass());
	s.SerializeObjectInstance(airBaseHandler, airBaseHandler->GetClass());
	s.SerializeObjectInstance(&interceptHandler, interceptHandler.GetClass());
	s.SerializeObjectInstance(categoryHandler, categoryHandler->GetClass());
	s.SerializeObjectInstance(uh, uh->GetClass());
}

void CLoadSaveHandler::SaveGame(std::string file)
{
	std::ofstream ofs(filesystem.LocateFile(file, FileSystem::WRITE).c_str(), std::ios::out|std::ios::binary);

	PrintLoadMsg("Saving game");
	if (ofs.bad() || !ofs.is_open()) {
		handleerror(0,"Couldnt save game to file",file.c_str(),0);
		return;
	}

	WriteString(ofs, modName);
	WriteString(ofs, mapName);

	CGameStateCollector *gsc = new CGameStateCollector();

	creg::COutputStreamSerializer os;
	os.SavePackage(&ofs, gsc, gsc->GetClass()); 
}

//this just loads the mapname and some other early stuff
void CLoadSaveHandler::LoadGameStartInfo(std::string file)
{
	ifs = new std::ifstream (filesystem.LocateFile(file).c_str(), std::ios::in|std::ios::binary);

	ReadString(*ifs, modName);
	ReadString(*ifs, mapName);
}

//this should be called on frame 0 when the game has started
void CLoadSaveHandler::LoadGame()
{
	creg::CInputStreamSerializer inputStream;
	void *pGSC = 0;
	creg::Class* gsccls = 0;
	inputStream.LoadPackage(ifs, pGSC, gsccls);

	assert (pGSC && gsccls == CGameStateCollector::StaticClass());

	CGameStateCollector *gsc = (CGameStateCollector *)pGSC;
	delete gsc; // only job of gsc is to collect gamestate data
	delete ifs;
}
