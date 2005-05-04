#include "StdAfx.h"
#include ".\syncer.h"
#include "FileHandler.h"
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include "SunParser.h"

CSyncer::CSyncer(int id)
{
	//Populate the list of unit files to consider
	files = CFileHandler::FindFiles("units\\*.fbi");
	localId = id;
}

CSyncer::~CSyncer(void)
{
}

crc_t CSyncer::CalculateCRC(string fileName)
{
	//return 1;

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

void CSyncer::ParseUnit(string fileName)
{
	CSunParser *p = new CSunParser();

	p->LoadFile(fileName);

	delete p;
}

void CSyncer::MapUnitIds()
{
	for (map<string, Unit>::iterator i = units.begin(); i != units.end(); ++i) {
		unitIds.push_back(i->first);
	}
}

int CSyncer::ProcessUnits()
{
	if (files.size() == 0) {
		return 0;
	}

	string curFile = files.back();
	files.pop_back();

	size_t len = curFile.find_last_of("\\")+1;
	string unitName = curFile.substr(len, curFile.size() - 4 - len);
	transform(unitName.begin(), unitName.end(), unitName.begin(), (int (*)(int))tolower);

	CSunParser *parser = new CSunParser();
	parser->LoadFile("units\\" + unitName + ".fbi");

	Unit u;
	u.fbi = CalculateCRC("units\\" + unitName + ".fbi");
	u.cob = CalculateCRC("scripts\\" + unitName + ".cob");

	//The model filenames has to be figured out from the fbi file
	string modelName = parser->SGetValueDef(unitName, "unitinfo\\Objectname");
	string deadName = parser->SGetValueDef(unitName + "_dead", "unitinfo\\Corpse");

	u.model = CalculateCRC("objects3d\\" + modelName + ".3do");
	u.model += CalculateCRC("objects3d\\" + deadName + ".3do");

	u.fullName = parser->SGetValueDef("unknown", "unitinfo\\Name");

	units[unitName] = u; 

	delete parser;

	//If we are done, map id numbers to names
	if (files.size() == 0) {
		MapUnitIds();
	}

	return (int)files.size();
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

void CSyncer::InstallClientDiff(string diff) 
{
	istringstream i(diff);

	int client;
	string name;
	bool wasRemove;
	i >> client;
	i >> wasRemove;

	//Handle remove if so
	if (wasRemove) {
		RemoveClient(client);
		return;
	}

	while (i >> name) {
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
	string unitName = unitIds[unit];
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