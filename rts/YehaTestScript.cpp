#include "StdAfx.h"
#include "YehaTestScript.h"
#include "TestScript.h"
#include "UnitLoader.h"
#include "Unit.h"
#include "Weapon.h"
#include "ReadMap.h"
#include "Ground.h"
#include "FeatureHandler.h"
#include "SunParser.h"
//#include "mmgr.h"

CYehaTestScript yehatest;

CYehaTestScript::CYehaTestScript(void)
:	CScript(std::string("ehatest"))
{
}

CYehaTestScript::~CYehaTestScript(void)
{
}


void CYehaTestScript::Update(void)
{
	switch(gs->frameNum){
	case 0:

	/*	for(int x=0;x<50;++x)
			for(int y=0;y<20;++y)
				unitLoader.LoadUnit("ARMTIDE",float3(1800+x*48,10,100+y*48),0,false);
*/
		unitLoader.LoadUnit("ARMFLASH",float3(1800,10,2910),0,false);
		unitLoader.LoadUnit("ARMSAM",float3(1870,10,2920),0,false);
		unitLoader.LoadUnit("ARMBULL",float3(1830,10,2910),0,false);
		unitLoader.LoadUnit("ARMVP",float3(1810,10,3100),0,false);
		unitLoader.LoadUnit("ARMAVP",float3(1980,10,3040),0,false);
		unitLoader.LoadUnit("ARMCOM",float3(2100,10,3020),0,false);
		unitLoader.LoadUnit("ARMGUARD",float3(1720,10,2850),0,false);
		unitLoader.LoadUnit("ARMMEX",float3(1520,10,2650),0,false);
		unitLoader.LoadUnit("ARMMERL",float3(1620,10,2650),0,false);
		unitLoader.LoadUnit("CORPYRO",float3(1720,10,2650),0,false);

	/*	for(int x=0;x<32;++x){
			for(int y=0;y<32;++y){
				unitLoader.LoadUnit("CORAK1",float3(2000+x*128,10,2000+y*128),1,false);
			}
		}*/
	}
}
