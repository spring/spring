-- Caching results for Spring.* functions

--FIXME add GetVisibleFeatures

-- *etTeamColor
local teamColor = {}

-- GetVisibleUnits
local visibleUnits = {}

-- original functions
local GetTeamColor = Spring.GetTeamColor
local SetTeamColor = Spring.SetTeamColor
local GetVisibleUnits = Spring.GetVisibleUnits

function Spring.GetTeamColor(teamid)
  if teamColor[teamid] then
  else
    teamColor[teamid] = { GetTeamColor(teamid) }
  end
  return unpack(teamColor[teamid])
end

function Spring.SetTeamColor(teamid, r, g, b)
  -- set and cache
  SetTeamColor(teamid, r, g, b)
  teamColor[teamid] = { GetTeamColor(teamid) }
end

function Spring.GetVisibleUnits(teamID, radius, Icons)
  --local index = tostring(teamID)..":"..tostring(radius)..":"..tostring(Icons)
  local t = {}
  if teamID then t[#t+1] = teamID end
  if radius then t[#t+1] = radius end
  -- concat wants a table where all elements are strings or numbers
  if Icons then t[#t+1] = 1 end
  local index = table.concat(t, ":")
  local ret
  local update = false
  if visibleUnits[index] then
    local visible = visibleUnits[index]
    -- check time
    local now = Spring.GetTimer()
    local diff = Spring.DiffTimers(now, visible.time)
    if diff > 1/25 then
      visible.time = now
      update = true
    else
      return visible.units
    end
  else
    update = true
  end

  if update then
    ret = GetVisibleUnits(teamID, radius, Icons)
    visibleUnits[index] = {}
    visibleUnits[index].units = ret
    visibleUnits[index].time = Spring.GetTimer()
  end
  return ret
end
