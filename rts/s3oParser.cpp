// s3oParser.cpp: implementation of the Cs3oParser class.
//
//////////////////////////////////////////////////////////////////////
//#include "StdAfx.h"
#include "StdAfx.h"
#include "s3oParser.h"
#include <fstream>
#include "windows.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CS3OParser::CS3OParser()
{

}

CS3OParser::~CS3OParser()
{

}

LObject* CS3OParser::Parse(const string& filename)
{
	LObject *s3o = new LObject;

	FILE *pStream = fopen(filename.c_str(),"rb");
	if(!pStream){
		return NULL;
	}

	S3OHeader header;
    fread(&header, sizeof(S3OHeader), 1, pStream);
	if(header.signature!=S3OSIGNATURE)
	{
		fclose(pStream);
		return NULL;
	}

	S3OObject s3oobject;
	fseek(pStream, header.offsetToBaseObject, SEEK_SET);
	fread(&s3oobject.object, sizeof(s3oobject.object), 1, pStream);

	s3o->offset = *((float3*)&s3oobject.object.offsetFromParrent);
	//s3o->turn = float3(0,0,0);
	s3o->type = s3oobject.object.type;

	switch(s3oobject.object.type)
	{
	case OBJECT_TYPE_GEOMETRY:
		fread(&s3oobject.geometry, sizeof(s3oobject.geometry), 1, pStream);
		ReadVertices(*s3o, pStream, s3oobject.geometry.offsetToVertexArray, s3oobject.geometry.numVertex);
		ReadPrimitives(*s3o, pStream, s3oobject.geometry.offsetToPrimitiveArray, s3oobject.geometry.numPrimitives);			
		break;
	case OBJECT_TYPE_POINT:
		break;
	/*case OBJECT_TYPE_LIGHT:
		fread(&s3oobject.Light, sizeof(s3oobject.Light), 1, pStream);
		s3o->lightType = s3oobject.Light.Type;
		s3o->color = *((float3*)&s3oobject.Light.Color);
		s3o->direction = *((float3*)&s3oobject.Light.Direction);
		s3o->attenuation = s3oobject.Light.Attenuation;
		s3o->spotSize = s3oobject.Light.SpotSize;
		break;*/
	}

	ReadName(*s3o, pStream, s3oobject.object.offsetToObjectName);
	//ComputeNormals(*s3o);

	if(s3oobject.object.offsetToChildObject)
		ReadObject(*s3o, pStream, s3oobject.object.offsetToChildObject);
	
	return s3o;
}

void CS3OParser::ReadObject(LObject &s3o, FILE *pStream, int fileOffset)
{
	S3OObject s3oobject;
	fseek(pStream, fileOffset, SEEK_SET);
	fread(&s3oobject.object, sizeof(s3oobject.object), 1, pStream);

	LObject child;

	switch(s3oobject.object.type)
	{
	case OBJECT_TYPE_GEOMETRY:
		fread(&s3oobject.geometry, sizeof(s3oobject.geometry), 1, pStream);
		ReadVertices(child, pStream, s3oobject.geometry.offsetToVertexArray, s3oobject.geometry.numVertex);
		ReadPrimitives(child, pStream, s3oobject.geometry.offsetToPrimitiveArray, s3oobject.geometry.numPrimitives);
		break;
	case OBJECT_TYPE_POINT:
		break;
	/*case OBJECT_TYPE_LIGHT:
		fread(&s3oobject.Light, sizeof(s3oobject.Light), 1, pStream);
			child.lightType = s3oobject.Light.Type;
			child.color = *((float3*)&s3oobject.Light.Color);
			child.direction = *((float3*)&s3oobject.Light.Direction);
			child.attenuation = s3oobject.Light.Attenuation;
			child.spotSize = s3oobject.Light.SpotSize;
		break;*/
	}

	child.offset = *((float3*)&s3oobject.object.offsetFromParrent);
	//child.turn = float3(0,0,0);
	child.type = s3oobject.object.type;
	
	ReadName(child, pStream, s3oobject.object.offsetToObjectName);

	if(s3oobject.object.offsetToChildObject)
		ReadObject(child, pStream, s3oobject.object.offsetToChildObject);
	if(s3oobject.object.offsetToSiblingObject)
		ReadObject(s3o, pStream, s3oobject.object.offsetToSiblingObject);

	s3o.childs.push_back(child);
}

void CS3OParser::ReadVertices(LObject &s3o, FILE *pStream, int fileOffset, int num)
{
	fseek(pStream, fileOffset, SEEK_SET);
	for(int i=0; i<num; i++)
	{
		VertexData vertdata;
		fread(&vertdata, sizeof(VertexData), 1, pStream);
		s3o.vertdata.push_back(vertdata);
	}
}

void CS3OParser::ReadPrimitives(LObject &s3o, FILE *pStream, int fileOffset, int num)
{

	for(int i=0; i<num; i++)
	{
		fseek(pStream, fileOffset + i*sizeof(S3OPrimitive), SEEK_SET);
		S3OPrimitive prim;
		fread(&prim, sizeof(S3OPrimitive), 1, pStream);

		SsPrimitive sprim;

		ReadVertexIndexes(sprim, pStream, prim.offsetToVertexIndexArray, prim.numberOfVertexIndexes);

		s3o.prims.push_back(sprim);
	}
}

void CS3OParser::ReadVertexIndexes(SsPrimitive &prim, FILE *pStream, int fileOffset, int num)
{
	fseek(pStream, fileOffset, SEEK_SET);
	for(int i=0; i<num; i++)
	{
		VertexIndexStruct vertindex;
		fread(&vertindex, sizeof(VertexIndexStruct), 1, pStream);

		int svertindex;
		svertindex = vertindex.vertexIndex;

		prim.vertexIdexes.push_back(svertindex);
	}
}

void CS3OParser::ReadName(LObject &s3o, FILE *pStream, int fileOffset)
{
	fseek(pStream, fileOffset, SEEK_SET);

	char Name[256];
	fread(Name, 256, 1, pStream);

	s3o.name = Name;

}