#include "stdafx.h"
#include ".\amphibopscript.h"
#include "unitloader.h"
//#include "mmgr.h"

static CAmphibOpScript aos;

CAmphibOpScript::CAmphibOpScript(void)
: CScript(std::string("Amphib op"))
{
}

CAmphibOpScript::~CAmphibOpScript(void)
{
}

void CAmphibOpScript::Update(void)
{
	switch(gs->frameNum){
	case 0:
		unitLoader.LoadUnit("ARMCOM",float3(5265,100,6900),0,false);
		unitLoader.LoadUnit("CORCOM",float3(3365,100,2420),1,false);

		for(int a=0;a<6;++a){
			unitLoader.LoadUnit("ARMFARK",float3(5265+a*40,100,6800),0,false);
			unitLoader.LoadUnit("CORNECRO",float3(3365+a*40,100,2520),1,false);

			unitLoader.LoadUnit("ARMACV",float3(5265+a*40,100,6700),0,false);
			unitLoader.LoadUnit("CORACV",float3(3365+a*40,100,2620),1,false);

			unitLoader.LoadUnit("ARMCV",float3(5265+a*40,100,6600),0,false);
			unitLoader.LoadUnit("CORCV",float3(3365+a*40,100,2520),1,false);

			unitLoader.LoadUnit("ARMFLASH",float3(5265+a*40,100,7200),0,false);
			unitLoader.LoadUnit("ARMMAV",float3(5265+a*40,100,7300),0,false);
			unitLoader.LoadUnit("ARMLATNK",float3(5265+a*40,100,7400),0,false);
		}
		unitLoader.LoadUnit("CORFMD",float3(3865,100,2520),1,false);
		unitLoader.LoadUnit("ARMSILO",float3(3865,100,2720),1,false);
		unitLoader.LoadUnit("ARMTHOVR",float3(5265,100,7500),0,false);
		unitLoader.LoadUnit("ARMTHOVR",float3(5465,100,7500),0,false);
		for(int a=0;a<20;++a){
			unitLoader.LoadUnit("ARMPNIX",float3(4765,100,6800+a*64),0,false);
			unitLoader.LoadUnit("CORSHAD",float3(2565,100,1520+a*64),1,false);

			unitLoader.LoadUnit("ARMHAWK",float3(4865,100,6800+a*64),0,false);
			unitLoader.LoadUnit("CORVAMP",float3(2665,100,1520+a*64),1,false);

			unitLoader.LoadUnit("CORWIN",float3(4064,100,1520+a*64),1,false);
			unitLoader.LoadUnit("CORWIN",float3(4128,100,1520+a*64),1,false);
		}
		for(int a=0;a<4;++a){
			unitLoader.LoadUnit("ARMBATS",float3(4565+a*128,100,5800),0,false);
			unitLoader.LoadUnit("ARMMSHIP",float3(4565+a*128,100,5928),0,false);
			unitLoader.LoadUnit("ARMAAS",float3(4565+a*128,100,6056),0,false);
		}
		for(int a=0;a<6;++a){
			unitLoader.LoadUnit("ARMPT",float3(3565+a*128,100,5800),0,false);
			unitLoader.LoadUnit("ARMPT",float3(3565+a*128,100,5928),0,false);
			unitLoader.LoadUnit("ARMROY",float3(3565+a*128,100,6056),0,false);

			unitLoader.LoadUnit("CORPT",float3(2565+a*128,100,3800),1,false);
			unitLoader.LoadUnit("CORPT",float3(2565+a*128,100,3928),1,false);
		}
		unitLoader.LoadUnit("ARMROY",float3(2765,100,4056),1,false);
		for(int a=0;a<10;++a){
			unitLoader.LoadUnit("ARMCROC",float3(2565+a*96,100,4800),0,false);
			unitLoader.LoadUnit("ARMCROC",float3(2565+a*96,100,5300),0,false);

			unitLoader.LoadUnit("CORTIDE",float3(2565+a*48,100,3500),1,false);
			unitLoader.LoadUnit("CORTIDE",float3(2565+a*48,100,3532),1,false);
			unitLoader.LoadUnit("CORFMKR",float3(2565+a*48,100,3564),1,false);
		}
		for(int a=0;a<8;++a){
			unitLoader.LoadUnit("ARMSH",float3(4565+a*96,100,4904),0,false);
			unitLoader.LoadUnit("ARMANAC",float3(4565+a*96,100,5000),0,false);
			unitLoader.LoadUnit("ARMAH",float3(4565+a*96,100,5096),0,false);
			if(a&1)
				unitLoader.LoadUnit("ARMMH",float3(4565+a*96,100,5192),0,false);
		}
		break;
	}
}

std::string CAmphibOpScript::GetMapName(void)
{
	return "map4.sm2";
}
