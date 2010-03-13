#include "LuaLobby.h"

#include "LogOutput.h"

CLogSubsystem LobbyLog("LuaLobby");

const char LuaLobby::className[] = "LuaLobby";
Lunar<LuaLobby>::RegType LuaLobby::methods[] =
{
	{ "Connect", &LuaLobby::Connect },
	{ "Login", &LuaLobby::Login },
	{ "JoinChannel", &LuaLobby::JoinChannel },
	{ "LeaveChannel", &LuaLobby::LeaveChannel },
	{ "Say", &LuaLobby::Say },
	{ "SayEx", &LuaLobby::SayEx },
	{ "Poll", &LuaLobby::Poll },
	{ 0 }
};

LuaLobby::LuaLobby(lua_State *_L) : L(_L)
{
}

LuaLobby::~LuaLobby()
{
}

int LuaLobby::Poll(lua_State *L)
{
	Connection::Poll();
	return 1;
}

int LuaLobby::Connect(lua_State *L)
{
	std::string host(luaL_checkstring(L, 1));
	int port = luaL_checknumber(L, 2);
	Connection::Connect(host, port);
	return 1;
}

void LuaLobby::DoneConnecting(bool succes, const std::string& err)
{
	Lunar<LuaLobby>::push(L, this);
	lua_pushboolean(L, succes);
	lua_pushstring(L, err.c_str());
	const int ret = Lunar<LuaLobby>::call(L, "DoneConnecting", 2, 0);
	if (ret < 0)
		LogObject(LobbyLog) << "Error: " << luaL_checkstring(L, -1);
}

int LuaLobby::Register(lua_State *L)
{
	std::string user(luaL_checkstring(L, 1));
	std::string pass(luaL_checkstring(L, 2));
	Connection::Register(user, pass);
	return 1;
}

void LuaLobby::RegisterDenied(const std::string& reason)
{
	Lunar<LuaLobby>::push(L, this);
	lua_pushstring(L, reason.c_str());
	const int ret = Lunar<LuaLobby>::call(L, "RegisterDenied", 1, 0);
	if (ret < 0)
		LogObject(LobbyLog) << "Error: " << luaL_checkstring(L, -1);
}

void LuaLobby::RegisterAccept()
{
	Lunar<LuaLobby>::push(L, this);
	const int ret = Lunar<LuaLobby>::call(L, "RegisterAccept", 0, 0);
	if (ret < 0)
		LogObject(LobbyLog) << "Error: " << luaL_checkstring(L, -1);
}

int LuaLobby::Login(lua_State *L)
{
	std::string user(luaL_checkstring(L, 1));
	std::string pass(luaL_checkstring(L, 2));
	Connection::Login(user, pass);
	return 1;
}

void LuaLobby::Denied(const std::string& reason)
{
	Lunar<LuaLobby>::push(L, this);
	lua_pushstring(L, reason.c_str());
	const int ret = Lunar<LuaLobby>::call(L, "Denied", 1, 0);
	if (ret < 0)
		LogObject(LobbyLog) << "Error: " << luaL_checkstring(L, -1);
}

void LuaLobby::LoginEnd()
{
	Lunar<LuaLobby>::push(L, this);
	const int ret = Lunar<LuaLobby>::call(L, "LoginEnd", 0, 0);
	if (ret < 0)
		LogObject(LobbyLog) << "Error: " << luaL_checkstring(L, -1);
}

void LuaLobby::Aggreement(const std::string text)
{
	Lunar<LuaLobby>::push(L, this);
	lua_pushstring(L, text.c_str());
	const int ret = Lunar<LuaLobby>::call(L, "Aggreement", 1, 0);
	if (ret < 0)
		LogObject(LobbyLog) << "Error: " << luaL_checkstring(L, -1);
}

int LuaLobby::ConfirmAggreement(lua_State *L)
{
	Connection::ConfirmAggreement();
	return 1;
}

void LuaLobby::Motd(const std::string text)
{
	Lunar<LuaLobby>::push(L, this);
	lua_pushstring(L, text.c_str());
	const int ret = Lunar<LuaLobby>::call(L, "Motd", 1, 0);
	if (ret < 0)
		LogObject(LobbyLog) << "Error: " << luaL_checkstring(L, -1);
}

