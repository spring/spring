// Universal build.h
#include "AICallback.h"

class Tunit;

class CUBuild{
public:
	CUBuild();
	virtual ~CUBuild();

	void Init(Global* GL, const UnitDef* u, int unit); // Initialize object
	bool SetWater();
	string operator() (btype build); // () operator, allows it to be called like this, Object(build); rather than something like object->getbuild(build);

	string GetMEX(); // metal extractor
	string GetPOWER(); // energy producer
	string GetRAND_ASSAULT(); // random attack unit
	string GetASSAULT(); // most efficient attack unit (give or take a percentage)
	string GetFACTORY_CHEAP(); // factory (cost based selection)
	string GetFACTORY(); // factory
	string GetBUILDER(); // builders
	string GetGEO(); // geothermal plant
	string GetSCOUT(); // scouter
	string GetRANDOM(); // random unit (no mines)
	string GetDEFENCE(); // random defense
	string GetRADAR(); // radar towers
	string GetESTORE(); // energy storage
	string GetMSTORE(); // metal storage
	string GetSILO(); // missile silos
	string GetJAMMER(); // radar jammers
	string GetSONAR(); // sonar
	string GetANTIMISSILE(); // antimissile units
	string GetARTILLERY(); // artillery units
	string GetFOCAL_MINE(); // focal mines
	string GetSUB(); // submarines
	string GetAMPHIB(); // amphibious units (pelican/gimp/triton etc
	string GetMINE(); // mines
	string GetCARRIER(); // units with air repair pads that floats
	string GetMETAL_MAKER(); // metal makers
	string GetFORTIFICATION(); // walls etc
	string GetTARG(); // Targeting facility type buildings
	string GetBOMBER(); // Bomber planes
	string GetSHIELD(); // units with shields
	string GetMISSILE_UNIT(); // units that launch missiles, such as the Merl or diplomat or dominator
	string GetFIGHTER();
	string GetGUNSHIP();

	bool Useless(string name); // To filter out things like dragons eye in AA, things that serve no purpose other than to simply exist (things made by bananas!)
	bool antistall;
private:
	Global* G;
	const UnitDef* ud;
	int uid;
	bool water;
};;
