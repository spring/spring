--//=============================================================================
--// Theme

theme = {}

theme.name = "default"


--//=============================================================================
--// Define default skins

local defaultSkin = "Robocracy"
--local defaultSkin = "DarkGlass"

theme.skin = {
  general = {
    skinName = defaultSkin,
  },

  imagelistview = {
  --  imageFolder      = "luaui/images/folder.png",
  --  imageFolderUp    = "luaui/images/folder_up.png",
  },

  icons = {
  --  imageplaceholder = "luaui/images/placeholder.png",
  },
}


--//=============================================================================
--// Theme

function theme.GetDefaultSkin(class)
  local skinName

  repeat
    skinName = theme.skin[class.classname].skinName
    class = class.inherited
--FIXME check if the skin contains the current control class! if not use inherit the theme table before doing so in the skin
  until ((skinName)and(SkinHandler.IsValidSkin(skinName)))or(not class);

  if (not skinName)or(not SkinHandler.IsValidSkin(skinName)) then
    skinName = theme.skin.general.skinName
  end

  if (not skinName)or(not SkinHandler.IsValidSkin(skinName)) then
    skinName = "default"
  end

  return skinName
end


function theme.LoadThemeDefaults(control)
  if (theme.skin[control.classname])
    then table.merge(control,theme.skin[control.classname]) end -- per-class defaults
  table.merge(control,theme.skin.general)
end

