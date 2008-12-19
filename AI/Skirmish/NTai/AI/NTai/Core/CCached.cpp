/*
 *  CCached.cpp
 *  ntai-xcode
 *
 *  Created by Tom Nowell on 18/12/2008.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "include.h"

namespace ntai {
	//
	
	CCached::CCached(Global* G){
		//
		comID = 0;
		randadd = 0;
		enemy_number = 0;
		lastcacheupdate = 0;
		
		team = G->cb->GetMyTeam();
		
		cheating = false;
		encache = new int[6001];
		
		//CLOG("Getting LOS pointer");
		losmap = G->cb->GetLosMap();
		
		//CLOG("initialising enemy cache elements to zero");
		for(int i = 0; i< 6000; i++){
			encache[i] = 0;
		}
	}
}
