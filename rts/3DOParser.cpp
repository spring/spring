// 3DOParser.cpp: implementation of the C3DOParser class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "3DOParser.h"
#include <math.h>
//#include <iostream>
//#include <ostream>
//#include <fstream>
//#include <windows.h>
#include "myGL.h"
//#include <gl\glu.h>			// Header File For The GLu32 Library
#include "GlobalStuff.h"
#include "InfoConsole.h"
#include <vector>
#include "VertexArray.h"
#include "HpiHandler.h"
#include <set>
#include "FileHandler.h"
#include "FartextureHandler.h"
#include "CobInstance.h"
#include "TAPalette.h"
#include "Matrix44f.h"
#include <algorithm>
#include <locale>
#include <cctype>

#ifdef NO_3DO

C3DOParser::C3DOParser()
{
	return;
};

C3DOParser::~C3DOParser()
{
	return;
};

S3DOModel* C3DOParser::Load3DO(string,float,int)
{
 	return (S3DOModel*)0;
}

LocalS3DOModel * C3DOParser::CreateLocalModel(S3DOModel *model, vector<struct PieceInfo> *pieces)
{
	LocalS3DOModel *foo;
	return foo;
}

float3 LocalS3DOModel::GetPiecePos(int)
{
	float3 foo;
	return foo;
};

float3 LocalS3DOModel::GetPieceDirection(int)
{
	float3 foo;
	return foo;
};

void LocalS3DOModel::Draw()
{
};

LocalS3DOModel::~LocalS3DOModel()
{
};



CMatrix44f LocalS3DOModel::GetPieceMatrix(int)
{
	CMatrix44f foo;
	return foo;
};

void S3DO::DrawStatic()
{};
	
C3DOParser* unit3doparser;



#endif //NO_3DO


