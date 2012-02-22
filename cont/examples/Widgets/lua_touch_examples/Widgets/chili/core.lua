local includes = {
  --"Headers/autolocalizer.lua",
  "Headers/util.lua",
  "Headers/links.lua",

  "Handlers/debughandler.lua",
  "Handlers/taskhandler.lua",
  "Handlers/skinhandler.lua",
  "Handlers/themehandler.lua",
  "Handlers/fonthandler.lua",
  "Handlers/texturehandler.lua",

  "Controls/object.lua",
  "Controls/font.lua",
  "Controls/control.lua",
  "Controls/screen.lua",
  "Controls/window.lua",
  "Controls/label.lua",
  "Controls/button.lua",
  "Controls/textbox.lua",
  "Controls/checkbox.lua",
  "Controls/trackbar.lua",
  "Controls/colorbars.lua",
  "Controls/scrollpanel.lua",
  "Controls/image.lua",
  "Controls/textbox.lua",
  "Controls/layoutpanel.lua",
  "Controls/grid.lua",
  "Controls/stackpanel.lua",
  "Controls/imagelistview.lua",
  "Controls/progressbar.lua",
  "Controls/multiprogressbar.lua",
  "Controls/scale.lua",
  "Controls/panel.lua",
  "Controls/treeviewnode.lua",
  "Controls/treeview.lua",
}

local Chili = widget

Chili.CHILI_DIRNAME = LUAUI_DIRNAME.."Widgets/chili/"

if (-1>0) then
  Chili = {}
  -- make the table strict
  VFS.Include(Chili.CHILI_DIRNAME .. "Headers/strict.lua")(Chili, widget)
end

for _, file in ipairs(includes) do
  VFS.Include(Chili.CHILI_DIRNAME .. file, Chili)
end


return Chili
