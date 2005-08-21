#include "StdAfx.h"
#include "KrogothScript.h"
#include "UnitLoader.h"
//#include "mmgr.h"

static CKrogothScript ks;

CKrogothScript::CKrogothScript(void)
: CScript(std::string("Small battle"))
{
}

CKrogothScript::~CKrogothScript(void)
{
}

void CKrogothScript::Update(void)
{
	switch(gs->frameNum){
	case 0:
		unitLoader.LoadUnit("ARMFLASH",float3(2400,100,2500),0,false);
		unitLoader.LoadUnit("ARMFLASH",float3(2464,100,2500),0,false);
		unitLoader.LoadUnit("ARMFLASH",float3(2528,100,2500),0,false);
		unitLoader.LoadUnit("ARMFLASH",float3(2592,100,2500),0,false);

		unitLoader.LoadUnit("CORKROG",float3(3965,100,2550),1,false);
		unitLoader.LoadUnit("CORCOM",float3(3065,100,2900),0,false);

/*		unitLoader.LoadUnit("CORGOL",float3(3565,100,2900),1,false);
		unitLoader.LoadUnit("CORSENT",float3(3465,100,3150),1,false);
		unitLoader.LoadUnit("CORSENT",float3(3565,100,3150),1,false);
*/
		for(int a=0;a<6;++a){
			unitLoader.LoadUnit("ARMFLASH",float3(3265+a*64,100,1500),0,false);
			unitLoader.LoadUnit("ARMPW",float3(3265+a*64,100,1600),0,false);
			unitLoader.LoadUnit("ARMJETH",float3(3265+a*64,100,1700),0,false);
			unitLoader.LoadUnit("ARMLATNK",float3(3265+a*64,100,1800),0,false);
			unitLoader.LoadUnit("ARMHAM",float3(3265+a*64,100,1900),0,false);
			unitLoader.LoadUnit("ARMHAWK",float3(3265+a*64,100,1400),0,false);
		}
		unitLoader.LoadUnit("ARMVADER",float3(3425,100,1750),0,false);
		for(int a=0;a<10;++a){
			unitLoader.LoadUnit("ARMBRAWL",float3(4765,100,6800+a*64),1,false);
			unitLoader.LoadUnit("ARMHAWK",float3(4825,100,6800+a*64),1,false);
		}
		unitLoader.LoadUnit("ARMFLASH",float3(5600,100,6800),1,false);
		break;
	}
}

std::string CKrogothScript::GetMapName(void)
{
	return "map4.smf";
}
