--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    teamcolors.lua
--  brief:   local team recoloring
--  author:  Dave Rodgers
--
--  notes:
--    This script can be used to configure the local
--    client's team colors before the game starts.
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Configuration
--

local disabled = true       --<< *** NOTE *** >>--   

local debug = false


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Color sets
--

local myColor = { 0, 1, 1 }

local gaiaColor = { 0.8, 0.8, 0.8 }

local allyColors = {
  { 0.0, 1.0, 0.0 },
  { 0.0, 1.0, 0.5 },
  { 0.0, 1.0, 1.0 },
  { 0.0, 0.7, 0.0 },
  { 0.0, 0.7, 0.4 },
  { 0.0, 0.7, 0.8 },
  { 0.0, 0.4, 0.0 },
  { 0.0, 0.4, 0.3 },
  { 0.0, 0.4, 0.6 },
}

local enemyColors = {
  { 1.0, 0.0, 0.0 },
  { 1.0, 0.0, 0.5 },
  { 1.0, 0.0, 1.0 },
  { 0.7, 0.0, 0.0 },
  { 0.7, 0.0, 0.4 },
  { 0.7, 0.0, 0.8 },
  { 0.4, 0.0, 0.0 },
  { 0.4, 0.0, 0.3 },
  { 0.4, 0.0, 0.6 },
}


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local myTeam = players[myPlayer].team

local myAllyTeam = teams[myTeam].allyTeam


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local teamColors = {}


-- set the colors
local a, e = 0, 0
for teamID, team in pairs(teams) do
  if (team.gaia) then
    teamColors[teamID] = gaiaColor
  elseif (team.active) then
    if (teamID == myTeam) then
      teamColors[teamID] = myColor
    elseif (team.allyTeam == myAllyTeam) then
      a = (a % #allyColors) + 1
      teamColors[teamID] = allyColors[a]
    else
      e = (e % #enemyColors) + 1
      teamColors[teamID] = enemyColors[e]
    end
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (debug) then
  print('TEAMCOLORS.LUA')

  print('  modName = '      .. modName);
  print('  modShortName = ' .. modShortName);
  print('  modVersion = '   .. modVersion);  

  print('  mapName = '      .. mapName);
  print('  mapHumanName = ' .. mapHumanName);
            
  print('  myPlayer   = ' .. myPlayer)
  print('  myTeam     = ' .. myTeam)
  print('  myAllyTeam = ' .. myAllyTeam)

  for teamID, team in pairs(teams) do
    local color = team.color
    print('  team ' .. teamID)
    print('    leader   = ' .. team.leader)
    print('    allyTeam = ' .. team.allyTeam)
    print('    gaia     = ' .. tostring(team.gaia))
    print('    active   = ' .. tostring(team.active))
    print('    color    = ' .. color[1]..' '..color[2]..' '..color[3])
  end

  for playerID, player in pairs(players) do
    if (player.active) then
      print('  player ' .. playerID)
      print('    name     = ' .. player.name)
      print('    team     = ' .. player.team)
      print('    active   = ' .. tostring(player.active))
      print('    spec     = ' .. tostring(player.spectating))
    end
  end

  for teamID, color in pairs(teamColors) do
    print(string.format('teamcolors.lua: %i = %f, %f, %f', teamID,
                        color[1], color[2], color[3]))
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

if (disabled) then
  return {}
else
  return teamColors
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
  