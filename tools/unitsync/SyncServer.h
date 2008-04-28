#ifndef SYNCSERVER_H
#define SYNCSERVER_H

#include "Syncer.h"
#include <string>
#include <set>

typedef std::map<std::string, Unit> unitlist_t;

struct MissingList {
    std::set<int> clients;
};

class CSyncServer : 
	public CSyncer
{
protected:
    std::map<int, unitlist_t> clientLists;
	int lastDiffClient;
	bool lastWasRemove;
	std::map<std::string, MissingList> curDiff;
	void InitMasterList();
public:
	CSyncServer(int id);
	~CSyncServer(void);
	void AddClient(int id, const std::string& unitList);
	void RemoveClient(int id);
	const std::string GetClientDiff(int id);
	virtual int ProcessUnits();
};

#endif
