#ifndef AIKEY_H
#define AIKEY_H
// aikey.h: defines the AIKey data type, a pair of (dllName, aiNumber)
// which allows multiple AIs to exist within a single DLL. An instance
// of AIKey uniquely identifies a particular AI.
//////////////////////////////////////////////////////////////////////

#include <string>

#include "creg/creg.h"

struct AIKey
{
	CR_DECLARE_STRUCT(AIKey);
	std::string dllName;
	unsigned aiNumber;

	AIKey(){}
	AIKey(std::string DllName,unsigned AiNumber)
	: dllName(DllName), aiNumber(AiNumber)
	{}

	bool operator<(const AIKey&that) const
	{
		if(dllName==that.dllName){
			return aiNumber<that.aiNumber;
		} else {
			return dllName<that.dllName;
		}
	};

	bool operator!=(const AIKey&that) const
	{
		return ! ((* this) == that);
	};

	bool operator==(const AIKey&that) const
	{
		return (aiNumber==that.aiNumber)
			&& (dllName==that.dllName);
	};

};

#endif // AIKEY_H
