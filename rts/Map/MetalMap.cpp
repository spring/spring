#include "StdAfx.h"
#include "MetalMap.h"
#include "GlobalStuff.h"
#include "ReadMap.h"
#include "Platform/ConfigHandler.h"
#include "mmgr.h"


/*
Constructor
Reciving a map over all metal, and creating a map over extraction.
*/
CMetalMap::CMetalMap(unsigned char *map, int sizex, int sizez, float metalscale)
{
	this->metalMap = map;
	this->sizex = sizex;
	this->sizez = sizez;
	this->metalscale = metalscale;
	
	//Creating an empty map over extraction.
	extractionMap = SAFE_NEW float[sizex * sizez];
	int i;
	for(i = 0; i < (sizex * sizez); i++) {
		extractionMap[i] = 0.0f;
	}
	
	int whichPalette = configHandler.GetInt("MetalMapPalette", 0);

	if (whichPalette == 1){
		/* Swap the green and blue channels. making metal go
		   black -> blue -> cyan,
		   rather than the usual black -> green -> cyan. */
		for(int a=0;a<256;++a){
			metalPal[a*3+0]=a;
			metalPal[a*3+1]=std::max(0,a*2-255);
			metalPal[a*3+2]=std::min(255,a*2);
		}
	}
	else {
		for(int a=0;a<256;++a){
			metalPal[a*3+0]=a;
			metalPal[a*3+1]=std::min(255,a*2);
			metalPal[a*3+2]=std::max(0,a*2-255);//a/2+((a*4)&127);
		}
	}
	
}


/*
Destrcutor
Free the memory used by maps.
*/
CMetalMap::~CMetalMap(void)
{
	delete[] metalMap;
	delete[] extractionMap;
}


/*
Gives the amount of metal over an area.
*/
float CMetalMap::getMetalAmount(int x1, int z1, int x2, int z2) 
{
	if(x1<0)
		x1=0;
	else if(x1>=sizex)
		x1=sizex-1;
	if(x2<0)
		x2=0;
	else if(x2>=sizex)
		x2=sizex-1;

	if(z1<0)
		z1=0;
	else if(z1>=sizez)
		z1=sizez-1;
	if(z2<0)
		z2=0;
	else if(z2>=sizez)
		z2=sizez-1;

	float metal = 0.0f;
	int x, z;
	for(x = x1; x < x2; x++) {
		for(z = z1; z < z2; z++) {
			metal += metalMap[x + z*sizex];
		}
	}
	return metal * metalscale;
}


/*
Gives the amount of metal on a single square.
*/
float CMetalMap::getMetalAmount(int x, int z) 
{
	if(x<0)
		x=0;
	else if(x>=sizex)
		x=sizex-1;

	if(z<0)
		z=0;
	else if(z>=sizez)
		z=sizez-1;

	return metalMap[x + z*sizex] * metalscale;
}


/*
Makes a reqest for extracting metal from a given square.
If there is metal left to extract to the requested depth,
the amount available will be returned and the requested
depth will be sat as new extraction-depth on the extraction-map.
If the requested depth is grounder than the current
extraction-depth 0.0 will be returned and nothing changed.
*/
float CMetalMap::requestExtraction(int x, int z, float toDepth) 
{
	if(x<0)
		x=0;
	else if(x>=sizex)
		x=sizex-1;

	if(z<0)
		z=0;
	else if(z>=sizez)
		z=sizez-1;

	if(toDepth <= extractionMap[x + z*sizex])
		return 0;

	float available = toDepth - extractionMap[x + z*sizex];
	extractionMap[x + z*sizex] = toDepth;
	return available;
}


/*
When a extraction ends, the digged depth should be left
back to the extraction-map. To be available for other
extractors to use.
*/
void CMetalMap::removeExtraction(int x, int z, float depth) 
{
	if(x<0)
		x=0;
	else if(x>=sizex)
		x=sizex-1;

	if(z<0)
		z=0;
	else if(z>=sizez)
		z=sizez-1;

	extractionMap[x + z*sizex] -= depth;
}
