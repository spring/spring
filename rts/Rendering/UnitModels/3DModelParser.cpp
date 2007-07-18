#include "StdAfx.h"
#include "3DModelParser.h"
#include "3DOParser.h"
#include "s3oParser.h"
#include <algorithm>
#include <cctype>

C3DModelParser* modelParser=0;

C3DModelParser::C3DModelParser(void)
{
	unit3doparser=SAFE_NEW C3DOParser();
	units3oparser=SAFE_NEW CS3OParser();
}

C3DModelParser::~C3DModelParser(void)
{
	delete unit3doparser;
	delete units3oparser;
}

S3DOModel* C3DModelParser::Load3DO(string name,float scale,int side)
{
	StringToLowerInPlace(name);
	if(name.find(".s3o")!=string::npos)
		return units3oparser->LoadS3O(name,scale,side);
	else
		return unit3doparser->Load3DO(name,scale,side);
}

S3DOModel* C3DModelParser::Load3DO(string name,float scale,int side,const float3& offsets)
{
	StringToLowerInPlace(name);
	if(name.find(".s3o")!=string::npos)
		return units3oparser->LoadS3O(name,scale,side);
	else
		return unit3doparser->Load3DO(name,scale,side,offsets);
}

LocalS3DOModel *C3DModelParser::CreateLocalModel(S3DOModel *model, vector<struct PieceInfo> *pieces)
{
	LocalS3DOModel* lm;
	if (model->rootobject3do) {
		lm = unit3doparser->CreateLocalModel(model,pieces);
	} else {
		lm = units3oparser->CreateLocalModel(model,pieces);
	}
	return lm;
}

void S3DOModel::DrawStatic()
{
	if(rootobject3do)
		rootobject3do->DrawStatic();
	else
		rootobjects3o->DrawStatic();
}
