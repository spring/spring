/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SYNCER_H
#define _SYNCER_H

#include <map>
#include <vector>
#include <set>
#include <string>

class CSyncer
{
	public:
		CSyncer();
		~CSyncer();
		virtual int ProcessUnits();

		int GetUnitCount();
		std::string GetUnitName(int unit);
		std::string GetFullUnitName(int unit);

	protected:
		void LoadUnits();

	protected:
		struct Unit
		{
			std::string fullName;
		};

		std::vector<std::string> unitIds;
		std::map<std::string, Unit> units;
};

#endif // _SYNCER_H
