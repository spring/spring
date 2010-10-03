/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef _MSC_VER
#include "StdAfx.h"
#endif
#include "LuaLobby.h"
#ifndef _MSC_VER
#include "StdAfx.h"
#endif

#include "Game/UI/LuaUI.h"
#include "LuaCallInCheck.h"
#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaUtils.h"


#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)


inline LuaLobby* toLuaLobby(lua_State* L, int idx)
{
	LuaLobby** lob = (LuaLobby**)luaL_checkudata(L, idx, "LuaLobby");
	if (*lob == NULL) {
		luaL_error(L, "Attempt to use a deleted LuaLobby object!");
	}
	return *lob;
}


/******************************************************************************/
/******************************************************************************/

LuaLobby::LuaLobby(lua_State* _L) : L(_L)
{
	if (L != luaUI->L) {
		luaL_error(L, "Tried to create a LuaLobby object in a non-LuaUI enviroment!");
	}
}

LuaLobby::~LuaLobby()
{
}

/******************************************************************************/
/******************************************************************************/

bool LuaLobby::PushEntries(lua_State* L)
{
	CreateMetatable(L);

	REGISTER_LUA_CFUNC(CreateLobby);

	return true;
}

bool LuaLobby::CreateMetatable(lua_State* L)
{
	luaL_newmetatable(L, "LuaLobby");
	HSTR_PUSH_CFUNC(L, "__gc",        meta_gc);
	HSTR_PUSH_CFUNC(L, "__index",     meta_index);
	HSTR_PUSH_CFUNC(L, "__newindex",     meta_newindex);
	LuaPushNamedString(L, "__metatable", "protected metatable");

	REGISTER_LUA_CFUNC(Poll);
	REGISTER_LUA_CFUNC(Connect);
	REGISTER_LUA_CFUNC(Disconnect);
	REGISTER_LUA_CFUNC(Register);
	REGISTER_LUA_CFUNC(Login);
	REGISTER_LUA_CFUNC(ConfirmAggreement);
	REGISTER_LUA_CFUNC(Rename);
	REGISTER_LUA_CFUNC(ChangePass);
	REGISTER_LUA_CFUNC(StatusUpdate);
	REGISTER_LUA_CFUNC(Channels);
	REGISTER_LUA_CFUNC(RequestMutelist);
	REGISTER_LUA_CFUNC(JoinChannel);
	REGISTER_LUA_CFUNC(LeaveChannel);
	REGISTER_LUA_CFUNC(KickChannelMember);
	REGISTER_LUA_CFUNC(ChangeTopic);
	REGISTER_LUA_CFUNC(Say);
	REGISTER_LUA_CFUNC(SayEx);
	REGISTER_LUA_CFUNC(SayPrivate);

	lua_pop(L, 1);
	return true;
}

/******************************************************************************/
/******************************************************************************/

int LuaLobby::CreateLobby(lua_State* L)
{
	LuaLobby* lob = new LuaLobby(L);

	LuaLobby** lobPtr = (LuaLobby**)lua_newuserdata(L, sizeof(LuaLobby*));
	*lobPtr = lob;

	luaL_getmetatable(L, "LuaLobby");
	lua_setmetatable(L, -2);

	lua_newtable(L);
	lob->luaRefEvents = luaL_ref(L, LUA_REGISTRYINDEX);
	if (lob->luaRefEvents == LUA_NOREF) {
		lua_pop(L, 1);
		*lobPtr = NULL;
		delete lob;
		return 0;
	}

	lua_pushvalue(L, 1);
	lob->luaRef = luaL_ref(L, LUA_REGISTRYINDEX);
	if (lob->luaRef == LUA_NOREF) {
		lua_pop(L, 1);
		*lobPtr = NULL;
		delete lob;
		return 0;
	}

	return 1;
}

/******************************************************************************/
/******************************************************************************/

