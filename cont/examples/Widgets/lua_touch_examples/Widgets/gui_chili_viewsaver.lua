function widget:GetInfo()
  return {
		name		= "View Saver",
		desc		= "Saves camera postions letting players quickly switch back and forth between them. Only allows map manipulation when holding down a button. Set view on first tap, or holding down view button and manipulating view.",
		author		= "bilhamil",
		date		= "1/21/2012",
		license   = "none",
		layer		 = 0,
		enabled   = false  --  loaded by default?
  }
end

options_path = 'Settings/Camera/View Saver Config'
options_order = {
    --'lblEntries',
    --'lblSmoothness',
    'smoothness',
    'columns',
    'rows',
}

local OverviewAction = function() end

options = {
    lblEntries = {name='Entries', type='label'},
    lblSmoothness = {name='Smoothness', type='label'},
    smoothness = {
		name = 'Smoothness',
		desc = "Controls how smooth the camera moves when switching to a new view.",
		type = 'number',
		min = 0.0, max = 0.8, step = 0.05,
		value = 0.2,
	},
    columns = {
        name = 'Columns',
        desc = 'Columns of view selectors',
        type = 'number',
        min = 1, max = 10, step = 1,
		value = 2,
        OnChange = function(self) ResetColRows() end,
    },
    rows = {
        name = 'Rows',
        desc = 'Rows of view selectors.',
        type = 'number',
        min = 1, max = 10, step = 1,
		value = 2,
        OnChange = function(self) ResetColRows() end,
    }
}
----------------------------------------------------------------------------------------------------
-- OpenGL Stuff
----------------------------------------------------------------------------------------------------
local GL_LINE_LOOP = GL.LINE_LOOP
local GL_LINE_STRIP = GL.LINE_STRIP
local GL_LINES = GL.LINES
local GL_TRIANGLES = GL.TRIANGLES
local GL_QUADS = GL.QUADS
local GL_NICEST = GL.NICEST
local GL_FASTEST = GL.FASTEST
local GL_DONT_CARE = GL.DONT_CARE

local glDepthTest = gl.DepthTest
local glDepthMask = gl.DepthMask
local glBlending = gl.Blending
local glSmoothing = gl.Smoothing
local glColor = gl.Color
local glRect = gl.Rect
local glLineWidth = gl.LineWidth
local glTexCoord = gl.TexCoord
local glTexture = gl.Texture
local glTexRect = gl.TexRect
local glVertex = gl.Vertex
local glBeginEnd = gl.BeginEnd
local glText = gl.Text
local glScissor = gl.Scissor
local glPushMatrix = gl.PushMatrix
local glPopMatrix = gl.PopMatrix
local glTranslate = gl.Translate
local glRotate = gl.Rotate
local glScale = gl.Scale
local glUnitRaw = gl.UnitRaw
local glUseShader = gl.UseShader
local glCallList = gl.CallList
local glLineWidth = gl.LineWidth

local echo = Spring.Echo
local spSetCameraState		= Spring.SetCameraState
local spGetCameraState		= Spring.GetCameraState
local spTraceScreenRay		= Spring.TraceScreenRay

local abs	= math.abs
local min 	= math.min
local max	= math.max
local sqrt	= math.sqrt
local sin	= math.sin
local cos	= math.cos
local modf  = math.fmod
local asin  = math.asin

local windows = {}
local Window
local Chili
local tx = "Many of your fathers and brothers have perished valiantly in the face of a contemptible enemy. We must never forget what the Federation has done to our people! My brother, Garma Zabi, has shown us these virtues through our own valiant sacrifice. By focusing our anger and sorrow, we are finally in a position where victory is within our grasp, and once again, our most cherished nation will flourish. Victory is the greatest tribute we can pay those who sacrifice their lives for us! Rise, our people, Rise! Take your sorrow and turn it into anger! Zeon thirsts for the strength of its people! SIEG ZEON!!! SIEG ZEON!!! SIEG ZEON!!"
local buttonStack
local cameraEnableButton


local mwidth, mheight = Game.mapSizeX, Game.mapSizeZ
local mcx, mcz 	= mwidth / 2, mheight / 2

local buttonPadding = {v=7,h=5}

local downButton = nil

