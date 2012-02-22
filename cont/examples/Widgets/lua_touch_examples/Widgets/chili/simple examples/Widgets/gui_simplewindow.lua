

function widget:GetInfo()
  return {
		name		= "chilli example",
		desc		= "example chilli code",
		author		= "smoth",
		date		= "8/26/10",
		license   = "none",
		layer		 = 0,
		enabled   = false  --  loaded by default?
  }
end


local windows = {}
local Window
local ScrollPanel
local Chili
local lab
local tx = "Many of your fathers and brothers have perished valiantly in the face of a contemptible enemy. We must never forget what the Federation has done to our people! My brother, Garma Zabi, has shown us these virtues through our own valiant sacrifice. By focusing our anger and sorrow, we are finally in a position where victory is within our grasp, and once again, our most cherished nation will flourish. Victory is the greatest tribute we can pay those who sacrifice their lives for us! Rise, our people, Rise! Take your sorrow and turn it into anger! Zeon thirsts for the strength of its people! SIEG ZEON!!! SIEG ZEON!!! SIEG ZEON!!"
local btn
local scrollpanel1
local redlabel
local bluelabel
local scrolllabel

function widget:Shutdown()
	for i=1,#windows do
		(windows[i]):Dispose()
	end
end

function widget:Initialize()

	if (not WG.Chili) then
		widgetHandler:RemoveWidget()
		return
	end

	Chili = WG.Chili
	Window = Chili.Window
	ScrollPanel = Chili.ScrollPanel
		
	local screen0 = Chili.Screen0
		
	blockOfText = Chili.TextBox:New{
		x = 5,
		y = 10,
		width = '50%',
		align="left",
		valign="ascender",
		lineSpacing = 0,
		--itemPadding  = {5,5,5,5},
		--itemMargin  = {1,1,1,1},
		text = tx,
		fontOutline = true,
	}
	
	scrollabel1 = Chili.Label:New{
		x = '51%',
		y = '10%',
		width = 150,
		height = 15,
		valign = "ascender",
		caption = "I AM A RED LABEL!",
		fontsize = 13,
		autosize = false,
		textColor = {1,0.1,0.1,0.9},
		anchors = {left=true,bottom=true,right=true},
	}	
		
	scrollabel2 = Chili.Label:New{
		x = '51%',
		y = '40%',
		width = 150,
		height = 15,
		valign = "ascender",
		caption = "I AM A GREEN LABEL!",
		fontsize = 13,
		autosize = false,
		textColor = {0.1,1,0.1,0.9},
		anchors = {left=true,bottom=true,right=true},
	}		
	
	scrollpanel1 = ScrollPanel:New{
  		x = 0,
		y = 50,
		bottom = 0,
		right=6,
		horizontalScrollbar = false,
		verticalSmartScroll = true,
		disableChildrenHitTest = true,

		children = {
			blockOfText,
			scrollabel1,
			scrollabel2,
		},
	}		
		
	btn = Chili.Button:New{
		x = 5,
		y = 25,
		width = 15,
		height = 15,
		caption = "O",
		anchors = {top=true,right=true},
		textColor = {1,1,1,0.9},
		backgroundColor = {0.1,1,0.1,0.9},
		-- OnClick = {function() window1:Dispose() end},
		}

	redlabel = Chili.Label:New{
		x = 5,
		y = 8,
		width = 150,
		height = 15,
		valign = "ascender",
		caption = "I AM A RED LABEL!",
		fontsize = 13,
		autosize = false,
		textColor = {1,0.1,0.1,0.9},
		anchors = {left=true,bottom=true,right=true},
	}	
	
	window0 = Chili.Window:New{
		dockable = true,
		parent = screen0,
		caption = "Window name",
		draggable = true,
		resizable = true,
		dragUseGrip = true,
		clientWidth = 400,
		clientHeight = 150,
		backgroundColor = {0.8,0.8,0.8,0.9},
		
		children = { -- to nest this way, table has to come BEFORE the parent or it won't show
						scrollpanel1,
						btn,
						redlabel,
		},
	}
	windows[#windows+1] = window0
	
	bluelabel = Chili.Label:New{
		parent = window0,--can nest this way as well, has to come AFTER parent or it won't see the parent
		x = 160,
		y = 8,
		width = 150,
		height = 15,
		valign = "ascender",
		caption = "I AM A BLUE LABEL!",
		fontsize = 13,
		autosize = false,
		textColor = {0.1,0.1,1,0.9},
		anchors = {left=true,bottom=true,right=true},
	}	

end