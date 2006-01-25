#pragma once

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
protected:
	bool populated;
	vector<string> files;
	map<string,Unit> units;
	map<string, DisabledUnit> disabledUnits;
	vector<string> unitIds;
	void RemoveClient(int id);
	crc_t CalculateCRC(const string& fileName);
	void ParseUnit(const string& fileName);
	int localId;
	void MapUnitIds();
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
};
