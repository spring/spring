--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    unitdefs.lua
--  brief:   unitdef parser
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local unitDefs = {}

local shared = {} -- shared amongst the lua unitdef enviroments

local preProcFile  = 'gamedata/unitdefs_pre.lua'
local postProcFile = 'gamedata/unitdefs_post.lua'

local FBI = FBIparser or VFS.Include('gamedata/parse_fbi.lua')
local TDF = TDFparser or VFS.Include('gamedata/parse_tdf.lua')

TDF.AllowDuplicates(true)
TDF.SetKeyFilter(string.lower)

local system = VFS.Include('gamedata/system.lua')


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Run a pre-processing script if one exists
--

if (VFS.FileExists(preProcFile)) then
  Shared   = shared    -- make it global
  UnitDefs = unitDefs  -- make it global
  VFS.Include(preProcFile)
  UnitDefs = nil
  Shared   = nil
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Load the FBI/SWU unitdef files
--

local fbiFiles = {}

do
  fbiFiles = VFS.DirList('units/', '*.fbi')
  local swus = VFS.DirList('units/', '*.swu')
  for _, f in ipairs(swus) do
    table.insert(fbiFiles, f)
  end
end

for _, filename in ipairs(fbiFiles) do
  local ud, err = FBI.Parse(filename)
  if (ud == nil) then
    Spring.Echo('Error parsing ' .. filename .. ': ' .. err)
  elseif (ud.unitname == nil) then
    Spring.Echo('Missing unitName in ' .. filename)
  else
    ud.filename = filename
    ud.unitname = string.lower(ud.unitname)
    unitDefs[ud.unitname] = ud
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Load the raw LUA format unitdef files
--  (these will override the FBI/SWU versions)
--

local luaFiles = VFS.DirList('units/', '*.lua')

for _, filename in ipairs(luaFiles) do
  local udEnv = {}
  udEnv._G = udEnv
  udEnv.Shared = shared
  udEnv.GetFilename = function() return filename end
  setmetatable(udEnv, { __index = system })
  local success, uds = pcall(VFS.Include, filename, udEnv)
  if (not success) then
    Spring.Echo('Error parsing ' .. filename .. ': ' .. ud)
  elseif (uds == nil) then
    Spring.Echo('Missing return table from: ' .. filename)
  else
    for udName, ud in pairs(uds) do
      if ((type(udName) == 'string') and (type(ud) == 'table')) then
        ud.filename = filename
        unitDefs[udName] = ud
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
  Shared   = shared    -- make it global
  UnitDefs = unitDefs  -- make it global
  VFS.Include(postProcFile)
  UnitDefs = nil
  Shared   = nil
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Basic checks to kill unitDefs that will crash ".give all"
--

for name, def in pairs(unitDefs) do
  local cob   = 'scripts/'   .. name .. '.cob'
  if (not VFS.FileExists(cob)) then
    unitDefs[name] = nil
    Spring.Echo('WARNING: removed ' .. name .. ' unitDef, missing cob script')
  end

  local obj = def.objectname
  if (obj == nil) then
    unitDefs[name] = nil
    Spring.Echo('WARNING: removed ' .. name .. ' unitDef, missing model param')
  else
    local objfile = 'objects3d/' .. obj
    if ((not VFS.FileExists(objfile))           and
        (not VFS.FileExists(objfile .. '.3do')) and
        (not VFS.FileExists(objfile .. '.s3o'))) then
      unitDefs[name] = nil
      Spring.Echo('WARNING: removed ' .. name .. ' unitDef, missing model file')
    end
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return unitDefs

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
