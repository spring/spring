--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    explosions.lua
--  brief:   explosions/*.tdf parser
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local explosionDefs = {}

local shared = {} -- shared amongst the lua explosiondef enviroments

local preProcFile  = 'gamedata/explosions_pre.lua'
local postProcFile = 'gamedata/explosions_post.lua'

local TDF = TDFparser or VFS.Include('gamedata/parse_tdf.lua')

local system = VFS.Include('gamedata/system.lua')
VFS.Include('gamedata/VFSUtils.lua')
local section = 'explosions.lua'

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Run a pre-processing script if one exists
--

if (VFS.FileExists(preProcFile)) then
  Shared = shared  -- make it global
  ExplosionDefs = explosionDefs  -- make it global
  VFS.Include(preProcFile)
  ExplosionDefs = nil
  Shared = nil
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Load the TDF explosiondef files
--

local function ParseColorString(str)
  local color = { 1.0, 1.0, 0.8 }
  local i = 1
  for word in string.gmatch(str, '[^,]+') do
    local val = tonumber(word)
    if (val) then
      color[i] = val
    end
    i = i + 1
    if (i > 3) then
      break
    end
  end
  return color
end


local function FixGroundFlashColor(ed)
  for spawnName, groundFlash  in pairs(ed) do
    if ((spawnName == 'groundflash') and (type(groundFlash) == 'table')) then
      local colorStr = groundFlash.color
      if (type(colorStr) == 'string') then
        groundFlash.color = ParseColorString(colorStr)
      end
    end
  end
end

local function LoadTDFs(dir)
  local tdfFiles = RecursiveFileSearch(dir, '*.tdf')

  for _, filename in ipairs(tdfFiles) do
    local eds, err = TDF.Parse(filename)
    if (eds == nil) then
      Spring.Log(section, LOG.ERROR, 'Error parsing ' .. filename .. ': ' .. err)
    else
      for name, ed in pairs(eds) do
        ed.filename = filename
        explosionDefs[name] = ed
        FixGroundFlashColor(ed)
      end
    end
  end
 end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------


local function LoadLuas(dir)
  local luaFiles = RecursiveFileSearch(dir, '*.lua')

  for _, filename in ipairs(luaFiles) do
    local edEnv = {}
    edEnv._G = edEnv
    edEnv.Shared = shared
    edEnv.GetFilename = function() return filename end
    setmetatable(edEnv, { __index = system })
    local success, eds = pcall(VFS.Include, filename, edEnv)
    if (not success) then
      Spring.Log(section, LOG.ERROR, 'Error parsing ' .. filename .. ': ' .. tostring(eds))
    elseif (eds == nil) then
      Spring.Log(section, LOG.ERROR, 'Missing return table from: ' .. filename)
    else
      for edName, ed in pairs(eds) do
        if ((type(edName) == 'string') and (type(ed) == 'table')) then
          ed.filename = filename
          explosionDefs[edName] = ed
		end
      end
    end
  end  
end

--  Load the TDF format explosiondef files
--  Files in effects/ will override those in gamedata/explosions/
LoadTDFs('gamedata/explosions/')
LoadTDFs('effects/')
--  Load the raw LUA format explosiondef files
--  (these will override the TDF versions)
--  Files in effects/ will override those in gamedata/explosions/
LoadLuas('gamedata/explosions/')
LoadLuas('effects/')

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Run a post-processing script if one exists
--

if (VFS.FileExists(postProcFile)) then
  Shared = shared  -- make it global
  ExplosionDefs = explosionDefs  -- make it global
  VFS.Include(postProcFile)
  ExplosionDefs = nil
  Shared = nil
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return explosionDefs

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
