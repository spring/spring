#ifndef SYNCER_H
#define SYNCER_H

#include <map>
#include <vector>
#include <set>

using namespace std;


typedef unsigned int crc_t;


struct Unit
{
	crc_t fbi;
	crc_t cob;
	crc_t model;

	string fullName;
};


struct DisabledUnit {
	set<int> clients;
};


class CSyncer
{
	public:
		CSyncer(int id);
		~CSyncer(void);
		string GetCurrentList();
		void InstallClientDiff(const string& diff);
		virtual int ProcessUnits(bool checksum = true);
		
		int GetUnitCount();
		string GetUnitName(int unit);
		string GetFullUnitName(int unit);
		bool IsUnitDisabled(int unit);
		bool IsUnitDisabledByClient(int unit, int clientId);

	protected:
		void LoadUnits(bool checksum);
		void RemoveClient(int id);
		crc_t CalculateCRC(const string& fileName);

	protected:
		int localId;
		int unitsLeft; // decrements for each ProcessUnits() call,
		               // causes LoadUnits() to be called when set to -1
		vector<string> unitIds;
		map<string, Unit> units;
		map<string, DisabledUnit> disabledUnits;
};

#endif
