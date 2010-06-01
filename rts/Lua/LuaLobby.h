/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUALOBBY_H
#define LUALOBBY_H

#include "lib/lobby/Connection.h"
#include "Lunar.h"

class LuaLobby : public Connection
{
public:
	LuaLobby(lua_State *L);
	~LuaLobby();

	// callables and callbacks (virtual)
	int Poll(lua_State *L);
	
	int Connect(lua_State *L);
	virtual void DoneConnecting(bool success, const std::string& err);
	virtual void ServerGreeting(const std::string& serverVer, const std::string& springVer, int udpport, int mode);

	int Register(lua_State *L);
	virtual void RegisterDenied(const std::string& reason);
	virtual void RegisterAccept();

	int Login(lua_State *L);
	virtual void Denied(const std::string& reason);
	virtual void LoginEnd();
	virtual void Aggreement(const std::string text);
	int ConfirmAggreement(lua_State *L);

	int Rename(lua_State *L);
	int ChangePass(lua_State *L);

	virtual void Motd(const std::string text);
	virtual void ServerMessage(const std::string& text);
	virtual void ServerMessageBox(const std::string& text, const std::string& url);

	virtual void AddUser(const std::string& name, const std::string& country, int cpu);
	virtual void RemoveUser(const std::string& name);
	int StatusUpdate(lua_State *L);
	virtual void ClientStatusUpdate(const std::string& name, ClientStatus status);

	int Channels(lua_State *L);
	virtual void ChannelInfo(const std::string& channel, unsigned users);
	virtual void ChannelInfoEnd();

	int RequestMutelist(lua_State *L);
	virtual void Mutelist(const std::string& channel, std::list<std::string> list);

	int JoinChannel(lua_State *L);
	virtual void Joined(const std::string& channame);
	virtual void ChannelMember(const std::string& channame, const std::string& name, bool joined);
	virtual void ChannelMemberLeft(const std::string& channame, const std::string& name, const std::string& reason);
	virtual void JoinFailed(const std::string& channame, const std::string& reason);
	int LeaveChannel(lua_State *L);
	int ForceLeaveChannel(lua_State *L);
	virtual void ForceLeftChannel(const std::string& channame, const std::string& user, const std::string& reason);

	int ChangeTopic(lua_State *L);
	virtual void ChannelTopic(const std::string& channame, const std::string& author, long unsigned time, const std::string& topic);
	virtual void ChannelMessage(const std::string& channel, const std::string& text);

	int Say(lua_State *L);
	int SayEx(lua_State *L);
	int SayPrivate(lua_State *L);
	virtual void Said(const std::string& channel, const std::string& user, const std::string& text);
	virtual void SaidEx(const std::string& channel, const std::string& user, const std::string& text);
	virtual void SaidPrivate(const std::string& user, const std::string& text);

	virtual void Disconnected();
	virtual void NetworkError(const std::string& msg);

	// inside stuff
	static bool RegisterUserdata(lua_State *L);

	static const char className[];
	static Lunar<LuaLobby>::RegType methods[];
	
private:
	lua_State *L;
};


#endif
