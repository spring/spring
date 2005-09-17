#ifndef READMAP_H
#define READMAP_H
// ReadMap.h: interface for the CReadMap class.
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>
#include "mapfile.h"
#include "float3.h"
#include "MetalMap.h"
#include "TdfParser.h"

class CFileHandler;
class CUnit;
class CSolidObject;
class CFileHandler;
class CLoadSaveInterface;

using namespace std;

class CReadMap  
{
public:
	virtual ~CReadMap();

	void AddGroundBlockingObject(CSolidObject *object);
	void AddGroundBlockingObject(CSolidObject *object, unsigned char *blockingMap);
	void RemoveGroundBlockingObject(CSolidObject *object);
	void MoveGroundBlockingObject(CSolidObject *object, float3 oldPos);
	void OpenBlockingYard(CSolidObject *yard, unsigned char *blockingMap);
	void CloseBlockingYard(CSolidObject *yard);
	CSolidObject* GroundBlocked(float3 pos);
	CSolidObject* GroundBlocked(int mapSquare);
	void CleanBlockingMap(CSolidObject* object);	//Debug

	unsigned int bigtex[16],minimapTex,detailtex,detailtex2;

	static CReadMap* Instance();

	float* heightmap;									//höjd
	float* orgheightmap;							//höjd utan skada
	float* centerheightmap;						//höjd i mitten av squares
	float* halfHeightmap;							//höjd i halva upplösningen (för los etc)
	float* slopemap;
//	float3* normals;
	float3* facenormals;							//normalerna för trianglarna (2 per square)
	unsigned char* typemap;						//typ av terräng (skog,väg etc
	unsigned char* heightLinePal;			//palette för höjd karte visning

	struct TerrainType{
		std::string name;
		float hardness;
		float tankSpeed;
		float kbotSpeed;
		float hoverSpeed;
		float shipSpeed;
		int receiveTracks;
	};

	TerrainType terrainTypes[256];

	CSolidObject** groundBlockingObjectMap;				//Pekar-karta till alla blockerande objekt på marken.
	CMetalMap *metalMap;					//Metal-density/height-map

	float tidalStrength;
	
	float3 groundCols[256];

//	unsigned char* yardmapLevels[5];
	unsigned char yardmapPal[256*3];
	unsigned int mapChecksum;
protected:
	int CReadMap::LoadInt(CFileHandler* ifs);
	CReadMap();
	static CReadMap* _instance;
public:
	virtual void RecalcTexture(int x1, int x2, int y1, int y2)=0;
	virtual void Update(){};
	virtual void Explosion(float x,float y,float strength){};
	virtual void ExplosionUpdate(int x1,int x2,int y1,int y2){};
//	void AddYardmap(CSolidObject* unit,bool openYardmap);
//	void RemoveYardmap(CSolidObject* unit,bool openYardmap);

	MapHeader header;
	CFileHandler* ifs;
  TdfParser mapDefParser;

	float3 ambientColor;
	float3 sunColor;
	float shadowDensity;
	float extractorRadius;			//extraction radius for mines
	bool voidWater;

	float minheight,maxheight;
	void LoadSaveMap(CLoadSaveInterface* file,bool loading);

	std::string skyBox;
};

extern CReadMap* readmap;

//Converts a map-square into a float3-position.
inline float3 SquareToFloat3(int xSquare, int zSquare) {return float3(((xSquare))*SQUARE_SIZE, readmap->centerheightmap[(xSquare) + (zSquare) * gs->mapx], ((zSquare))*SQUARE_SIZE);};

#endif /* READMAP_H */
