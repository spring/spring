--//=============================================================================
--// Theme

SkinHandler = {}

--//=============================================================================
--// Some important folders

local skinFolder = CHILI_DIRNAME .. "Skins/"

SkinHandler.skinFolder = skinFolder

--//=============================================================================
--// load shared skin utils

local SkinUtilsEnv = {}
setmetatable(SkinUtilsEnv,{__index = getfenv()})
VFS.Include(CHILI_DIRNAME .. "Headers/skinutils.lua", SkinUtilsEnv)

--//=============================================================================
--// translates the skin's FileNames to the correct FilePaths
--// (skins just define the name not the path!)

local function SplitImageOptions(str)
  local options, str2 = str:match "^(:.*:)(.*)"
  if (options) then
    return options, str2
  else
    return "", str
  end
end

local function TranslateFilePaths(skinConfig, dirs)
  for i,v in pairs(skinConfig) do
    if (i == "info") then
      --// nothing
    elseif istable(v) then
      TranslateFilePaths(v, dirs)
    elseif isstring(v) then
      local opt, fileName = SplitImageOptions(v)

      for _,dir in ipairs(dirs) do
        local filePath = dir .. fileName
        if VFS.FileExists(filePath) then
          skinConfig[i] = opt .. filePath
          break
        end
      end

    end
  end
end

--//=============================================================================
--// load all skins

knownSkins = {}
SkinHandler.knownSkins = knownSkins

local n = 1
local skinDirs = VFS.SubDirs(skinFolder , "*", VFS.RAW_FIRST)
for i,dir in ipairs(skinDirs) do
  local skinCfgFile = dir .. 'skin.lua'

  if (VFS.FileExists(skinCfgFile, VFS.RAW_FIRST)) then

    --// use a custom enviroment (safety + auto loads skin utils)
    local senv = {}
    setmetatable(senv,{__index = SkinUtilsEnv})

    --// load the skin
    local skinConfig = VFS.Include(skinCfgFile,senv)
    if (skinConfig)and(type(skinConfig)=="table")and(type(skinConfig.info)=="table") then
      skinConfig.info.dir = dir
      SkinHandler.knownSkins[n] = skinConfig
      SkinHandler.knownSkins[skinConfig.info.name:lower()] = skinConfig
      n = n + 1
    end
  end
end

--//FIXME handle multiple-dependencies correctly! (atm it just works with 1 level!)
--// translate filepaths and handle dependencies
for i,skinConfig in ipairs(SkinHandler.knownSkins) do
  local dirs = { skinConfig.info.dir }

  --// translate skinName -> skinDir and remove broken dependencies
  local brokenDependencies = {}
  for i,dependSkinName in ipairs(skinConfig.info.depend or {}) do
    local dependSkin = SkinHandler.knownSkins[dependSkinName:lower()]
    if (dependSkin) then
      dirs[#dirs+1] = dependSkin.info.dir
      table.merge(skinConfig, dependSkin)
    else
      Spring.Echo("Chili: Skin " .. skinConfig.info.name .. " depends on an unknown skin named " .. dependSkinName .. ".")
    end
  end

  --// add the default skindir to the end
  dirs[#dirs+1] = skinFolder .. 'default/'

  --// finally translate all paths
  TranslateFilePaths(skinConfig, dirs)
end

--//=============================================================================
--// Internal

local function GetSkin(skinname)
  return SkinHandler.knownSkins[tostring(skinname):lower()]
end

SkinHandler.defaultSkin = GetSkin('default')

--//=============================================================================
--// API

function SkinHandler.IsValidSkin(skinname)
  return (not not GetSkin(skinname))
end


function SkinHandler.GetSkinInfo()
  local sk = GetSkin(skinname)
  if (sk) then
    return table.shallowcopy(sk.info)
  end
  return {}
end


function SkinHandler.GetAvailableSkins()
  local skins = {}
  local knownSkins = SkinHandler.knownSkins
  for i=1,#knownSkins do
    skins[i] = knownSkins[i].info.name
  end
  return skins
end


function SkinHandler.LoadSkin(control)
  local skin = GetSkin(control.skinName)
  if (skin) then
    if (skin[control.classname]) then
      table.merge(control,skin[control.classname]) -- per-class defaults
    end 
    table.merge(control,skin.general)
  end

  skin = SkinHandler.defaultSkin
  if (skin[control.classname]) then
    table.merge(control,skin[control.classname]) -- per-class defaults
  end
  table.merge(control,skin.general)
end