function widget:Shutdown()
	for i=1,#windows do
		(windows[i]):Dispose()
	end
end

local vsx, vsy = widgetHandler:GetViewSizes()
local cx,cy = vsx * 0.5,vsy * 0.5
function widget:ViewResize(viewSizeX, viewSizeY)
	vsx = viewSizeX
	vsy = viewSizeY
	cx = vsx * 0.5
	cy = vsy * 0.5
end

function normalize(vect)
    local mag = sqrt(vect.x * vect.x + vect.y * vect.y + vect.z * vect.z)
    vect.x = vect.x / mag
    vect.y = vect.y / mag
    vect.z = vect.z / mag
    vect.mag = mag
end

--taken from camera_cofc
--Note: If the x,y is not pointing at an onmap point, this function traces a virtual ray to an
--          offmap position using the camera direction and disregards the x,y parameters. Fixme.
local function VirtTraceRay(x,y, cs)
	local whatHit, gpos = spTraceScreenRay(x, y, true, false, true)
	
	if gpos then
        local gx, gy, gz = gpos[1], gpos[2], gpos[3]
        if whatHit ~= "sky" then
            
            if gx < 0 or gx > mwidth or gz < 0 or gz > mheight then
                return false, gx, gy, gz	
            else
                return true, gx, gy, gz
            end
        elseif whatHit == "sky" then
            local dir = {}
            dir.x = gx - cs.px
            dir.y = gy - cs.py
            dir.z = gz - cs.pz
            
            normalize(dir)
            
            local vecDist = (- cs.py) / dir.y
            gx, gy, gz = cs.px + vecDist*dir.x, cs.py + vecDist*dir.y, cs.pz + vecDist*dir.z
            return false, gx, gy, gz
        end
    end
	
	if not cs or not cs.dy or cs.dy == 0 then
        return false, false
	end
	local vecDist = (- cs.py) / cs.dy
	local gx, gy, gz = cs.px + vecDist*cs.dx, 	cs.py + vecDist*cs.dy, 	cs.pz + vecDist*cs.dz
	
	--gy = Spring.GetSmoothMeshHeight (gx,gz)
	return false, gx, gy, gz
end

local function RectangleVertices(l, b, r, t)
	glVertex(l - 0.5, b - 0.5) -- Bottom-left
	glVertex(r + 0.5, b - 0.5) -- Bottom-right
	glVertex(r + 0.5, t + 0.5) -- Top-right
	glVertex(l - 0.5, t + 0.5) -- Top-left
end

function viewButtonDown(button)
    if(button.cs) then
        spSetCameraState(button.cs, options.smoothness.value)
    end
    
    downButton = button
end

function viewButtonUp(button)
    downButton = nil
end

function setButtonCameraState(button)
    button.cs = spGetCameraState()
        
    button.viewport = {}
    button.viewport[1] = {}
    button.viewport[2] = {} 
    button.viewport[3] = {}
    button.viewport[4] = {}
    
    _, button.viewport[1].x, _, button.viewport[1].y  = VirtTraceRay(0  , 0, button.cs)
    _, button.viewport[2].x, _, button.viewport[2].y  = VirtTraceRay(vsx-1, 0, button.cs)
    _, button.viewport[3].x, _, button.viewport[3].y  = VirtTraceRay(vsx-1, vsy - 1, button.cs)
    _, button.viewport[4].x, _, button.viewport[4].y  = VirtTraceRay(0 , vsy - 1, button.cs)

end

function addNewButton()
       
    local newButton = Chili.Button:New{
		--x = 5,
		--y = 25,
        width = "" .. (100/options.columns.value) .. "%",
		height = "" ..(100/(options.rows.value + 1)) .. "%",
		caption = "",
        fontSize = 12;
		anchors = {top=true,right=true},
		textColor = {1,1,1,0.9},
		backgroundColor = {0.1,1,0.1,0.9},
		--onClick = {function() echo("+ button hit") end},
        OnAddCursor = {viewButtonDown},
        OnCursorEntered = {viewButtonDown},
        OnMouseDown = {viewButtonDown},
        showOffset = false,
	}
           
    buttonStack:AddChild(newButton)
end