int LuaLobby::meta_gc(lua_State* L)
{
	//FIXME: this might never get called because we hold a link to the userdata object in LUA_REGISTRYINDEX

	LuaLobby* lob = toLuaLobby(L, 1);

	luaL_unref(L, LUA_REGISTRYINDEX, lob->luaRef);
	luaL_unref(L, LUA_REGISTRYINDEX, lob->luaRefEvents);

	delete lob;
	return 0;
}

int LuaLobby::meta_index(lua_State* L)
{
	//! check if there is a function
	luaL_getmetatable(L, "LuaLobby");
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	if (!lua_isnil(L, -1)) {
		return 1;
	}
	lua_pop(L, 1);

	LuaLobby* lob = toLuaLobby(L, 1);
	lua_rawgeti(L, LUA_REGISTRYINDEX, lob->luaRefEvents);
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	if (!lua_isnil(L, -1)) {
		return 1;
	}
	lua_pop(L, 1);

	return 0;
}

int LuaLobby::meta_newindex(lua_State* L)
{
	//if (!lua_isfunction(L, 3)) {
	//	luaL_error(L, "tried to set non-function!");
	//}

	LuaLobby* lob = toLuaLobby(L, 1);
	lua_rawgeti(L, LUA_REGISTRYINDEX, lob->luaRefEvents);
	lua_pushvalue(L, 2); // name
	lua_pushvalue(L, 3); // function
	lua_rawset(L, -3);

	return 0;
}

/******************************************************************************/
/******************************************************************************/

inline bool LuaLobby::PushCallIn(const LuaHashString& name)
{
	// get callin lua function
	lua_rawgeti(L, LUA_REGISTRYINDEX, luaRefEvents);
	name.Push(L);
	lua_rawget(L, -2);

	// check if the function is valid/set
	if (lua_isfunction(L, -1)) {
		return true;
	}
	lua_pop(L, 1);
	return false;
}

/******************************************************************************/
/******************************************************************************/

int LuaLobby::Poll(lua_State *L)
{
	LuaLobby* lob = toLuaLobby(L, 1);
	lob->Poll();
	return 0;
}

int LuaLobby::Connect(lua_State *L)
{
	LuaLobby* lob = toLuaLobby(L, 1);
	std::string host(luaL_checkstring(L, 2));
	int port = luaL_checknumber(L, 3);
	lob->Connect(host, port);
	return 0;
}

int LuaLobby::Disconnect(lua_State *L)
{
	LuaLobby* lob = toLuaLobby(L, 1);
	lob->Disconnect();
	return 0;
}

int LuaLobby::Register(lua_State *L)
{
	LuaLobby* lob = toLuaLobby(L, 1);
	std::string user(luaL_checkstring(L, 2));
	std::string pass(luaL_checkstring(L, 3));
	lob->Register(user, pass);
	return 0;
}

int LuaLobby::Login(lua_State *L)
{
	LuaLobby* lob = toLuaLobby(L, 1);
	std::string user(luaL_checkstring(L, 2));
	std::string pass(luaL_checkstring(L, 3));
	lob->Login(user, pass);
	return 0;
}

int LuaLobby::ConfirmAggreement(lua_State *L)
{
	LuaLobby* lob = toLuaLobby(L, 1);
	lob->ConfirmAggreement();
	return 0;
}

int LuaLobby::Rename(lua_State *L)
{
	LuaLobby* lob = toLuaLobby(L, 1);
	std::string newname(luaL_checkstring(L, 2));
	lob->Rename(newname);
	return 0;
}

int LuaLobby::ChangePass(lua_State *L)
{
	LuaLobby* lob = toLuaLobby(L, 1);
	std::string oldpass(luaL_checkstring(L, 2));
	std::string newpass(luaL_checkstring(L, 3));
	lob->ChangePass(oldpass, newpass);
	return 0;
}

int LuaLobby::StatusUpdate(lua_State *L)
{
	LuaLobby* lob = toLuaLobby(L, 1);
	bool ingame = lua_toboolean(L, 2);
	bool away = lua_toboolean(L, 3);
	lob->StatusUpdate(ingame, away);
	return 0;
}

