#ifndef S3OPARSER_H
#define S3OPARSER_H
// s3oParser.h: interface for the Cs3oParser class.
//
//////////////////////////////////////////////////////////////////////

#include <vector>

#include <string>
//#include <windows.h>
#include "s3o.h"

using namespace std;

struct SsPrimitive {
	std::vector<int> vertexIdexes;
};

struct LObject{
	int type;

	unsigned int displist;
	float3 turn;

	std::vector<LObject> childs;
	float3 offset;
	string name;
	
	std::vector<SsPrimitive> prims;
	std::vector<VertexData> vertdata;	
};

class CS3OParser  
{
public:

	CS3OParser();
	virtual ~CS3OParser();
	LObject *Parse(const string& filename);
private:
	void ReadObject(LObject &s3o, FILE *pStream, int fileOffset);
	void ReadVertices(LObject &s3o, FILE *pStream, int fileOffset, int num);
	void ReadPrimitives(LObject &s3o, FILE *pStream, int fileOffset, int num);
	void ReadName(LObject &s3o, FILE *pStream, int fileOffset);
	void ReadVertexIndexes(SsPrimitive &prim, FILE *pStream, int fileOffset, int num);
	void ComputeNormals(LObject &object);
	void GenerateSmoothVertices(LObject &object);
};

#endif /* S3OPARSER_H */