function DrawButtonViewport(button)
    glVertex(button.viewport[1].x/mwidth, button.viewport[1].y/mheight)
    glVertex(button.viewport[2].x/mwidth, button.viewport[2].y/mheight)
    glVertex(button.viewport[3].x/mwidth, button.viewport[3].y/mheight)
    glVertex(button.viewport[4].x/mwidth, button.viewport[4].y/mheight)
end

function DrawUnitBox()
    glVertex(0, 0)
    glVertex(1, 0)
    glVertex(1, 1)
    glVertex(0, 1)
end

function widget:DrawScreen()

    for j = 1, (#(buttonStack.children)) do
        local child = buttonStack.children[j]
        
        
        local sx, sy = child:LocalToScreen(0 + buttonPadding.h, 0 + buttonPadding.v)
        sy = vsy - sy
        local width = child.width - (2*buttonPadding.h)
        local height = - (child.height - (2*buttonPadding.v))
        
        
        glScissor(sx, sy + height , width, -height)
        glPushMatrix()
        
        glTranslate(sx + 0.05 * width, sy + 0.05 * height, 0)
        glScale(width * 0.9, height * 0.9 , 1)
        
        --glColor({0, 0, 0, 1})
        --glRect(0,0,1, 1)
        glLineWidth(1.75)
        
        
        if child.viewport then
            glColor({0.9, 0.9, 0.9, 0.5})
            glBeginEnd(GL_QUADS, DrawButtonViewport, child)
            
            glColor({8/255, 232/255, 24/255, 201/255})
            glBeginEnd(GL_LINE_LOOP, DrawButtonViewport, child)
            
        end
        
        glPopMatrix()
        
        glScissor(false)
        
    end
    
end

function widget:Update()

    if(downButton and downButton.state ~= "pressed") then
        downButton = nil
    end
    
    for j = 1, (#(buttonStack.children)) do
        local child = buttonStack.children[j]
        if(child._down) then 
            downButton = child
        end
    end
    
    if WG.MultiTouchCameraStatus then
        if (not downButton) then
            WG.MultiTouchCameraStatus.enabled = false
        else
            WG.MultiTouchCameraStatus.enabled = true
        end
    end
    
    if(downButton) then
        if (not downButton.cs) or (WG.MultiTouchCameraStatus and WG.MultiTouchCameraStatus.touching) then
            setButtonCameraState(downButton)
        end
    end
end

function ResetColRows() 
    
    buttonStack:ClearChildren()
    
    for j = 1, options.rows.value * options.columns.value do
        addNewButton()
    end
    
    cameraEnableButton = Chili.Button:New{
		--x = 5,
		--y = 25,
        width = "100%",
		height = "" ..(100/(options.rows.value + 1)) .. "%",
		caption = "Camera",
        fontSize = 12;
		anchors = {top=true,right=true},
		textColor = {1,1,1,0.9},
		backgroundColor = {0.1,1,0.1,0.9},
		--onClick = {function() echo("+ button hit") end},
        OnAddCursor = {viewButtonDown},
        OnCursorEntered = {viewButtonDown},
        OnMouseDown = {viewButtonDown},
        showOffset = false,
	}
    
    buttonStack:AddChild(cameraEnableButton)
end

function widget:Initialize()

	if (not WG.Chili) then
		widgetHandler:RemoveWidget()
		return
	end

    if(WG.MultiTouchCameraStatus) then
        WG.MultiTouchCameraStatus.enabled = false
    end
    
	Chili = WG.Chili
	Window = Chili.Window
			
	local screen0 = Chili.Screen0		
	
    buttonStack = Chili.StackPanel:New{
        resizeItems = false,
        orientation = "horizontal",
        height = "100%",
        width = "100%",
        y = 0,
        x = 0,
        padding = {0, 0, 0, 0},
		itemMargin  = {0, 0, 0, 0},
        centerItems = false,
    }
    
	window0 = Chili.Window:New{
		dockable = true,
		parent = screen0,
		caption = "",
		draggable = true,
		resizable = true,
		dragUseGrip = false,
		width = 400,
		height = 150,
		backgroundColor = {0,0,0,0},
		
		children = { buttonStack }
	}
	windows[#windows+1] = window0
    ResetColRows() 
end

function widget:Shutdown()
    if(WG.MultiTouchCameraStatus) then
        WG.MultiTouchCameraStatus.enabled = true
    end
end