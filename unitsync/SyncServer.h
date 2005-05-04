#pragma once

#include "Syncer.h"
#include <string>
#include <set>

typedef map<string, Unit> unitlist_t;

struct MissingList {
	set<int> clients;
};

class CSyncServer : 
	public CSyncer
{
protected:
	map<int, unitlist_t> clientLists;
	int lastDiffClient;
	bool lastWasRemove;
	map<string, MissingList> curDiff;
	void InitMasterList();
public:
	CSyncServer(int id);
	~CSyncServer(void);
	void AddClient(int id, string unitList);
	void RemoveClient(int id);
	const string GetClientDiff(int id);
	virtual int ProcessUnits();
};
