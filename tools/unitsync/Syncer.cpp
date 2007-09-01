
#include "StdAfx.h"
#include "Syncer.h"
#include "FileSystem/FileHandler.h"
#include "Lua/LuaParser.h"
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

CSyncer::CSyncer(int id) :
	populated(false)
{
	localId = id;
}

CSyncer::~CSyncer(void)
{
}

crc_t CSyncer::CalculateCRC(const string& fileName)
{
	CFileHandler file(fileName);
	if (!file.FileExists()) {
		return 0;
	}

	long size = file.FileSize();
	char *data = new char[size];
	file.Read(data, size);

	//todo: do a less silly algorithm
	crc_t cur = 0;
	for (int i = 0; i < size; ++i) {
		cur += data[i];
	}

	delete[] data;
	return cur;
}

void CSyncer::ParseUnit(const string& fileName)
{
	// FIXME - do nothing?
}

void CSyncer::MapUnitIds()
{
	for (map<string, Unit>::iterator i = units.begin(); i != units.end(); ++i) {
		unitIds.push_back(i->first);
	}
}

int CSyncer::ProcessUnits(bool checksum)
{
	if (populated && units.empty()) {
		return 0;
	}
	populated = true;

	int count = 0;

	LuaParser luaParser("gamedata/unitdefs.lua",
	                    SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!luaParser.Execute()) {
		return 0; // FIXME -- report this somehow?
	}

	LuaTable rootTable = luaParser.GetRoot();
	if (!rootTable.IsValid()) {
		return 0;
	}
	vector<string> unitDefNames;
	rootTable.GetKeys(unitDefNames);

	count = (int)unitDefNames.size();

	for ( int i = 0; i < count; ++i) {
		const string& udName =  unitDefNames[i];
		LuaTable udTable = rootTable.SubTable(udName);
		Unit u;
		u.fullName = udName;

		// FIXME -- only checksum the last unitDef?  (0.75b2 behaviour)
		if (checksum) {
			//The model filenames has to be figured out from the fbi file
			const string fileName  = udTable.GetString("filename", "");
			const string deadName  = udTable.GetString("corpse", udName + "_dead");
			const string modelName = udTable.GetString("objectname", udName);

			u.fbi    = CalculateCRC(fileName);
			u.cob    = CalculateCRC("scripts/" + udName + ".cob");
			u.model  = CalculateCRC("objects3d/" + modelName); // s3o ?
			u.model += CalculateCRC("objects3d/" + modelName + ".3do");
			u.model += CalculateCRC("objects3d/" +  deadName + ".3do");
		}

		units[udName] = u;
	}

	MapUnitIds(); // if we are done, map id numbers to names

	return units.size();
}

string CSyncer::GetCurrentList()
{
	ostringstream s("");

	for (map<string,Unit>::iterator i = units.begin(); i != units.end(); ++i) {
		s << i->first << " ";
		s << i->second.fbi << " ";
		s << i->second.cob << " ";
		s << i->second.model << " ";
	}

	return s.str();
}

void CSyncer::InstallClientDiff(const string& diff) 
{
	istringstream i(diff);

	int client;
	string name;
	bool wasRemove;
	int count;	

	while (i >> client) {
		i >> wasRemove;

		//Handle remove if so
		if (wasRemove) {
			RemoveClient(client);
			return;
		}

		// Alas, it was not..
		i >> count;
		for (int a = 0; a < count; ++a) {
			i >> name;
			map<string, DisabledUnit>::iterator found = disabledUnits.find(name);
			if (found != disabledUnits.end()) {
				disabledUnits[name].clients.insert(client);
			}
			else {
				DisabledUnit mu;
				mu.clients.insert(client);
				disabledUnits[name] = mu;
			}
		}
	}
}

void CSyncer::RemoveClient(int id)
{
	for (map<string, DisabledUnit>::iterator i = disabledUnits.begin(); i != disabledUnits.end(); ++i) {
		set<int>::iterator clientId = i->second.clients.find(id);
		if (clientId != i->second.clients.end()) {
			i->second.clients.erase(clientId);
			
			//could delete the unit from the map if the list is empty now
			//the speed increase should be insignificant though I guess
		}
	}
}

int CSyncer::GetUnitCount()
{
	return units.size();
}

string CSyncer::GetUnitName(int unit)
{
	string unitName = unitIds[unit];
	return unitName;
}

string CSyncer::GetFullUnitName(int unit)
{
	const string& unitName = unitIds[unit];
	return units[unitName].fullName;
}

bool CSyncer::IsUnitDisabled(int unit)
{
	string unitName = unitIds[unit];
	map<string, DisabledUnit>::iterator found = disabledUnits.find(unitName);
	if (found != disabledUnits.end())
		return found->second.clients.size() > 0;
	else
		return false;
}

bool CSyncer::IsUnitDisabledByClient(int unit, int clientId)
{
	string unitName = unitIds[unit];
	
	map<string, DisabledUnit>::iterator found = disabledUnits.find(unitName);
	if (found == disabledUnits.end())
		return false;

	set<int> &clients = found->second.clients;	
	set<int>::iterator foundId = clients.find(clientId);
	return foundId != clients.end();
}
