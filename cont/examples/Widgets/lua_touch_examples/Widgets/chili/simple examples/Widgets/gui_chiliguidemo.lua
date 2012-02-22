--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "chiliGUIDemo",
    desc      = "GUI demo for robocracy",
    author    = "quantum",
    date      = "WIP",
    license   = "WIP",
    layer     = 0,
    enabled   = false  --  loaded by default?
  }
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

-- chili short cuts
local Button
local Control
local Label
local Colorbars
local Checkbox
local Trackbar
local Window
local ScrollPanel
local StackPanel
local Grid
local TextBox
local Image
local screen0

local oldPrint = print
local function print(...)
  oldPrint(...)
  io.flush()
end

-- gui elements
local window0
local window01
local gridWindow0
local gridWindow1
local windowImageList
local window1

function widget:Initialize()

  Chili = WG.Chili
  Button = Chili.Button
  Control = Chili.Control
  Label = Chili.Label
  Colorbars = Chili.Colorbars
  Checkbox = Chili.Checkbox
  Trackbar = Chili.Trackbar
  Window = Chili.Window
  ScrollPanel = Chili.ScrollPanel
  StackPanel = Chili.StackPanel
  Grid = Chili.Grid
  TextBox = Chili.TextBox
  Image = Chili.Image

  screen0 = Chili.Screen0

local function ToggleOrientation(self)
  local panel = self:FindParent"layoutpanel"
  panel.orientation = ((panel.orientation == "horizontal") and "vertical") or "horizontal"
  panel:UpdateClientArea()
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local testText2 = 
[[Bolivians are ]]

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local cs = {
    Button:New{
      x      = 20,
      y      = 20,
    },
    Label:New{
      x      = 20,
      y      = 50,
      caption= 'FOOBAR',
    },
    ScrollPanel:New{
      backgroundColor = {0,0,0,0.5},
      children = {
        Button:New{caption="foo", width = 100, height = 100},
      }
    },
    Checkbox:New{
      x     = 20,
      y     = 70,
      caption = 'foo',
    },
    Trackbar:New{
      x     = 20,
      y     = 90,
    },
    Colorbars:New{
      x     = 20,
      y     = 120,
    },
}

window0 = Window:New{
  x = 200,
  y = 450,
  width  = 200,
  height = 200,
  parent = screen0,

  children = {
    Label:New{valign="center",autosize=true,caption='This window should be empty!\n(except this text)'},
  },
}

local panel0 = StackPanel:New{
  width = 200,
  height = 200,
  --resizeItems = false,
  defaultChildMargin = {5, 5, 5, 5},
  margin = {10, 10, 10, 10},
  parent = window0,
  children = cs,
}

panel0:Dispose()

-- we need a container that supports margin if the control inside uses margins
window01 = Window:New{  
  x = 200,  
  y = 200,  
  clientWidth  = 200,
  clientHeight = 200,
  parent = screen0,

}

local panel1 = StackPanel:New{
  width = 200,
  height = 200,
  --resizeItems = false,
  defaultChildMargin = {5, 5, 5, 5},
  anchors = {top=true,left=true,bottom=true,right=true},
  margin = {10, 10, 10, 10},
  parent = window01,
  children = cs,
}


local gridControl = Grid:New{
  name = 'foogrid',
  width = 200,  
  height = 200,  
  children = {
    Button:New{backgroundColor = {0,0.6,0,1}, textColor = {1,1,1,1}, caption = "Toggle", OnMouseUp = {ToggleOrientation}},
    Button:New{caption = "2"},
    Button:New{caption = "3"},
    Button:New{caption = "4", margin = {10, 10, 10, 10}},
    Button:New{caption = "5"},
    Button:New{caption = "6"},
    Button:New{caption = "7"},
  }
}

gridWindow0 = Window:New{  
  parent = screen0,
  x = 450,  
  y = 450,  
  clientWidth = 200,  
  clientHeight = 200,  
  children = { 
    gridControl
  },
}

gridWindow1 = Window:New{  
  parent = screen0,
  x = 650,  
  y = 750,
  clientWidth = 200,  
  clientHeight = 200,  
  children = {

    Button:New{x=120,y=180, anchors = {bottom=true,right=true}, caption = "2"},

    TextBox:New{width = 200, anchors = {top=true,left=true,bottom=true,right=true}, text = testText2}

  },
}



--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

--[[
local layoutPanel0 = Chili.LayoutPanel:New{
  x = 750,
  y = 200,
  width = 200,
  height = 400,
  parent = screen0,

  --columns = 2,
  --minItemWidth = 120,
  minItemHeight = 60,

  children = {
    Button:New{width = 120, height = 40, caption = "Toggle Orientation", OnMouseUp = {ToggleOrientation}},
    Button:New{},
    Button:New{},
    Button:New{},
    Button:New{},
    Button:New{},
    Button:New{},
  }
}
--]]

windowImageList = Window:New{
  x = 700,
  y = 200,
  clientWidth = 410,
  clientHeight = 400,
  parent = screen0,
}

local control = ScrollPanel:New{  
  clientWidth = 410,
  clientHeight = 400,
  anchors = {top=true,left=true,bottom=true,right=true},
  parent = windowImageList,  
  children = {    
    --Button:New{width = 410, height = 400, anchors = {top=true,left=true,bottom=true,right=true}},
    Chili.ImageListView:New{
      name = "MyImageListView",
      width = 410,
      height = 400,
      anchors = {top=true,left=true,bottom=true,right=true},
      dir = "LuaUI/Images/",
      OnSelectItem = {
        function(obj,itemIdx,selected)
          Spring.Echo("image selected ",itemIdx,selected)
        end,
      },
      OnDblClickItem = {
        function(obj,itemIdx)
          Spring.Echo("image dblclicked ",itemIdx)
        end,
      },
    }
  }
}


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local testText = 
[[Bolivians are voting in a referendum on a new constitution that President Evo Morales says will empower the country's indigenous majority.

The changes also include strengthening state control of Bolivia's natural resources, and no longer recognising Catholicism as the official religion.

The constitution is widely expected to be approved.
Mr Morales, an Aymara Indian, has pursued political reform but has met fierce resistance from some sectors.
Opponents concentrated in Bolivia's eastern provinces, which hold rich gas deposits, argue that the new constitution would create two classes of citizenship - putting indigenous people ahead of others.

The wrangling has spilled over into, at times, deadly violence. At least 30 peasant farmers were ambushed and killed on their way home from a pro-government rally in a northern region in September.

President Morales has said the new constitution will pave the way for correcting the historic inequalities of Bolivian society, where the economic elite is largely of European descent.
]]

window1 = Window:New{  
  x = 450,  
  y = 200,  
  clientWidth  = 200,
  clientHeight = 200,
  resizable = true,
  draggable = true,
  parent = screen0,
  children = {
    ScrollPanel:New{
      width = 200,
      height = 200,
      anchors = {top=true,left=true,bottom=true,right=true},
      horizontalScrollbar = false,
      children = {
        TextBox:New{width = 200, anchors = {top=true,left=true,bottom=true,right=true}, text = testText}
      },
    },
  }
}

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
end --Initialize

function widget:Shutdown()
  window0:Dispose()
  window01:Dispose()
  gridWindow0:Dispose()
  gridWindow1:Dispose()
  windowImageList:Dispose()
  window1:Dispose()
end

