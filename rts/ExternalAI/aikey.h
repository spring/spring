#ifndef AIKEY_H
#define AIKEY_H
// aikey.h: defines the AIKey data type, a pair of (dllName, aiNumber)
// which allows multiple AIs to exist within a single DLL. An instance
// of AIKey uniquely identifies a particular AI.
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <string>

struct AIKey
{
	std::string dllName;
	unsigned aiNumber;

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

