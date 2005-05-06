#pragma once
#include "LoadSaveInterface.h"
#include <fstream>

class CLoadInterface :
	public CLoadSaveInterface
{
public:
	CLoadInterface(std::ifstream* ifs);
	~CLoadInterface(void);

	void lsBool(bool& v);
	void lsChar(char& v);
	void lsUChar(unsigned char& v);
	void lsInt(int& v);
	void lsShort(short int& v);
	void lsFloat(float& v);
	void lsFloat3(float3& v);
	void lsDouble(double& v);
	void lsString(std::string& v);

	std::ifstream* ifs;
};

