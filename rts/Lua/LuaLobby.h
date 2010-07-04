/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUALOBBY_H
#define LUALOBBY_H

#include "lib/lobby/Connection.h"

struct lua_State;
struct LuaHashString;


class LuaLobby : public Connection
{
	LuaLobby(lua_State* L);
	~LuaLobby();

public:
	static bool PushEntries(lua_State* L);
	static bool CreateMetatable(lua_State* L);

private: // metatable
	static const void* metatable;

	static int meta_gc(lua_State* L);
	static int meta_index(lua_State* L);
	static int meta_newindex(lua_State* L);

private:
	lua_State* L;
	int luaRef; // saves userdata
	int luaRefEvents; // saves table with callin lua functions

	bool PushCallIn(const LuaHashString& name);

private:
	static int CreateLobby(lua_State* L);

private:
	//! we are overloading them, so we need to redefine them to make them usable (c++ disallows overloading with class-inheritance)
	using Connection::Poll;
	using Connection::Connect;
	using Connection::Disconnect;
	using Connection::Register;
	using Connection::Login;
	using Connection::ConfirmAggreement;
	using Connection::Rename;
	using Connection::ChangePass;
	using Connection::StatusUpdate;
	using Connection::Channels;
	using Connection::RequestMutelist;
	using Connection::JoinChannel;
	using Connection::LeaveChannel;
	using Connection::KickChannelMember;
	using Connection::ChangeTopic;
	using Connection::Say;
	using Connection::SayEx;
	using Connection::SayPrivate;

private: // call-outs
	static int Poll(lua_State *L);
	static int Connect(lua_State *L);
	static int Disconnect(lua_State *L);
	static int Register(lua_State *L);
	static int Login(lua_State *L);
	static int ConfirmAggreement(lua_State *L);
	static int Rename(lua_State *L);
	static int ChangePass(lua_State *L);
	static int StatusUpdate(lua_State *L);
	static int Channels(lua_State *L);
	static int RequestMutelist(lua_State *L);
	static int JoinChannel(lua_State *L);
	static int LeaveChannel(lua_State *L);
	static int KickChannelMember(lua_State *L);
	static int ChangeTopic(lua_State *L);
	static int Say(lua_State *L);
	static int SayEx(lua_State *L);
	static int SayPrivate(lua_State *L);

private: // call-ins
	virtual void DoneConnecting(bool success, const std::string& err);
	virtual void ServerGreeting(const std::string& serverVer, const std::string& springVer, int udpport, int mode);

	virtual void RegisterDenied(const std::string& reason);
	virtual void RegisterAccepted();

	virtual void LoginDenied(const std::string& reason);
	virtual void LoginEnd();
	virtual void Aggreement(const std::string& text);

	virtual void Motd(const std::string& text);
	virtual void ServerMessage(const std::string& text);
	virtual void ServerMessageBox(const std::string& text, const std::string& url);

	virtual void AddUser(const std::string& name, const std::string& country, int cpu);
	virtual void RemoveUser(const std::string& name);
	virtual void UserStatusUpdate(const std::string& name, ClientStatus status);

	virtual void ChannelInfo(const std::string& channel, unsigned users);
	virtual void ChannelInfoEnd();

	virtual void Mutelist(const std::string& channel, std::list<std::string> list);

	virtual void Joined(const std::string& channame);
	virtual void ChannelMember(const std::string& channame, const std::string& name, bool joined);
	virtual void ChannelMemberLeft(const std::string& channame, const std::string& name, const std::string& reason);
	virtual void JoinFailed(const std::string& channame, const std::string& reason);

	virtual void ChannelMemberKicked(const std::string& channame, const std::string& user, const std::string& reason);

	virtual void ChannelTopic(const std::string& channame, const std::string& author, long unsigned time, const std::string& topic);
	virtual void ChannelMessage(const std::string& channel, const std::string& text);

	virtual void Said(const std::string& channel, const std::string& user, const std::string& text);
	virtual void SaidEx(const std::string& channel, const std::string& user, const std::string& text);
	virtual void SaidPrivate(const std::string& user, const std::string& text);

	virtual void Disconnected();
	virtual void NetworkError(const std::string& msg);
};


#endif