int LuaLobby::Channels(lua_State *L)
{
	LuaLobby* lob = toLuaLobby(L, 1);
	lob->Channels();
	return 0;
}

int LuaLobby::RequestMutelist(lua_State *L)
{
	LuaLobby* lob = toLuaLobby(L, 1);
	std::string channame(luaL_checkstring(L, 2));
	lob->RequestMutelist(channame);
	return 0;
}

int LuaLobby::JoinChannel(lua_State *L)
{
	LuaLobby* lob = toLuaLobby(L, 1);
	std::string channame(luaL_checkstring(L, 2));
	std::string passwd = "";
	if (lua_isstring(L, 3))
		passwd = luaL_checkstring(L, 3);
	lob->JoinChannel(channame, passwd);
	return 0;
}

int LuaLobby::LeaveChannel(lua_State *L)
{
	LuaLobby* lob = toLuaLobby(L, 1);
	std::string channame(luaL_checkstring(L, 2));
	lob->LeaveChannel(channame);
	return 0;
}

int LuaLobby::ChangeTopic(lua_State *L)
{
	LuaLobby* lob = toLuaLobby(L, 1);
	std::string channame(luaL_checkstring(L, 2));
	std::string topic(luaL_checkstring(L, 3));
	lob->ChangeTopic(channame, topic);
	return 0;
}

int LuaLobby::Say(lua_State *L)
{
	LuaLobby* lob = toLuaLobby(L, 1);
	std::string channame(luaL_checkstring(L, 2));
	std::string message(luaL_checkstring(L, 3));
	lob->Say(channame, message);
	return 0;
}

int LuaLobby::SayEx(lua_State *L)
{
	LuaLobby* lob = toLuaLobby(L, 1);
	std::string channame(luaL_checkstring(L, 2));
	std::string message(luaL_checkstring(L, 3));
	lob->SayEx(channame, message);
	return 0;
}

int LuaLobby::SayPrivate(lua_State *L)
{
	LuaLobby* lob = toLuaLobby(L, 1);
	std::string username(luaL_checkstring(L, 2));
	std::string message(luaL_checkstring(L, 3));
	lob->SayPrivate(username, message);
	return 0;
}

int LuaLobby::KickChannelMember(lua_State *L)
{
	LuaLobby* lob = toLuaLobby(L, 1);
	std::string channame(luaL_checkstring(L, 2));
	std::string user(luaL_checkstring(L, 3));
	std::string reason(luaL_checkstring(L, 4));
	lob->KickChannelMember(channame, user, reason);
	return 0;
}

/******************************************************************************/
/******************************************************************************/

void LuaLobby::DoneConnecting(bool succes, const std::string& err)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushboolean(L, succes);
	lua_pushstring(L, err.c_str());

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 2, 0);
}

void LuaLobby::ServerGreeting(const std::string& serverVer, const std::string& springVer, int udpport, int mode)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 6);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, serverVer.c_str());
	lua_pushstring(L, springVer.c_str());
	lua_pushnumber(L, udpport);
	lua_pushnumber(L, mode);

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 4, 0);
}

void LuaLobby::RegisterDenied(const std::string& reason)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, reason.c_str());

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 1, 0);
}

void LuaLobby::RegisterAccepted()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 0, 0);
}

void LuaLobby::LoginDenied(const std::string& reason)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, reason.c_str());

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 1, 0);
}

void LuaLobby::LoginEnd()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 0, 0);
}

void LuaLobby::Aggreement(const std::string& text)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, text.c_str());

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 1, 0);
}

void LuaLobby::Motd(const std::string& text)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, text.c_str());

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 1, 0);
}

void LuaLobby::ServerMessage(const std::string& text)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, text.c_str());

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 1, 0);
}

void LuaLobby::ServerMessageBox(const std::string& text, const std::string& url)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, text.c_str());
	lua_pushstring(L, url.c_str());

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 2, 0);
}

