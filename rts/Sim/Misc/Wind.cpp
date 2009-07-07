#include "StdAfx.h"
#include "mmgr.h"

#include "Wind.h"
#include "GlobalSynced.h"
#include "Sim/Units/UnitHandler.h"
#include "Map/MapInfo.h"

CR_BIND(CWind, );

CR_REG_METADATA(CWind, (

	CR_MEMBER(maxWind),
	CR_MEMBER(minWind),

	CR_MEMBER(curWind),
	CR_MEMBER(curStrength),
	CR_MEMBER(curDir),

	CR_MEMBER(newWind),
	CR_MEMBER(oldWind),
	CR_MEMBER(status),
	CR_RESERVED(12)
	));


CWind wind;


CWind::CWind()
{
	curDir=float3(1,0,0);
	curStrength=0;
	curWind=float3(0,0,0);
	newWind=curWind;
	oldWind=curWind;
	maxWind=300;
	minWind=50;
	status=895; //make sure we can read in the correct wind before we try to set it
}


CWind::~CWind()
{
}


void CWind::LoadWind()
{
	// TODO: decouple
	minWind = mapInfo->atmosphere.minWind;
	maxWind = mapInfo->atmosphere.maxWind;
}


void CWind::Update()
{
	if(status==0){
		oldWind=curWind;
		float ns=gs->randFloat()*(maxWind-minWind)+minWind;
		float nd=gs->randFloat()*2*PI;

		newWind=float3(sin(nd)*ns,0,cos(nd)*ns);

		// TODO: decouple
		uh->UpdateWind(newWind.x, newWind.z, newWind.Length());

		status++;
	} else if(status<=900) {
		float mod=status/900.0f;
		curWind=oldWind*(1.0-mod)+newWind*mod;
		curStrength=curWind.Length();
		curDir=curWind;
		curDir.Normalize();
		status++;
	} else if(status==901) {
		status=0;
	} else {
		status++;
	}
}
