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


--------------------------------------------------------------------------------

local function SafeLower(str)
  if (type(str) == 'string') then
    return string.lower(str)
  end
  return nil
end


local function Load()
--  print('download_builds.lua Load() start')
  dlBuilds = {}
  local files = VFS.DirList('download/', '*.tdf')
  for i, f in ipairs(files) do
    local tdf, err = TDF.Parse(f)
    if (tdf == nil) then
      Spring.Echo(err)
    else
      for menuEntry, menuTable in pairs(tdf) do
        if (type(menuTable) == 'table') then
          local unitMenu = SafeLower(menuTable.unitmenu)
          local dlMenu = dlBuilds[unitMenu] or {}
          local unitName   = SafeLower(menuTable.unitname)
          local afterName  = SafeLower(menuTable.aftername)
          local beforeName = SafeLower(menuTable.beforename)
          local menu       = tonumber(dlMenu.menu)
          local button     = tonumber(dlMenu.button)
          if (unitName) then
            table.insert(dlMenu, {
              unitName   = unitName,
              afterName  = afterName,
              beforeName = beforeName,
              menu       = menu,
              button     = button,
            })
          end
        end
      end
    end
  end
--  print('download_builds.lua Load() end')
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


local function Execute(unitDefs)
--  print('download_builds.lua Execute() start')
  if (dlBuilds == nil) then
    Load()
  end
  for name, ud in pairs(unitDefs) do
    local dlMenu = dlBuilds[name]
    if (dlMenu) then
      for _, entry in ipairs(entry) do
        local buildOptions = ud.buildOptions or {}
        local index = nil
        if (entry.afterName) then
          index = FindNameIndex(entry.afterName, buildOptions)
          index = index and (index + 1) or nil
        elseif (entry.beforeName) then
          index = FindNameIndex(entry.afterName, buildOptions)
        end

        if (index == nil) then
          if (entry.menu and entry.button) then
            index = ((entry.menu - 2) * 6) + entry.button + 1
          else
            index = (#buildOptions + 1) -- the end
          end
        end

        table.insert(buildOptions, index, unitName)

        ud.buildOptions = buildOptions
      end
    end
  end
--  print('download_builds.lua Execute() end')
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return {
  Execute = Execute,
}

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
                                                                                                      