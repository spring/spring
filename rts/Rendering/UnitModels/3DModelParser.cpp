#include "StdAfx.h"
#include "3DModelParser.h"
#include "3DOParser.h"
#include "s3oParser.h"
#include <algorithm>
#include <cctype>

C3DModelParser* modelParser=0;

C3DModelParser::C3DModelParser(void)
{
	unit3doparser=new C3DOParser();
	units3oparser=new CS3OParser();
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
		return units3oparser->Load3DO(name,scale,side);
	else
		return unit3doparser->Load3DO(name,scale,side);
}

LocalS3DOModel *C3DModelParser::CreateLocalModel(S3DOModel *model, vector<struct PieceInfo> *pieces)
{
	if(model->rootobject3do)
		return unit3doparser->CreateLocalModel(model,pieces);
	else
		return units3oparser->CreateLocalModel(model,pieces);
}

void S3DOModel::DrawStatic()
{
	if(rootobject3do)
		rootobject3do->DrawStatic();
	else
		rootobjects3o->DrawStatic();
}
