#include "StdAfx.h"
#include "Wind.h"
#include "Sim/Units/UnitHandler.h"
#include "Map/ReadMap.h"
#include "mmgr.h"

CWind wind;

CWind::CWind(void)
{
	curDir=float3(1,0,0);
	curStrength=0;
	curWind=float3(0,0,0);
	newWind=curWind;
	oldWind=curWind;
	maxWind=300;
	minWind=50;
	status=895;						//make sure we can read in the correct wind before we try to set it
}

CWind::~CWind(void)
{
}

void CWind::LoadWind()
{
	readmap->mapDefParser.GetDef(minWind,"5","MAP\\ATMOSPHERE\\MinWind");
	readmap->mapDefParser.GetDef(maxWind,"25","MAP\\ATMOSPHERE\\MaxWind");
}

void CWind::Update()
{
	if(status==0){
		oldWind=curWind;
		float ns=gs->randFloat()*(maxWind-minWind)+minWind;
		float nd=gs->randFloat()*2*PI;

		newWind=float3(sin(nd)*ns,0,cos(nd)*ns);

		uh->PushNewWind(newWind.x, newWind.z, newWind.Length());

		status++;
	} else if(status<=300) {
		float mod=status/300.0;
		curWind=oldWind*(1-mod)+newWind*mod;
		curStrength=curWind.Length();
		curDir=curWind;
		curDir.Normalize();
		status++;
	} else if(status==900) {
		status=0;
	} else {
		status++;
	}
}

