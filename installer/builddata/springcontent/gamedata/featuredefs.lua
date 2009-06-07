--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    featuredefs.lua
--  brief:   featuredef parser
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local featureDefs = {}

local shared = {} -- shared amongst the lua featuredef enviroments

local preProcFile  = 'gamedata/featuredefs_pre.lua'
local postProcFile = 'gamedata/featuredefs_post.lua'

local TDF = TDFparser or VFS.Include('gamedata/parse_tdf.lua')

local system = VFS.Include('gamedata/system.lua')


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Run a pre-processing script if one exists
--

if (VFS.FileExists(preProcFile)) then
  Shared = shared  -- make it global
  FeatureDefs = featureDefs  -- make it global
  VFS.Include(preProcFile)
  FeatureDefs = nil
  Shared = nil
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Load the TDF featuredef files
--

local tdfFiles = {}
do
  tdfFiles      = VFS.DirList('features/all worlds/', '*.tdf')
  local corpses = VFS.DirList('features/corpses/',    '*.tdf')
  for _, f in ipairs(corpses) do
    table.insert(tdfFiles, f)
  end
end

for _, filename in ipairs(tdfFiles) do
  local fds, err = TDF.Parse(filename)
  if (fds == nil) then
    Spring.Echo('Error parsing ' .. filename .. ': ' .. err)
  else
    for name, fd in pairs(fds) do
      fd.filename = filename
      featureDefs[name] = fd
    end
  end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  Load the raw LUA format featuredef files
--  (these will override the TDF versions)
--

local luaFiles = {}
do
  luaFiles      = VFS.DirList('features/all worlds/', '*.lua')
  local corpses = VFS.DirList('features/corpses/',    '*.lua')
  for _, f in ipairs(corpses) do
    table.insert(luaFiles, f)
  end
end

for _, filename in ipairs(luaFiles) do
  local fdEnv = {}
  fdEnv._G = fdEnv
  fdEnv.Shared = shared
  fdEnv.GetFilename = function() return filename end
  setmetatable(fdEnv, { __index = system })
  local success, fds = pcall(VFS.Include, filename, fdEnv)
  if (not success) then
    Spring.Echo('Error parsing ' .. filename .. ': ' .. fds)
  elseif (fds == nil) then
    Spring.Echo('Missing return table from: ' .. filename)
  else
    for fdName, fd in pairs(fds) do
      if ((type(fdName) == 'string') and (type(fd) == 'table')) then
        fd.filename = filename
        featureDefs[fdName] = fd
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
  FeatureDefs = featureDefs  -- make it global
  VFS.Include(postProcFile)
  FeatureDefs = nil
  Shared = nil
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return featureDefs

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
