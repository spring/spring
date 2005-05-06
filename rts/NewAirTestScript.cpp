#include "StdAfx.h"
#include "NewAirTestScript.h"
#include "UnitLoader.h"
//#include "mmgr.h"

static CNewAirTestScript ats;

CNewAirTestScript::CNewAirTestScript(void)
: CScript(std::string("NewAirTest"))
{
}

CNewAirTestScript::~CNewAirTestScript(void)
{
}

void CNewAirTestScript::Update(void)
{
	switch(gs->frameNum){
	case 0:
		unitLoader.LoadUnit("ARMCOM",float3(3000,80,3000),0,false);
		unitLoader.LoadUnit("ARMCA",float3(3100,80,3000),0,false);
		unitLoader.LoadUnit("ARMBRAWL",float3(3200,80,3000),0,false);
		unitLoader.LoadUnit("ARMATLAS",float3(3300,80,3000),0,false);
//		unitLoader.LoadUnit("ARMSUB",float3(1265,100,4500),1,false);
//		unitLoader.LoadUnit("ARMPT",float3(1365,100,4400),1,false);
		unitLoader.LoadUnit("CORCOM",float3(1365,80,2720),1,false);

//		unitLoader.LoadUnit("armsilo",float3(2650,10,2600),1,false);
//		unitLoader.LoadUnit("armscab",float3(2250,10,4800),2,false);

		break;
	}
}
