--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    weapondefs.lua
--  brief:   weapondef parser
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local weaponDefs = {}

local shared = {} -- shared amongst the lua weapondef enviroments

local preProcFile  = 'gamedata/weapondefs_pre.lua'
local postProcFile = 'gamedata/weapondefs_post.lua'

local TDF = TDFparser or VFS.Include('gamedata/parse_tdf.lua')

TDF.AllowDuplicates(true)
TDF.SetKeyFilter(string.lower)

local system = VFS.Include('gamedata/system.lua')


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Run a post-processing script if one exists
--

if (VFS.FileExists(preProcFile)) then
  Shared = shared  -- make it global
  WeaponDefs = weaponDefs  -- make it global
  VFS.Include(preProcFile)
  WeaponDefs = nil
  Shared = nil
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Load the TDF weapondef files
--

local tdfFiles = VFS.DirList('weapons/', '*.tdf')

for _, filename in ipairs(tdfFiles) do
  local wds, err = TDF.Parse(filename)
  if (wds == nil) then
    Spring.Echo('Error parsing ' .. filename .. ': ' .. err)
  else
    for name, wd in pairs(wds) do
      wd.filename = filename
      weaponDefs[name] = wd
    end
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Load the raw LUA format weapondef files
--  (these will override the TDF versions)
--

local luaFiles = {}
do
  luaFiles      = VFS.DirList('weapons/all worlds/', '*.lua')
  local corpses = VFS.DirList('weapons/corpses/',    '*.lua')
  for _, f in ipairs(corpses) do
    table.insert(luaFiles, f)
  end
end

for _, filename in ipairs(luaFiles) do
  local wdEnv = {}
  wdEnv._G = wdEnv
  wdEnv.Shared = shared
  wdEnv.GetFilename = function() return filename end
  setmetatable(wdEnv, { __index = system })
  local success, wds = pcall(VFS.Include, filename, wdEnv)
  if (not success) then
    Spring.Echo('Error parsing ' .. filename .. ': ' .. wd)
  elseif (wds == nil) then
    Spring.Echo('Missing return table from: ' .. filename)
  else
    for wdName, wd in pairs(wds) do
      if ((type(wdName) == 'string') and (type(wd) == 'table')) then
        wd.filename = filename
        weaponDefs[wdName] = wd
      end
    end
  end  
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Run a post-processing script if one exists
--

if (VFS.FileExists(postProcFile)) then
  Shared = shared  -- make it global
  WeaponDefs = weaponDefs  -- make it global
  VFS.Include(postProcFile)
  WeaponDefs = nil
  Shared = nil
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return weaponDefs

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
