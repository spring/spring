#include "StdAfx.h"
#include "mmgr.h"

#include "LoadInterface.h"

CLoadInterface::CLoadInterface(std::ifstream* ifs)
: ifs(ifs)
{
}

CLoadInterface::~CLoadInterface(void)
{
}

void CLoadInterface::lsBool(bool& v)
{
	ifs->read((char*)&v,sizeof(bool));
}

void CLoadInterface::lsChar(char& v)
{
	ifs->read((char*)&v,sizeof(char));
}

void CLoadInterface::lsUChar(unsigned char& v)
{
	ifs->read((char*)&v,sizeof(unsigned char));
}

void CLoadInterface::lsInt(int& v)
{
	ifs->read((char*)&v,sizeof(int));
}

void CLoadInterface::lsShort(short int& v)
{
	ifs->read((char*)&v,sizeof(short int));
}

void CLoadInterface::lsFloat(float& v)
{
	ifs->read((char*)&v,sizeof(float));
}

void CLoadInterface::lsFloat3(float3& v)
{
	ifs->read((char*)&v.x,sizeof(float)*3);
}

void CLoadInterface::lsDouble(double& v)
{
	ifs->read((char*)&v,sizeof(double));
}

void CLoadInterface::lsString(std::string& v)
{
	int size;
	ifs->read((char*)&size,sizeof(int));

	char* txt=new char[size+2];
	ifs->read(txt,size);
	txt[size]=0;
	v=txt;
}
