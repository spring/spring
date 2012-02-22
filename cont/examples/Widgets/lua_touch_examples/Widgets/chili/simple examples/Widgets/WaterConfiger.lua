
--//=============================================================================

function widget:GetInfo()
  return {
    name      = ".WaterConfiger",
    desc      = "GUI for BumpWater Map-Defined-Configs",
    author    = "jK",
    date      = "2009",
    license   = "GPLv2",
    layer     = 0,
    enabled   = true  --  loaded by default?
  }
end

--//=============================================================================

local windows = {}

function widget:Shutdown()
  Spring.SetConfigInt('BumpWaterUseUniforms', 0)
  Spring.SendCommands('water 4')
  for i=1,#windows do
    (windows[i]):Dispose()
  end
end

function widget:Initialize()
  Spring.SetConfigInt('BumpWaterUseUniforms', 1)
  Spring.SendCommands('water 4')

--//=============================================================================
--// Helpers

  local function GetGameConst(name)
    local wanted = 'water' .. name:lower()
    for constName,v in pairs(Game) do
      if (constName:lower()==wanted) then
        return v
      end
    end
  end


  local function formatNumber(num, fraction)
    local s = ('%.7f'):format(tonumber(num))
    if (not fraction) then
      s = s:sub(1,s:find('.',1,true)-1)
    else
      _,_,s = s:find('(%d*%.%d%d)')
    end
    return s
  end


  local function ReloadWater()
    Spring.SendCommands('water ' .. Spring.GetWaterMode())
  end

--//=============================================================================
--// Available Water Options

  local options = {
    {param = 'forceRendering', type = 'checkbox', needReload = true, tooltip = "Should the water be rendered even when minMapHeight>0.\nUse it to avoid the jumpin of the outside-map water rendering (BumpWater: endlessOcean option) when combat explosions reach groundwater.", },

    {param = 'numTiles', type = 'float', options={0, 5, 1, 2}, tooltip = "How many Tiles does the `normalTexture` have?\nSuch Tiles are used when DynamicWaves are enabled in BumpWater, the more the better.\nCheck the example php script to generate such tiled bumpmaps.", },
    {param = 'normalTexture', type = 'texture', tooltip = "The normal texture.", },

    {param = 'perlinStartFreq', type = 'float', options={0, 20, 0.5, 8}, tooltip = "Scales the normal texture.", },
    {param = 'perlinLacunarity', type = 'float', options={0, 20, 0.1, 3}, tooltip = "Scales the normal texture.", },
    {param = 'perlinAmplitude', type = 'float', options={0, 3, 0.01, 0.9}, tooltip = "Scales the normal texture.", },

                                            --//  min, max, step, default
    {param = 'diffuseFactor', type = 'float', options={0, 10, 0.1, 1}},
    {param = 'diffuseColor', type = 'color'},

    {param = 'specularFactor', type = 'float', options={0, 10, 0.1, 1}},
    {param = 'specularPower', type = 'float', options={0, 150, 2, 20}},
    {param = 'specularColor', type = 'color'},

    {param = 'ambientFactor', type = 'float', options={0, 10, 0.1, 1}},

    {param = 'fresnelMin', type = 'float', options={0, 1, 0.05, 0.2}, tooltip = "Minimum reflection strength.", },
    {param = 'fresnelMax', type = 'float', options={0, 1, 0.05, 0.8}, tooltip = "Maximum reflection strength.", },
    {param = 'fresnelPower', type = 'float', options={0, 50, 0.25, 4}, tooltip = "Determines how fast the reflection increase with angle(viewdir,water_normal).", },

    {param = 'reflectionDistortion', type = 'float', options={0, 2, 0.05, 1}, tooltip = "", },

    {param = 'blurBase', type = 'float', options={0, 10, 1, 2}, tooltip = "How much should the reflection be blurred.", },
    {param = 'blurExponent', type = 'float', options={0, 10, 1, 1.5}, tooltip = "How much should the reflection be blurred.",},

    {param = 'hasWaterPlane', type = 'checkbox', tooltip = "The WaterPlane is a single Quad beneath the map.\nIt should have the same color as the ocean floor to hide the map -> background boundary.",},
    {param = 'planeColor',  type = 'color', tooltip = "The WaterPlane is a single Quad beneath the map.\nIt should have the same color as the ocean floor to hide the map -> background boundary.",},

    {param = 'shoreWaves', type = 'checkbox', needReload = true},
    {param = 'foamTexture', type = 'texture', tooltip = "Used for Shorewaves.",},

    {param = 'texture', type = 'texture', tooltip = "`water 0` texture",},
    {param = 'repeatX', type = 'float', options={0, 10, 1, 0}, needReload = true, tooltip = "`water 0` texture repeat horizontal",},
    {param = 'repeatY', type = 'float', options={0, 10, 1, 0}, needReload = true, tooltip = "`water 0` texture repeat vertical",},
  }

  for i=1,#options do
    options[i].value = GetGameConst(options[i].param)
    if (options[i].value==nil)and(options[i].type=="float") then
       options[i].value = ((options[i].options)and(options[i].options[4]))
    end
  end

