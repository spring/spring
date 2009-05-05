/* Author: Tobi Vollebregt */

#ifndef UNITSCRIPTNAMES_H
#define UNITSCRIPTNAMES_H


#include <string>
#include <vector>


class CUnitScriptNames
{
	static const std::vector<std::string>& GetScriptNames();
	static int GetScriptNumber(const std::string& fname);
	static const std::string& GetScriptName(int num);
};

#endif
