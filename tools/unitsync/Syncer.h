#ifndef SYNCER_H
#define SYNCER_H

#include <map>
#include <vector>
#include <set>
#include <string>

typedef unsigned int crc_t;


struct Unit
{
	crc_t fbi;
	crc_t cob;
	crc_t model;

	std::string fullName;
};


struct DisabledUnit {
    std::set<int> clients;
};


class CSyncer
{
	public:
		CSyncer(int id);
		~CSyncer(void);
		std::string GetCurrentList();
		void InstallClientDiff(const std::string& diff);
		virtual int ProcessUnits(bool checksum = true);
		
		int GetUnitCount();
		std::string GetUnitName(int unit);
		std::string GetFullUnitName(int unit);
		bool IsUnitDisabled(int unit);
		bool IsUnitDisabledByClient(int unit, int clientId);

	protected:
		void LoadUnits(bool checksum);
		void RemoveClient(int id);
		crc_t CalculateCRC(const std::string& fileName);

	protected:
		int localId;
		int unitsLeft; // decrements for each ProcessUnits() call,
		               // causes LoadUnits() to be called when set to -1
		std::vector<std::string> unitIds;
		std::map<std::string, Unit> units;
		std::map<std::string, DisabledUnit> disabledUnits;
};

#endif
