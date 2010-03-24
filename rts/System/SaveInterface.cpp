/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "SaveInterface.h"

CSaveInterface::CSaveInterface(std::ofstream* ofs)
: ofs(ofs)
{
}

CSaveInterface::~CSaveInterface(void)
{
}

void CSaveInterface::lsBool(bool& v)
{
	ofs->write((char*)&v,sizeof(bool));
}

void CSaveInterface::lsChar(char& v)
{
	ofs->write((char*)&v,sizeof(char));
}

void CSaveInterface::lsUChar(unsigned char& v)
{
	ofs->write((char*)&v,sizeof(unsigned char));
}

void CSaveInterface::lsInt(int& v)
{
	ofs->write((char*)&v,sizeof(int));
}

void CSaveInterface::lsShort(short int& v)
{
	ofs->write((char*)&v,sizeof(short int));
}

void CSaveInterface::lsFloat(float& v)
{
	ofs->write((char*)&v,sizeof(float));
}

void CSaveInterface::lsFloat3(float3& v)
{
	ofs->write((char*)&v.x,sizeof(float)*3);
}

void CSaveInterface::lsDouble(double& v)
{
	ofs->write((char*)&v,sizeof(double));
}

void CSaveInterface::lsString(std::string& v)
{
	int size=v.size();
	ofs->write((char*)&size,sizeof(int));
	ofs->write(v.c_str(),size);
}
