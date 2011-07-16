/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SYNCER_H
#define _SYNCER_H

#include <vector>
#include <string>

class CSyncer
{
public:
	CSyncer();
	~CSyncer();

	virtual int ProcessUnits();

	int GetUnitCount();
	const std::string& GetUnitName(int unit);
	const std::string& GetFullUnitName(int unit);

protected:
	void LoadUnits();

protected:
	struct Unit
	{
		std::string name;
		std::string fullName;
	};

	std::vector<Unit> units;
};

#endif // _SYNCER_H
