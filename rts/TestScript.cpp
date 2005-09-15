// TestScript.cpp: implementation of the CTestScript class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TestScript.h"
#include "UnitLoader.h"
#include "Unit.h"
#include "Weapon.h"
#include "ReadMap.h"
#include "Ground.h"
#include "FeatureHandler.h"
#include "TdfParser.h"
#include "Team.h"
#include "SelectedUnits.h"
#include "UnitHandler.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static CTestScript ts;

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

		for(int a=0;a<0000;++a){
			float3 pos((gs->randFloat()+gs->randFloat())*2000,0,(gs->randFloat()+gs->randFloat())*2000);
			pos.y=ground->GetHeight(pos.x,pos.z);
			int num=featureHandler->wreckParser.GetSectionList("").size();
			string feature=featureHandler->wreckParser.GetSectionList("")[(int)(gs->randFloat()*num)];
			featureHandler->CreateWreckage(pos,feature,0,1,-1,false,"");
		}
		for(int a=0;a<30;++a){
			unitLoader.LoadUnit("ARMFLASH",float3(2000+a*80,10,2900),0,false);
			unitLoader.LoadUnit("ARMZEUS",float3(2000+a*80,10,2855),0,false);
			unitLoader.LoadUnit("ARMMAV",float3(2000+a*80,10,2800),0,false);
			unitLoader.LoadUnit("CORAK",float3(2000+a*80,10,4560),1,false);
			unitLoader.LoadUnit("CORCAN",float3(2000+a*80,10,4595),1,false);
			unitLoader.LoadUnit("CORFAST",float3(2000+a*80,10,4640),1,false);
		}/**/

		unitLoader.LoadUnit("ARMROCK",float3(1800,10,2950),0,false);
		unitLoader.LoadUnit("ARMROCK",float3(1825,10,2950),0,false);
		unitLoader.LoadUnit("ARMROCK",float3(1850,10,2950),0,false);
		unitLoader.LoadUnit("ARMROCK",float3(1875,10,2950),0,false);
		unitLoader.LoadUnit("ARMLLT",float3(1900,10,2950),0,false);
		unitLoader.LoadUnit("ARMMART",float3(1800,10,2925),0,false);
		unitLoader.LoadUnit("ARMMART",float3(1825,10,2925),0,false);
		unitLoader.LoadUnit("ARMMART",float3(1850,10,2925),0,false);
		unitLoader.LoadUnit("ARMMERL",float3(1825,10,2910),0,false);
		unitLoader.LoadUnit("armcv",float3(1850,10,2905),0,false);

		unitLoader.LoadUnit("armck",float3(1850,10,2900),0,false);
		unitLoader.LoadUnit("ARMFLASH",float3(1900,10,2900),0,false);
		unitLoader.LoadUnit("ARMBULL",float3(1950,10,2900),0,false);
		unitLoader.LoadUnit("ARMSAM",float3(1875,10,2925),0,false);
		unitLoader.LoadUnit("ARMJETH",float3(1850,10,2890),0,false);
		unitLoader.LoadUnit("ARMPNIX",float3(1850,10,2600),0,false);
		unitLoader.LoadUnit("ARMHAWK",float3(1850,10,2680),0,false);
		unitLoader.LoadUnit("ARMBRAWL",float3(1950,10,2680),0,false);
		unitLoader.LoadUnit("ARMBRTHA",float3(1650,10,1550),0,false);
		unitLoader.LoadUnit("CORKROG",float3(1950,10,2750),0,false);
//		unitLoader.LoadUnit("ARMTHOVR",float3(1750,10,2550),0,false);

		unitLoader.LoadUnit("ARMMEX",float3(2850,10,2300),0,false);
		unitLoader.LoadUnit("ARMMOHO",float3(2950,10,2380),0,false);

		unitLoader.LoadUnit("ARMARAD",float3(2950,10,3780),0,false);

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

		unitLoader.LoadUnit("CORCAN",float3(1770,10,4540),1,false);
		unitLoader.LoadUnit("CORCAN",float3(1815,10,4540),1,false);
		unitLoader.LoadUnit("CORAK",float3(1850,10,4540),1,false);
		unitLoader.LoadUnit("CORAK",float3(1895,10,4540),1,false);
		unitLoader.LoadUnit("ARMZEUS",float3(2000,10,4580),1,false);
		unitLoader.LoadUnit("CORTHUD",float3(1750,10,4640),1,false);
		unitLoader.LoadUnit("CORRAID",float3(1800,10,4640),1,false);
		unitLoader.LoadUnit("CORGOL",float3(1850,10,4600),1,false);
		unitLoader.LoadUnit("CORPYRO",float3(1850,10,4700),1,false);
		unitLoader.LoadUnit("CORMORT",float3(1900,10,4700),1,false);
		unitLoader.LoadUnit("CORREAP",float3(2050,10,4660),1,false);
		unitLoader.LoadUnit("CORREAP",float3(1975,10,4580),1,false);

		unitLoader.LoadUnit("CORDOOM",float3(1875,10,4465),1,false);
		unitLoader.LoadUnit("CORSHAD",float3(1850,10,4840),1,false);
		unitLoader.LoadUnit("CORVAMP",float3(1850,10,4860),1,false);
		unitLoader.LoadUnit("CORTOAST",float3(1740,10,4480),1,false);/**/
		unitLoader.LoadUnit("x1ebcorjtorn",float3(2850,10,4860),1,false);

		unitLoader.LoadUnit("armsilo",float3(2650,10,2600),0,false);
		unitLoader.LoadUnit("armscab",float3(2250,10,4800),1,false);

		unitLoader.LoadUnit("ARMANNI",float3(2075,10,5765),1,false);
		unitLoader.LoadUnit("ARMANNI",float3(2275,10,5765),1,false);
		unitLoader.LoadUnit("ARMANNI",float3(2475,10,5765),1,false);
//		unitLoader.LoadUnit("ARMROCK",float3(18,10,29),0,false);
		break;
	}
}
