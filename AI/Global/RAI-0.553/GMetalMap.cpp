#include "GMetalMap.h"
#include <set>

cRMetalMap::cRMetalMap(IAICallback* cb, cLogFile* l)
{
	*l<<"\n Get Max Metal: "<<cb->GetMaxMetal();

	int MetalZSize =cb->GetMapHeight() / 2;
	int MetalXSize =cb->GetMapWidth() / 2;
	int XtractorRadius = int(cb->GetExtractorRadius()/16.0);
	const unsigned char *MetalMap = cb->GetMetalMap();
	set<int> Remaining;
	set<int> Discarded;
	set<int> Current;
	for(int z=0; z < MetalZSize; z++)
	{
//		*l<<"\n";
		for(int x=0; x < MetalXSize; x++)
		{
//			*l<<" "<<MetalMap[z*MetalXSize + x];
			if( MetalMap[z*MetalXSize + x] > '0' )
			{
				Remaining.insert(z*MetalXSize + x);
//				*l<<"*";
			}
		}
	}

	while( int(Remaining.size()) > 0 )
	{
		int m = *Remaining.begin();
		Remaining.erase(m);
		Current.insert(m);
	}
}

cRMetalMap::~cRMetalMap()
{

}
