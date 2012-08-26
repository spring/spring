--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    download_builds.lua
--  brief:   downloaded unit buildOptions insertion
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local TDF = TDFparser or VFS.Include('gamedata/parse_tdf.lua')

local dlBuilds

local section='download_builds.lua'

--------------------------------------------------------------------------------

local function SafeLower(str)
  if (type(str) == 'string') then
    return string.lower(str)
  end
  return nil
end


local function Load()
  dlBuilds = {}
  local files = VFS.DirList('download/', '*.tdf')
  for i, f in ipairs(files) do
    local tdf, err = TDF.Parse(f)
    if (tdf == nil) then
      Spring.Log(section, LOG.ERROR, err)
    else
      for menuEntry, menuTable in pairs(tdf) do
        if (type(menuTable) == 'table') then
          local unitMenu = SafeLower(menuTable.unitmenu)
          
          local dlMenu = dlBuilds[unitMenu]
          if (dlMenu == nil) then
            dlMenu = {}
            dlBuilds[unitMenu] = dlMenu
          end
          local unitName   = SafeLower(menuTable.unitname)
          local afterName  = SafeLower(menuTable.aftername)
          local beforeName = SafeLower(menuTable.beforename)
          local menu       = tonumber(menuTable.menu)
          local button     = tonumber(menuTable.button)
          if (unitName) then
            table.insert(dlMenu, {
              unitName   = unitName,
              afterName  = afterName,
              beforeName = beforeName,
              menu       = menu,
              button     = button,
            })
          end
          local dlMenu = dlBuilds[unitMenu] or {}
        end
      end
    end
  end
end


--------------------------------------------------------------------------------

local function FindNameIndex(unitName, buildOptions)
  for i, name in ipairs(buildOptions) do
    if (unitName == name) then
      return i
    end
  end
  return nil
end


local function LinearArray(t)
  if (t == nil) then
    return nil
  end
  local sorted = {}
  for k,v in pairs(t) do
    if (type(k) == 'number') then
      table.insert(sorted, { k, v })
    end
  end
  table.sort(sorted, function(a, b) return a[1] < b[1] end)
  local array = {}
  for i, kv in ipairs(sorted) do
    array[i] = kv[2]
  end
  return array
end


local function Execute(unitDefs)
  if (dlBuilds == nil) then
    Load()
  end

  for name, ud in pairs(unitDefs) do
    local dlMenu = dlBuilds[name]
    if (dlMenu) then

      ud.buildoptions = ud.buildoptions or {}
      for _, entry in ipairs(dlMenu) do
        local buildOptions = ud.buildoptions

        local index = nil
        if (entry.afterName) then
          index = FindNameIndex(entry.afterName, buildOptions)
          index = index and (index + 1) or nil
        elseif (entry.beforeName) then
          index = FindNameIndex(entry.beforeName, buildOptions)
        end

        if (index == nil) then
          if (entry.menu and entry.button) then
            index = ((entry.menu - 2) * 6) + entry.button + 1
          else
            index = (#buildOptions + 1) -- the end
          end
        end
        table.insert(buildOptions, index, entry.unitName)
      end
      ud.buildoptions = LinearArray(ud.buildoptions)

    end
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return {
  Execute = Execute,
}

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

