#include "../helper.h"

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

void Line::Draw(Global* G, float3 rpos, int scale){
	float3 Tstart = rpos + (start*scale);
	float3 Tend = rpos +(end*scale);
	G->Pl->DrawLine(colour,Tstart,Tend,FPS,width,arrow);
	/* draw this line relative to rpos */
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

