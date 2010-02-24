/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SAVEINTERFACE_H
#define SAVEINTERFACE_H

#include "LoadSaveInterface.h"
#include <fstream>

class CSaveInterface :
	public CLoadSaveInterface
{
public:
	CSaveInterface(std::ofstream* ofs);
	~CSaveInterface(void);

	void lsBool(bool& v);
	void lsChar(char& v);
	void lsUChar(unsigned char& v);
	void lsInt(int& v);
	void lsShort(short int& v);
	void lsFloat(float& v);
	void lsFloat3(float3& v);
	void lsDouble(double& v);
	void lsString(std::string& v);

	std::ofstream* ofs;
};


#endif /* SAVEINTERFACE_H */
