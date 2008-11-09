// TestScript.cpp: implementation of the CTestScript class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "mmgr.h"

#include "TestScript.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/Weapon.h"
#include "Map/ReadMap.h"
#include "Map/Ground.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/Team.h"
#include "Game/SelectedUnits.h"
#include "Sim/Units/UnitHandler.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CTestScript::CTestScript()
: CScript(std::string("Test script"))
{

}

CTestScript::~CTestScript()
{

}

void CTestScript::Update()
{
	switch(gs->frameNum){
	case 0:
		{
		/* NOTE: wasn't doing anything anyways
		for(int a=0;a<0000;++a){
			float3 pos((gs->randFloat()+gs->randFloat())*2000,0,(gs->randFloat()+gs->randFloat())*2000);
			pos.y=ground->GetHeight(pos.x,pos.z);
			int num=featureHandler->GetWreckParser().GetSectionList("").size();
			string feature=featureHandler->GetWreckParser().GetSectionList("")[(int)(gs->randFloat()*num)];
			featureHandler->CreateWreckage(pos,feature,0,0,1,-1,-1,false,"");
		}
		*/
 		for(int a=0;a<30;++a){
			unitLoader.LoadUnit("ARM_FLASH",float3(2000+a*80,10,2900),0,false,0,NULL);
			unitLoader.LoadUnit("ARM_ZEUS",float3(2000+a*80,10,2855),0,false,0,NULL);
			unitLoader.LoadUnit("ARM_MAVERICK",float3(2000+a*80,10,2800),0,false,0,NULL);
			unitLoader.LoadUnit("CORE_AK",float3(2000+a*80,10,4560),1,false,0,NULL);
			unitLoader.LoadUnit("CORE_THE_CAN",float3(2000+a*80,10,4595),1,false,0,NULL);
			unitLoader.LoadUnit("CORE_FREAKER",float3(2000+a*80,10,4640),1,false,0,NULL);
		}/**/

		unitLoader.LoadUnit("ARM_ROCKO",float3(1800,10,2950),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_ROCKO",float3(1825,10,2950),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_ROCKO",float3(1850,10,2950),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_ROCKO",float3(1875,10,2950),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_LIGHT_LASER_TOWER",float3(1900,10,2950),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_LUGER",float3(1800,10,2925),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_LUGER",float3(1825,10,2925),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_LUGER",float3(1850,10,2925),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_MERL",float3(1825,10,2910),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_CONSTRUCTION_VEHICLE",float3(1850,10,2905),0,false,0,NULL);

		unitLoader.LoadUnit("ARM_CONSTRUCTION_KBOT",float3(1850,10,2900),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_FLASH",float3(1900,10,2900),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_BULLDOG",float3(1950,10,2900),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_SAMSON",float3(1875,10,2925),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_JETHRO",float3(1850,10,2890),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_PHOENIX",float3(1850,10,2600),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_HAWK",float3(1850,10,2680),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_BRAWLER",float3(1950,10,2680),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_BIG_BERTHA",float3(1650,10,1550),0,false,0,NULL);
		unitLoader.LoadUnit("CORE_KROGOTH",float3(1950,10,2750),0,false,0,NULL);
//		unitLoader.LoadUnit("ARMTHOVR",float3(1750,10,2550),0,false,0,NULL);

		unitLoader.LoadUnit("ARM_METAL_EXTRACTOR",float3(2850,10,2300),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_MOHO_MINE",float3(2950,10,2380),0,false,0,NULL);

		unitLoader.LoadUnit("ARM_ADVANCED_RADAR_TOWER",float3(2950,10,3780),0,false,0,NULL);

		std::vector<int> su;
		for(std::list<CUnit*>::iterator ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();++ui){
			if((*ui)->team!=0)
				continue;
			selectedUnits.AddUnit(*ui);
			su.push_back((*ui)->id);
		}
		selectedUnits.NetSelect(su,0);
		Command c;
		c.id=CMD_MOVE;
		c.params.push_back(3000);
		c.params.push_back(100);
		c.params.push_back(5000);
		c.options=CONTROL_KEY;
		selectedUnits.NetOrder(c,0);
		//selectedUnits.GiveCommand(c,false);

		unitLoader.LoadUnit("CORE_THE_CAN",float3(1770,10,4540),1,false,0,NULL);
		unitLoader.LoadUnit("CORE_THE_CAN",float3(1815,10,4540),1,false,0,NULL);
		unitLoader.LoadUnit("CORE_AK",float3(1850,10,4540),1,false,0,NULL);
		unitLoader.LoadUnit("CORE_AK",float3(1895,10,4540),1,false,0,NULL);
		unitLoader.LoadUnit("ARM_ZEUS",float3(2000,10,4580),1,false,0,NULL);
		unitLoader.LoadUnit("CORE_THUD",float3(1750,10,4640),1,false,0,NULL);
		unitLoader.LoadUnit("CORE_RAIDER",float3(1800,10,4640),1,false,0,NULL);
		unitLoader.LoadUnit("CORE_GOLIATH",float3(1850,10,4600),1,false,0,NULL);
		unitLoader.LoadUnit("CORE_PYRO",float3(1850,10,4700),1,false,0,NULL);
		unitLoader.LoadUnit("CORE_MORTY",float3(1900,10,4700),1,false,0,NULL);
		unitLoader.LoadUnit("CORE_REAPER",float3(2050,10,4660),1,false,0,NULL);
		unitLoader.LoadUnit("CORE_REAPER",float3(1975,10,4580),1,false,0,NULL);

		unitLoader.LoadUnit("CORE_DOOMSDAY_MACHINE",float3(1875,10,4465),1,false,0,NULL);
		unitLoader.LoadUnit("CORE_SHADOW",float3(1850,10,4840),1,false,0,NULL);
		unitLoader.LoadUnit("CORE_VAMP",float3(1850,10,4860),1,false,0,NULL);
		unitLoader.LoadUnit("CORE_TOASTER",float3(1740,10,4480),1,false,0,NULL);/**/
		unitLoader.LoadUnit("ARM_RADAR_JAMMING_TOWER",float3(2850,10,4860),1,false,0,NULL);

		unitLoader.LoadUnit("ARM_RETALIATOR",float3(2650,10,2600),0,false,0,NULL);
		unitLoader.LoadUnit("ARM_SCARAB",float3(2250,10,4800),1,false,0,NULL);

		unitLoader.LoadUnit("ARM_ANNIHILATOR",float3(2075,10,5765),1,false,0,NULL);
//		unitLoader.LoadUnit("ARMANNI",float3(2275,10,5765),1,false,0,NULL);
//		unitLoader.LoadUnit("ARMANNI",float3(2475,10,5765),1,false,0,NULL);
//		unitLoader.LoadUnit("ARMROCK",float3(18,10,29),0,false,0,NULL);
		break;}
	case 3000:
		gs->paused = true;
		break;
	}
}
