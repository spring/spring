function widget:GetInfo()
  return {
    name      = "TeamDiedMessages",
    desc      = "Prints a message when a Team died",
    author    = "abma",
    date      = "Sep, 2013",
    license   = "GNU GPL, v2 or later",
    layer     = 5,
    enabled   = true  --  loaded by default?
  }
end

local sec = "game_messages.lua" -- log section
local msgsfile = "gamedata/messages.tdf"

local luaMsgs = {}

function widget:Initialize()
	local TDF = VFS.Include('gamedata/parse_tdf.lua')
	if (not VFS.FileExists(msgsfile)) then
		Spring.Log(sec, LOG.INFO, "gamedata/messages.tdf doesn't exist.")
		return
	end

	local tdfMsgs, err = TDF.Parse(msgsfile)
	if (tdfMsgs == nil) then
		Spring.Log(sec, LOG.ERROR, 'Error parsing '.. msgsfile  .. err)
	end
	if (type(tdfMsgs.messages) ~= 'table') then
		Spring.Log(sec, LOG.ERROR, 'Missing "messages" section in ' .. msgsfile )
	end


	for label, msgs in pairs(tdfMsgs.messages) do
		if ((type(label) == 'string') and (type(msgs)  == 'table')) then
				local msgArray = {}
				for _, msg in pairs(msgs) do
					if (type(msg) == 'string') then
						table.insert(msgArray, msg)
					end
				end
			end
		luaMsgs[label:lower()] = msgArray -- lower case keys
	end
end

local function tr(msg)
	msg = msg:lower()
	if luaMsgs and type(luaMsgs[msg]) == "table" then
		local msgs = luaMsgs[msg]
		return msgs[ math.random( #msgs ) ]
	end
	Spring.Log(sec, LOG.DEBUG, string.format("Missing translation for %s", msg))
	return msg

end

function widget:TeamDied(TeamID)
	local teamID, playerid = Spring.GetTeamInfo(TeamID)
	if playerid then
		local name = Spring.GetPlayerInfo(playerid)
		msg = string.format(tr("Team %i (lead by %s) is no more"), TeamID, name )
	else
		msg = string.format(tr("Team %i is no more"), TeamID)
	end
	Spring.Echo(msg)
end

