#include "StdAfx.h"
#include "SWTATestScrip.h"
#include "Sim/Units/UnitLoader.h"
//#include "mmgr.h"

static CSWTATestScrip swtasTest;

CSWTATestScrip::CSWTATestScrip(void)
: CScript(std::string("SWTA Test"))
{
}

CSWTATestScrip::~CSWTATestScrip(void)
{
}

void CSWTATestScrip::Update(void)
{
	switch(gs->frameNum){
	case 0:
		unitLoader.LoadUnit("REBCOM",float3(3000,100,3400),1,false);

		unitLoader.LoadUnit("REBEGEN",float3(3200,100,3400),1,false);
		unitLoader.LoadUnit("REBFUS",float3(3600,100,3400),1,false);
		unitLoader.LoadUnit("REBGEO",float3(3600,100,3600),1,false);
		unitLoader.LoadUnit("REBESTOR",float3(2700,100,3800),1,false);
		unitLoader.LoadUnit("REBESTOR",float3(3700,100,3800),1,false);
		unitLoader.LoadUnit("REBVP",float3(3700,100,3400),1,false);

		unitLoader.LoadUnit("REBAAA",float3(3100,100,3400),1,false);
		unitLoader.LoadUnit("REBATGAR",float3(2900,100,3400),1,false);
		unitLoader.LoadUnit("REBATGAR",float3(2900,100,3600),1,false);
		unitLoader.LoadUnit("REBATGAR",float3(2900,100,3200),1,false);

		unitLoader.LoadUnit("REBHT",float3(3100,100,3500),1,false);
	
		unitLoader.LoadUnit("REBAA",float3(3000,100,3500),1,false);
		unitLoader.LoadUnit("REBAA",float3(3200,100,3500),1,false);
		unitLoader.LoadUnit("REBAA",float3(3400,100,3500),1,false);

		unitLoader.LoadUnit("REBGOLAN",float3(3000,100,3700),1,false);
		unitLoader.LoadUnit("REBGOLAN",float3(3200,100,3700),1,false);
		unitLoader.LoadUnit("REBGOLAN",float3(3400,100,3700),1,false);

		unitLoader.LoadUnit("REBAT",float3(3000,100,3800),1,false);

		for(int a=0;a<8;++a){
			unitLoader.LoadUnit("REBWOOK",float3(3000+a*20,100,3850),1,false);
			unitLoader.LoadUnit("REBTROOPER",float3(3000+a*20,100,3870),1,false);
			unitLoader.LoadUnit("REBTROOPER",float3(3000+a*20,100,3900),1,false);
		}

		for(int a=0;a<4;++a){
			unitLoader.LoadUnit("REBELITE",float3(2800+a*100,100,4000),1,false);
		}

		for(int a=0;a<4;++a){
			unitLoader.LoadUnit("REBXWING",float3(5000+a*90,100,3800),1,false);
			unitLoader.LoadUnit("REBAWING",float3(5000+a*90,100,3900),1,false);
			unitLoader.LoadUnit("REBYWING",float3(4500+a*90,100,2700),1,false);
			unitLoader.LoadUnit("REBBWING",float3(4500+a*90,100,2600),1,false);
		}



		unitLoader.LoadUnit("IMPCOM",float3(1365,100,2720),2,false);

		for(int a=0;a<8;++a){
			unitLoader.LoadUnit("IMPSTORM",float3(2500+a*20,500,4700),2,false);
			unitLoader.LoadUnit("IMPSTORM",float3(2500+a*20,500,4720),2,false);
			unitLoader.LoadUnit("IMPSTORM",float3(2500+a*20,500,4740),2,false);

			unitLoader.LoadUnit("IMPDES",float3(2200+a*20,500,5540),2,false);
			unitLoader.LoadUnit("IMPBDROIDC",float3(2200+a*20,500,5560),2,false);
			unitLoader.LoadUnit("IMPSBDROID",float3(2200+a*20,500,5580),2,false);
			unitLoader.LoadUnit("IMPRKT",float3(2200+a*20,500,5590),2,false);

		}

		for(int a=0;a<6;++a){
			unitLoader.LoadUnit("IMPTIEF",float3(4500+a*100,500,7200),2,false);
			unitLoader.LoadUnit("IMPTIEI",float3(5000+a*100,500,8000),2,false);		
		}

		for(int a=0;a<6;++a){
			unitLoader.LoadUnit("IMPTIEB",float3(1800+a*100,500,6000),2,false);		
			unitLoader.LoadUnit("IMPTIEB",float3(1800+a*100,500,7000),2,false);		
		}
		for(int a=0;a<6;++a){
			unitLoader.LoadUnit("IMPTIEA",float3(1000+a*100,500,6700),2,false);
			unitLoader.LoadUnit("IMPTIED",float3(1000+a*100,500,7700),2,false);
		}

		for(int a=0;a<4;++a){
			unitLoader.LoadUnit("IMPATST",float3(2500+a*100,500,5100),2,false);
			unitLoader.LoadUnit("IMPATPT",float3(2500+a*100,500,5000),2,false);		

			unitLoader.LoadUnit("IMPATST",float3(2500+a*100,500,6100),2,false);
			unitLoader.LoadUnit("IMPATPT",float3(2500+a*100,500,6000),2,false);		
		}
		for(int a=0;a<4;++a){
			unitLoader.LoadUnit("IMPRKT",float3(2500+a*100,500,5300),2,false);		
			unitLoader.LoadUnit("IMPMOBILEA",float3(2500+a*100,500,5200),2,false);
		}

		break;
	}
}