--//=============================================================================
--// Chili GUI creation

  local Chili = WG.Chili
  local screen0 = Chili.Screen0

  window0 = Chili.Window:New{
    parent = screen0,
    caption = "Water Settings",
    x = 20,
    y = 250,
    clientWidth = 350,
    clientHeight = 630,
    backgroundColor = {0.8,0.8,0.8,0.9},
  }
  windows[#windows+1] = window0

--//=============================================================================
--// Determine first column width
  
  local testLabel = Chili.Label:New{}
  local maxWidth = 0
  for i=1,#options do
    testLabel:SetCaption(options[i].param)
    if (testLabel.width>maxWidth) then maxWidth = testLabel.width end
  end
  testLabel:Dispose()

--//=============================================================================
--// GUI Helpers

  local function OnChange(self,value)
    Spring.SetWaterParams({[self.name] = value})
    self.custom.options.value = value 

    if (self.custom)and(self.custom.needReload) then
      ReloadWater()
    end

    if (type(value)~="number") then
      return;
    end

    local s = formatNumber(value, (self.step%1)~=0)
    self.custom.label:SetCaption(s)
  end

  local function OpenTextureSelector(button)
    local window1 = Chili.Window:New{
      parent = screen0,
      x = 400,
      y = 250,
      clientWidth = 500,
      clientHeight = 500,
      backgroundColor = {0.8,0.8,0.8,0.5},
    }
    windows[#windows+1] = window1

    local btn = Chili.Button:New{
      parent = window1,
      x = 483,
      y = 3,
      width = 15,
      height = 15,
      caption = "x",
      anchors = {top=true,right=true},
      textColor = {1,1,1,0.9},
      backgroundColor = {0.1,0.1,0.1,0.9},
      OnClick = {function() window1:Dispose() end},
    }

    local label = Chili.Label:New{
      parent = window1,
      x = 10,
      y = 485,
      width = 460,
      height = 15,
      valign = "ascender",
      caption = button.caption,
      fontsize = 13,
      autosize = false,
      anchors = {left=true,bottom=true,right=true},
    }

    local scroll0 = Chili.ScrollPanel:New{
      parent = window1,
      x = 20,
      y = 20,
      width = 460,
      height = 460,
      anchors = {top=true,left=true,bottom=true,right=true},
      backgroundColor = {0.1,0.1,0.1,0.9},
    }

    local imagelistview = Chili.ImageListView:New{
      parent = scroll0,
      width = 450,
      height = 450,
      multiSelect = false,
      anchors = {top=true,left=true,bottom=true,right=true},
      OnSelectItem = {
        function(obj,itemIdx,selected)
          if (selected) then
            label:SetCaption(obj.items[itemIdx])
          end
        end,
      },
      OnDblClickItem = {
        function(obj,filename,itemIdx)
          Spring.SetWaterParams({[button.name] = filename});
          button.custom.options.value = filename
          button:SetCaption(filename);
          window1:Dispose();
          ReloadWater();
        end,
      },
    }

    imagelistview:GotoFile(button.caption)
  end


  local i = 1
  local y = 30
  local function AddTrackbar(paramOpt)
    local label = Chili.Label:New{
      parent = window0,
      caption = paramOpt.param,
      x = 10,
      y = y,
      width = maxWidth,
      height = 15,
      tooltip = paramOpt.tooltip,
    }

    local label2 = Chili.Label:New{
      parent = window0,
      caption = formatNumber(paramOpt.value, (paramOpt.options[3]%1)~=0),
      align = "right",
      x = 290,
      y = y,
      width = 40,
      height = 15,
      autosize = false,
      tooltip = paramOpt.tooltip,
    }

    local trackbar = Chili.Trackbar:New{
      parent = window0,
      name = paramOpt.param,
      x = 10+maxWidth,
      y = y,
      width = 280-maxWidth,
      height = 15,
      min  = paramOpt.options[1],
      max  = paramOpt.options[2],
      step = paramOpt.options[3],
      value = paramOpt.value,
      custom = {label = label2, options = paramOpt},
      OnChange = {OnChange},
      tooltip = paramOpt.tooltip,
    }

    y = y + 20
  end


  local function AddColorbar(paramOpt)
    local label = Chili.Label:New{
      parent = window0,
      name = 'label'..i,
      caption= paramOpt.param,
      x = 10,
      y = y,
      width = 320,
      height = 20,
      tooltip = paramOpt.tooltip,
    }

    Chili.Colorbars:New{
      parent = window0,
      name  = paramOpt.param,
      x = 10+maxWidth,
      y = y,
      width = 320-maxWidth,
      height = 20,
      color = paramOpt.value,
      custom = {options = paramOpt},
      OnChange = {OnChange},
      tooltip = paramOpt.tooltip,
    }
    i = i + 1
    y = y + 25
  end


  local function AddCheckbox(paramOpt)
    Chili.Checkbox:New{
      parent = window0,
      name = paramOpt.param,
      caption = paramOpt.param,
      custom = {needReload = paramOpt.needReload, options = paramOpt},
      tooltip = paramOpt.tooltip,
      x = 10,
      y = y,
      width = 320,
      height = 20,
      checked = paramOpt.value,
      OnChange = {OnChange},
    }
    i = i + 1
    y = y + 22
  end


  local function AddTextureButton(paramOpt)
    local label = Chili.Label:New{
      parent = window0,
      name = 'lbl_' .. paramOpt.param,
      caption = paramOpt.param .. ':',
      valign = "center",
      tooltip = paramOpt.tooltip,
      x = 10,
      y = y,
      width = 320,
      height = 20,
    }

    Chili.Button:New{
      parent = window0,
      name  = paramOpt.param,
      caption = paramOpt.value,
      tooltip = paramOpt.tooltip,
      x = 10+maxWidth,
      y = y,
      width = 320-maxWidth,
      height = 20,
      custom = {options = paramOpt},
      OnClick = {function(obj) OpenTextureSelector(obj) end},
    }
    i = i + 1
    y = y + 25
  end


--//=============================================================================
--// Create GUI Header controls

  Chili.Button:New{
    parent = window0,
    caption = "x",
    x = 325,
    y = 5,
    width = 20,
    height = 20,
    textColor = {1,1,1,0.9},
    backgroundColor = {0.9,0.1,0.1,0.9},
    anchors = {top=true, right=true},
    OnClick = {function() Spring.SendCommands('luaui disablewidget ' .. widget:GetInfo().name) end},
  }

--//=============================================================================
--// Create water config controls

  for i=1,#options do
    local paramOpt = options[i]
    if (paramOpt.type == 'texture') then
      AddTextureButton(paramOpt)
    elseif (paramOpt.type == 'color') then
      AddColorbar(paramOpt)
    elseif (paramOpt.type == 'float') then
      AddTrackbar(paramOpt)
    elseif (paramOpt.type == 'checkbox') then
      AddCheckbox(paramOpt)
    end
  end

--[[
  local label = Chili.Label:New{
    parent = window0,
    name = 'label'..i,
    caption = 'surfaceColor',
    x = 10,
    y = y,
    width = 320,
    height = 20,
  }

  local surfaceColor = Game.waterSurfaceColor
  surfaceColor[4] = Game.waterSurfaceAlpha

  Chili.Colorbars:New{
    parent = window0,
    name  = 'surfaceColor',
    x = 10+maxWidth,
    y = y,
    width = 320-maxWidth,
    height = 20,
    color = surfaceColor,
    OnChange = {function(self,color) Spring.SetWaterParams({surfaceColor = color, surfaceAlpha = color[4]}) end},
  }
  i,y = i + 1,y + 25

--]]

--//=============================================================================
--// Footer

  local function CamelCase(str)
    return str:sub(1,1):upper() .. str:sub(2)
  end

  local function Align(str,chars)
    return str .. (" "):rep(chars-str:len())
  end

  Chili.Button:New{
    parent = window0,
    caption = "Save SMD/Lua table to MapWater.txt",
    x = 35,
    y = y+15,
    width = 280,
    height = 40,
    fontsize = 15,
    textColor = {1,1,1,0.9},
    backgroundColor = {0.9,0.1,0.1,0.9},
    OnClick = { function()
      local f = io.open('MapWater.txt','w')
      f:write('//SMF Table\n')
      f:write('  [WATER]\n')
      f:write('  {\n')
      for i=1,#options do
        local opt = options[i]
        if (opt.type == "float") then
          f:write('    Water' .. CamelCase(opt.param) .. '=' .. formatNumber(opt.value, (opt.options[3]%1)~=0) .. ';\n')
        elseif (opt.type == "checkbox") then
          f:write('    Water' .. CamelCase(opt.param) .. '=' .. (opt.value and "1" or "0") .. ';\n')
        elseif (opt.type == "texture") then
          f:write('    Water' .. CamelCase(opt.param) .. '=' .. (opt.value) .. ';\n')
        elseif (opt.type == "color") then
          f:write('    Water' .. CamelCase(opt.param) .. '=' .. (formatNumber(opt.value[1],true) .. ' ' .. formatNumber(opt.value[2],true) .. ' ' .. formatNumber(opt.value[3],true)) .. ';\n')
        end
      end
      f:write('  }\n\n')

      f:write('//Lua Table\n')
      f:write('  water = {\n')
      f:write('  {\n')
      local maxLength = 0
      for i=1,#options do
        local opt = options[i]
        local l = opt.param:len()
        if (l>maxLength) then maxLength = l end
      end
      for i=1,#options do
        local opt = options[i]
        if (opt.type == "float") then
          f:write('    ' .. Align(opt.param,maxLength) .. ' = ' .. formatNumber(opt.value, (opt.options[3]%1)~=0) .. ',\n')
        elseif (opt.type == "checkbox") then
          f:write('    ' .. Align(opt.param,maxLength) .. ' = ' .. (opt.value and "true" or "false") .. ',\n')
        elseif (opt.type == "texture") then
          f:write('    ' .. Align(opt.param,maxLength) .. ' = "' .. (opt.value) .. '",\n')
        elseif (opt.type == "color") then
          f:write('    ' .. Align(opt.param,maxLength) .. ' = "' .. (formatNumber(opt.value[1],true) .. ' ' .. formatNumber(opt.value[2],true) .. ' ' .. formatNumber(opt.value[3],true)) .. '",\n')
        end
      end
      f:write('  },\n')
      f:close()
    end}
  }

end