do
	return false -- don't show this widget in the userlist by default
end

function widget:GetInfo()
	return {
		name      = "Ingame lobby",
		desc      = "stuff",
		author    = "Auswaschbar (updated by jK)",
		date      = "",
		license   = "GNU GPL v3",
		layer     = 0,
		enabled   = false
	}
end

local username = "foo"
local password = "bar"

local Echo = Spring.Echo
local LuaLobby = Script.CreateLobby()
local curChannel

function LuaLobby.DoneConnecting(success, errMsg)
	if (success) then
		Echo("Connection completed");
		LuaLobby:Login(username, password)
	else
		Echo("Connection error: " .. errMsg);
	end
end

function LuaLobby.LoginEnd()
	Echo("Logged in to server");
end

function LuaLobby.LoginDenied(errMsg)
	Echo("Cannot login: " .. errMsg);
end

function LuaLobby.Joined(channel)
	if (curChannel) then
		LuaLobby:LeaveChannel(curChannel);
	end
	curChannel = channel
	Echo("Joined channel: " .. channel);
end

function LuaLobby.ChannelTopic(channel, author, time, topic)
	Echo(channel .. " topic: " .. topic);
	Echo("Set by " .. author .. " in " .. time)
end

function LuaLobby.Said(channel, user, message)
	Echo("["..channel.."]<"..user..">: " .. message);
end

function LuaLobby.SaidEx(channel, user, message)
	Echo("["..channel.."] "..user.." " .. message);
end

function LuaLobby.NetworkError(errMsg)
	Echo("Network error: " .. errMsg);
end

function LuaLobby.ChannelInfo(channel, num_users)
	Echo("["..channel.."] "..num_users);
end

function widget:Initialize()
	LuaLobby:Connect("taspringmaster.clan-sy.com", 8200);
end

function widget:Update()
	LuaLobby:Poll()
end

function widget:TextCommand(command)
	if (string.find(command, 'lsayex') == 1) then
		LuaLobby:SayEx(curChannel, command:sub(8))
	elseif (string.find(command, 'lsay') == 1) then
		LuaLobby:Say(curChannel, command:sub(6))
	elseif (string.find(command, 'ljoin') == 1) then
		LuaLobby:JoinChannel(command:sub(7))
	elseif (string.find(command, 'lchan') == 1) then
		LuaLobby:Channels()
	end
end

function widget:Shutdown()
end
