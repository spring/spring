/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOBBYCONNECTION_H
#define LOBBYCONNECTION_H

#include "lib/lobby/Connection.h"

class UpdaterWindow;

class LobbyConnection : public Connection
{
public:
	LobbyConnection();
	~LobbyConnection();

	void ConnectDialog(bool show);
	bool WantClose() const;

	virtual void DoneConnecting(bool success, const std::string& err);
	virtual void ServerGreeting(const std::string& serverVer, const std::string& springVer, int udpport, int mode);
	virtual void Denied(const std::string& reason);
	virtual void Aggreement(const std::string text);
	virtual void LoginEnd();
	virtual void RegisterDenied(const std::string& reason);
	virtual void RegisterAccept();
	
private:
	UpdaterWindow* upwin;
};

#endif