int LuaLobby::JoinChannel(lua_State *L)
{
	std::string channame(luaL_checkstring(L, 1));
	std::string passwd;
	if (lua_isstring(L, 2))
		passwd = luaL_checkstring(L, 2);
	Connection::JoinChannel(channame, passwd);
	return 1;
}

void LuaLobby::Joined(const std::string& channame)
{
	Lunar<LuaLobby>::push(L, this);
	lua_pushstring(L, channame.c_str());
	const int ret = Lunar<LuaLobby>::call(L, "Joined", 1, 0);
	if (ret < 0)
		LogObject(LobbyLog) << "Error: " << luaL_checkstring(L, -1);
}

void LuaLobby::JoinFailed(const std::string& channame, const std::string& reason)
{
	Lunar<LuaLobby>::push(L, this);
	lua_pushstring(L, channame.c_str());
	lua_pushstring(L, reason.c_str());
	const int ret = Lunar<LuaLobby>::call(L, "JoinFailed", 2, 0);
	if (ret < 0)
		LogObject(LobbyLog) << "Error: " << luaL_checkstring(L, -1);
}

int LuaLobby::LeaveChannel(lua_State *L)
{
	std::string channame(luaL_checkstring(L, 1));
	Connection::LeaveChannel(channame);
	return 1;
}

int LuaLobby::Say(lua_State *L)
{
	std::string channame(luaL_checkstring(L, 1));
	std::string message(luaL_checkstring(L, 2));
	Connection::Say(channame, message);
	return 1;
}

int LuaLobby::SayEx(lua_State *L)
{
	std::string channame(luaL_checkstring(L, 1));
	std::string message(luaL_checkstring(L, 2));
	Connection::SayEx(channame, message);
	return 1;
}

int LuaLobby::SayPrivate(lua_State *L)
{
	std::string username(luaL_checkstring(L, 1));
	std::string message(luaL_checkstring(L, 2));
	Connection::SayPrivate(username, message);
	return 1;
}

void LuaLobby::Said(const std::string& channel, const std::string& user, const std::string& text)
{
	Lunar<LuaLobby>::push(L, this);
	lua_pushstring(L, channel.c_str());
	lua_pushstring(L, user.c_str());
	lua_pushstring(L, text.c_str());
	const int ret = Lunar<LuaLobby>::call(L, "Said", 3, 0);
	if (ret < 0)
		LogObject(LobbyLog) << "Error: " << luaL_checkstring(L, -1);
}

void LuaLobby::SaidEx(const std::string& channel, const std::string& user, const std::string& text)
{
	Lunar<LuaLobby>::push(L, this);
	lua_pushstring(L, channel.c_str());
	lua_pushstring(L, user.c_str());
	lua_pushstring(L, text.c_str());
	const int ret = Lunar<LuaLobby>::call(L, "SaidEx", 3, 0);
	if (ret < 0)
		LogObject(LobbyLog) << "Error: " << luaL_checkstring(L, -1);
}

void LuaLobby::SaidPrivate(const std::string& user, const std::string& text)
{
	Lunar<LuaLobby>::push(L, this);
	lua_pushstring(L, user.c_str());
	lua_pushstring(L, text.c_str());
	const int ret = Lunar<LuaLobby>::call(L, "SaidPrivate", 2, 0);
	if (ret < 0)
		LogObject(LobbyLog) << "Error: " << luaL_checkstring(L, -1);
}

void LuaLobby::Disconnected()
{
	Lunar<LuaLobby>::push(L, this);
	const int ret = Lunar<LuaLobby>::call(L, "Disconnected", 0, 0);
	if (ret < 0)
		LogObject(LobbyLog) << "Error: " << luaL_checkstring(L, -1);
}

void LuaLobby::NetworkError(const std::string& msg)
{
	Lunar<LuaLobby>::push(L, this);
	lua_pushstring(L, msg.c_str());
	const int ret = Lunar<LuaLobby>::call(L, "NetworkError", 1, 0);
	if (ret < 0)
		LogObject(LobbyLog) << "Error: " << luaL_checkstring(L, -1);
}

bool LuaLobby::RegisterUserdata(lua_State *L)
{
	Lunar<LuaLobby>::Register(L);
	return true;
}
