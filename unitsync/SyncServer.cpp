#include "StdAfx.h"
#include ".\syncserver.h"
#include <sstream>

CSyncServer::CSyncServer(int id) :
	CSyncer(id)
{
}

CSyncServer::~CSyncServer(void)
{
}

void CSyncServer::AddClient(int id, string unitList)
{
	istringstream s(unitList);
	Unit u;
	string name;

	while (s >> name) {
		s >> u.fbi;
		s >> u.cob;
		s >> u.model;

		clientLists[id][name] = u;
	}

	//The time has come, to calculate diffs! ohnoes!
	curDiff.clear();
	lastDiffClient = id;
	lastWasRemove = false;

	for (map<string, Unit>::iterator i = clientLists[id].begin(); i != clientLists[id].end(); ++i) {

		//Check with each other client if they have this unit
		for (map<int, unitlist_t>::iterator clientId = clientLists.begin(); clientId != clientLists.end(); ++clientId) {
			
			//No need to compare with ourselves
			if (clientId->first == id)
				continue;

			//Now, determine if this unit is here, and if so, if it has the same crc
			map<string, Unit>::iterator curUnit = clientLists[clientId->first].find(i->first);
			bool unitOk = false;
			if (curUnit != clientLists[clientId->first].end()) {
				if ((curUnit->second.fbi == i->second.fbi) &&
					(curUnit->second.cob == i->second.cob) &&
					(curUnit->second.model == i->second.model))
				{
					unitOk = true;
				}
			}

			if (unitOk) {
				//No need to say anything about this unit
			}
			else {
				map<string, MissingList>::iterator mli = curDiff.find(i->first);
				if (mli != curDiff.end()) {
					curDiff[i->first].clients.insert(clientId->first);
				}
				else {
					MissingList ml;
					ml.clients.insert(clientId->first);
					curDiff[i->first] = ml;
				}
			}
		}
	}

	//Now we need to check the other way around.. If any client had a unit that we did not
	//if we find such a unit, it must be added to the difflist for that client
	//this could possibly be optimized by flagging everything that we do find in the above pass..
	for (map<int, unitlist_t>::iterator client = clientLists.begin(); client != clientLists.end(); ++client) {
		
		//No need to diff with ourselves
		if (client->first == id)
			continue;

		for (map<string, Unit>::iterator unit = client->second.begin(); unit != client->second.end(); ++unit) {
			map<string, Unit>::iterator found = clientLists[id].find(unit->first);

			//If not found, we should do things
			if (found == clientLists[id].end()) {
				map<string, MissingList>::iterator mli = curDiff.find(unit->first);
				if (mli != curDiff.end()) {
					curDiff[unit->first].clients.insert(client->first);
				}
				else {
					MissingList ml;
					ml.clients.insert(client->first);
					curDiff[unit->first] = ml;
				}
			}
		}
	}

	//Alright, now we have a map that for each unit contains id's of clients that need to know that
	//this unit should now be disabled
}

void CSyncServer::RemoveClient(int id)
{
	//Since each client keeps a list of who caused a unit to be disabled, it should suffice to let them
	//keep their lists in order if we just tell them who left

	lastDiffClient = id;
	lastWasRemove = true;
}

const string CSyncServer::GetClientDiff(int id)
{
	ostringstream s("");

	//Always say who is guilty
	s << lastDiffClient << " ";

	//and of what they are guilt
	s << lastWasRemove << " ";

	//If remove, we are done now
	if (lastWasRemove)
		return s.str();

	for (map<string, MissingList>::iterator i = curDiff.begin(); i != curDiff.end(); ++i) {
		set<int>::iterator client = i->second.clients.find(id);

		//If found, add the name of the disabled unit
		if (client != i->second.clients.end()) {
			s << i->first << " ";
		}
	}

	return s.str();
}

void CSyncServer::InitMasterList() 
{
	//Add our info as a clientList to avoid special handling

	for (map<string, Unit>::iterator i = units.begin(); i != units.end(); ++i) {
		clientLists[localId][i->first] = i->second;
	}
}

int CSyncServer::ProcessUnits()
{
	int ret = CSyncer::ProcessUnits();

	if (ret == 0) {
		InitMasterList();
	}

	return ret;
}