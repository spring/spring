--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    teamcolors.lua
--  brief:   local team recoloring
--  author:  Dave Rodgers
--
--  Copyright (C) 2008.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Configuration
--

local disabled = true       --<< *** NOTE *** >>--   

local debug = true and false

local print = Spring.Echo

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Color sets
--

local myColor = { 0.0, 1.0, 1.0 } -- cyan

local gaiaColor = { 0.8, 0.8, 0.8 } -- off-white

local allyColors = { -- greens
  { 0.0, 1.0, 0.0 },
  { 0.0, 1.0, 0.4 },
  { 0.0, 1.0, 0.8 },
  { 0.0, 0.7, 0.0 },
  { 0.0, 0.7, 0.3 },
  { 0.0, 0.7, 0.6 },
  { 0.0, 0.4, 0.0 },
  { 0.0, 0.4, 0.2 },
  { 0.0, 0.4, 0.4 },
}

local enemyColors = { -- reds
  { 1.0, 0.0, 0.0 },
  { 1.0, 0.0, 0.4 },
  { 1.0, 0.0, 0.8 },
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
  elseif (teamID == myTeam) then
    teamColors[teamID] = myColor
  elseif (team.allyTeam == myAllyTeam) then
    a = (a % #allyColors) + 1
    teamColors[teamID] = allyColors[a]
  else
    e = (e % #enemyColors) + 1
    teamColors[teamID] = enemyColors[e]
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
  