void LuaLobby::AddUser(const std::string& name, const std::string& country, int cpu)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, name.c_str());
	lua_pushstring(L, country.c_str());
	lua_pushnumber(L, cpu);

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 3, 0);
}

void LuaLobby::RemoveUser(const std::string& name)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, name.c_str());

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 1, 0);
}

void LuaLobby::UserStatusUpdate(const std::string& name, ClientStatus status)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 8);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, name.c_str());
	lua_pushboolean(L, status.away);
	lua_pushboolean(L, status.bot);
	lua_pushboolean(L, status.ingame);
	lua_pushboolean(L, status.moderator);
	lua_pushnumber(L, status.rank);

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 6, 0);
}

void LuaLobby::ChannelInfo(const std::string& channel, unsigned users)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, channel.c_str());
	lua_pushnumber(L, users);

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 2, 0);
}

void LuaLobby::ChannelInfoEnd()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 0, 0);
}

void LuaLobby::Mutelist(const std::string& channel, std::list<std::string> list)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, channel.c_str());
	lua_newtable(L);
	int i = 1;
	for (std::list<std::string>::const_iterator it = list.begin(); it != list.end(); ++it)
	{
		lua_pushstring(L, it->c_str());
		lua_rawseti(L, -2, i++);
	}

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 2, 0);
}

void LuaLobby::Joined(const std::string& channame)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, channame.c_str());

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 1, 0);
}

void LuaLobby::ChannelMember(const std::string& channame, const std::string& name, bool joined)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, channame.c_str());
	lua_pushstring(L, name.c_str());
	lua_pushboolean(L, joined);

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 3, 0);
}

void LuaLobby::ChannelMemberLeft(const std::string& channame, const std::string& name, const std::string& reason)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, channame.c_str());
	lua_pushstring(L, name.c_str());
	lua_pushstring(L, reason.c_str());

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 3, 0);
}

void LuaLobby::JoinFailed(const std::string& channame, const std::string& reason)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, channame.c_str());
	lua_pushstring(L, reason.c_str());

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 2, 0);
}

void LuaLobby::ChannelMemberKicked(const std::string& channame, const std::string& user, const std::string& reason)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, channame.c_str());
	lua_pushstring(L, user.c_str());
	lua_pushstring(L, reason.c_str());

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 3, 0);
}

void LuaLobby::ChannelTopic(const std::string& channame, const std::string& author, long unsigned time, const std::string& topic)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 6);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, channame.c_str());
	lua_pushstring(L, author.c_str());
	lua_pushnumber(L, time/1000);
	lua_pushstring(L, topic.c_str());

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 4, 0);
}

void LuaLobby::ChannelMessage(const std::string& channel, const std::string& text)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, channel.c_str());
	lua_pushstring(L, text.c_str());

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 2, 0);
}

void LuaLobby::Said(const std::string& channel, const std::string& user, const std::string& text)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, channel.c_str());
	lua_pushstring(L, user.c_str());
	lua_pushstring(L, text.c_str());

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 3, 0);
}

void LuaLobby::SaidEx(const std::string& channel, const std::string& user, const std::string& text)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 5);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, channel.c_str());
	lua_pushstring(L, user.c_str());
	lua_pushstring(L, text.c_str());

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 3, 0);
}

void LuaLobby::SaidPrivate(const std::string& user, const std::string& text)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 4);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, user.c_str());
	lua_pushstring(L, text.c_str());

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 2, 0);
}

void LuaLobby::Disconnected()
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 2);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 0, 0);
}

void LuaLobby::NetworkError(const std::string& msg)
{
	LUA_CALL_IN_CHECK(L);
	lua_checkstack(L, 3);
	static const LuaHashString cmdStr(__FUNCTION__);
	if (!PushCallIn(cmdStr)) {
		return;
	}

	lua_pushstring(L, msg.c_str());

	// call the routine
	luaUI->RunCallInUnsynced(cmdStr, 1, 0);
}

/******************************************************************************/
/******************************************************************************/
