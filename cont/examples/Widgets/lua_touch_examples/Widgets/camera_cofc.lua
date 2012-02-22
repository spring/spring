--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "Combo Overhead/Free Camera (experimental)",
    desc      = "v0.101 Camera featuring 6 actions. Type \255\90\90\255/luaui cofc help\255\255\255\255 for help.",
    author    = "CarRepairer",
    date      = "2011-03-16",
    license   = "GNU GPL, v2 or later",
    layer     = 1002,
	handler   = true,
    enabled   = false,
  }
end

include("keysym.h.lua")

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local init = true
local trackmode = false --before options

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

options_path = 'Settings/Camera/Advanced Camera Config'
options_order = { 
	'helpwindow', 
	
	'lblRotate',
	'targetmouse', 
	'rotateonedge', 
	'rotfactor',
    'inverttilt',
    'groundrot',
	
	'lblScroll',
	'edgemove', 
	'smoothscroll',
	'speedFactor', 
	'speedFactor_k', 
	'invertscroll', 
	'smoothmeshscroll', 
	
	'lblZoom',
	'invertzoom', 
	'invertalt', 
	'zoomintocursor', 
	'zoomoutfromcursor', 
	'zoominfactor', 
	'zoomoutfactor', 
	
	'lblMisc',
	'overviewmode', 
	'follow', 
	'smoothness',
	'fov',
	--'restrictangle',
	--'mingrounddist',
	'freemode',
	
	'trackmode',
	'thirdpersontrack',
	'resetcam',

}

local OverviewAction = function() end

options = {
	
	lblblank1 = {name='', type='label'},
	lblRotate = {name='Rotation', type='label'},
	lblScroll = {name='Scrolling', type='label'},
	lblZoom = {name='Zooming', type='label'},
	lblMisc = {name='Misc.', type='label'},
	
	helpwindow = {
		name = 'COFCam Help',
		type = 'text',
		value = [[
			Complete Overhead/Free Camera has six main actions...
			
			Zoom..... <Mousewheel>
			Tilt World..... <Ctrl> + <Mousewheel>
			Altitude..... <Alt> + <Mousewheel>
			Mouse Scroll..... <Middlebutton-drag>
			Rotate World..... <Ctrl> + <Middlebutton-drag>
			Rotate Camera..... <Alt> + <Middlebutton-drag>
			
			Additional actions:
			Keyboard: <arrow keys> replicate middlebutton drag while <pgup/pgdn> replicate mousewheel. You can use these with ctrl, alt & shift to replicate mouse camera actions.
			Use <Shift> to speed up camera movements.
			Reset Camera..... <Ctrl> + <Alt> + <Shift> + <Middleclick>
		]],
	},
	smoothscroll = {
		name = 'Smooth scrolling',
		desc = 'Use smoothscroll method when mouse scrolling.',
		type = 'bool',
		value = true,
	},
	smoothmeshscroll = {
		name = 'Smooth Mesh Scrolling',
		desc = 'A smoother way to scroll. Applies to all types of mouse/keyboard scrolling.',
		type = 'bool',
		value = false,
	},
	
	targetmouse = {
		name = 'Rotate world origin at cursor',
		desc = 'Rotate world using origin at the cursor rather than the center of screen.',
		type = 'bool',
		value = false,
	},
	edgemove = {
		name = 'Scroll camera at edge',
		desc = 'Scroll camera when the cursor is at the edge of the screen.',
		springsetting = 'WindowedEdgeMove',
		type = 'bool',
		value = true,
		
	},
	speedFactor = {
		name = 'Mouse scroll speed',
		desc = 'This speed applies to scrolling with the middle button.',
		type = 'number',
		min = 10, max = 40,
		value = 25,
	},
	speedFactor_k = {
		name = 'Keyboard/edge scroll speed',
		desc = 'This speed applies to edge scrolling and keyboard keys.',
		type = 'number',
		min = 1, max = 50,
		value = 20,
	},
	zoominfactor = {
		name = 'Zoom-in speed',
		type = 'number',
		min = 0.1, max = 0.5, step = 0.05,
		value = 0.2,
	},
	zoomoutfactor = {
		name = 'Zoom-out speed',
		type = 'number',
		min = 0.1, max = 0.5, step = 0.05,
		value = 0.2,
	},
	invertzoom = {
		name = 'Invert zoom',
		desc = 'Invert the scroll wheel direction for zooming.',
		type = 'bool',
		value = true,
	},
	invertalt = {
		name = 'Invert altitude',
		desc = 'Invert the scroll wheel direction for altitude.',
		type = 'bool',
		value = false,
	},
    inverttilt = {
		name = 'Invert tilt',
		desc = 'Invert the tilt direction when using ctrl+mousewheel.',
		type = 'bool',
		value = false,
	},
    
	zoomoutfromcursor = {
		name = 'Zoom out from cursor',
		desc = 'Zoom out from the cursor rather than center of the screen.',
		type = 'bool',
		value = false,
	},
	zoomintocursor = {
		name = 'Zoom in to cursor',
		desc = 'Zoom in to the cursor rather than the center of the screen.',
		type = 'bool',
		value = true,
	},
	follow = {
		name = "Follow player's cursor",
		desc = "Follow the cursor of the player you're spectating (needs Ally Cursor widget to be on).",
		type = 'bool',
		value = false,
		path = 'Settings/Camera',
	},	
	rotfactor = {
		name = 'Rotation speed',
		type = 'number',
		min = 0.001, max = 0.020, step = 0.001,
		value = 0.005,
	},	
	rotateonedge = {
		name = "Rotate camera at edge",
		desc = "Rotate camera when the cursor is at the edge of the screen (edge scroll must be off).",
		type = 'bool',
		value = false,
	},
    
	smoothness = {
		name = 'Smoothness',
		desc = "Controls how smooth the camera moves.",
		type = 'number',
		min = 0.0, max = 0.8, step = 0.1,
		value = 0.2,
	},
	fov = {
		name = 'Field of View',
		desc = "FOV (35 deg - 100 deg). Requires restart to take effect.",
		springsetting = 'CamFreeFOV',
		type = 'number',
		min = 35, max = 100, step = 5,
		value = 45,
	},
	invertscroll = {
		name = "Invert scrolling direction",
		desc = "Invert scrolling direction (doesn't apply to smoothscroll).",
		type = 'bool',
		value = false,
	},
	restrictangle = {
		name = "Restrict Camera Angle",
		desc = "If disabled you can point the camera upward, but end up with strange camera positioning.",
		type = 'bool',
		advanced = true,
		value = true,
		OnChange = function(self) init = true; end
	},
	freemode = {
		name = "FreeMode (RISKY)",
		desc = "Be free. (USE AT YOUR OWN RISK!)",
		type = 'bool',
		advanced = true,
		value = false,
		OnChange = function(self) init = true; end,
	},
	mingrounddist = {
		name = 'Minimum Ground Distance',
		desc = 'Getting too close to the ground allows strange camera positioning.',
		type = 'number',
		advanced = true,
		min = 0, max = 100, step = 1,
		value = 1,
		OnChange = function(self) init = true; end,
	},
	
	overviewmode = {
		name = "Overview",
		desc = "Go to overview mode, then restore view to cursor position.",
		type = 'button',
		hotkey = {key='tab', mod=''},
		OnChange = function(self) OverviewAction() end,
	},
	
	trackmode = {
		name = "Enter Trackmode",
		desc = "Track the selected unit (midclick to cancel)",
		type = 'button',
        hotkey = {key='t', mod='alt+'},
		OnChange = function(self) trackmode = true; end,
	},
    
    thirdpersontrack = {
		name = "Enter 3rd Person Trackmode",
		desc = "Track the selected unit (midclick to cancel)",
		type = 'button',
		OnChange = function(self)
            Spring.SendCommands('viewfps')
            Spring.SendCommands('track')
        end,
	},
    
    
	resetcam = {
		name = "Reset Camera",
		desc = "Reset the camera position and orientation. Map a hotkey or use <Ctrl> + <Alt> + <Shift> + <Middleclick>",
		type = 'button',
        -- OnChange defined later
	},
	
	groundrot = {
		name = "Rotate When Camera Hits Ground",
		desc = "If world-rotation motion causes the camera to hit the ground, camera-rotation motion takes over. Doesn't apply in Free Mode.",
		type = 'bool',
		value = false,
	},
	
}

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local GL_LINES		= GL.LINES
local GL_GREATER	= GL.GREATER
local GL_POINTS		= GL.POINTS

local glBeginEnd	= gl.BeginEnd
local glColor		= gl.Color
local glLineWidth	= gl.LineWidth
local glVertex		= gl.Vertex
local glAlphaTest	= gl.AlphaTest
local glPointSize 	= gl.PointSize
local glTexture 	= gl.Texture
local glTexRect 	= gl.TexRect

local red   = { 1, 0, 0 }
local green = { 0, 1, 0 }
local black = { 0, 0, 0 }
local white = { 1, 1, 1 }


local spGetCameraState		= Spring.GetCameraState
local spGetCameraVectors	= Spring.GetCameraVectors
local spGetModKeyState		= Spring.GetModKeyState
local spGetMouseState		= Spring.GetMouseState
local spIsAboveMiniMap		= Spring.IsAboveMiniMap
local spSendCommands		= Spring.SendCommands
local spSetCameraState		= Spring.SetCameraState
local spSetMouseCursor		= Spring.SetMouseCursor
local spTraceScreenRay		= Spring.TraceScreenRay
local spWarpMouse			= Spring.WarpMouse
local spGetCameraDirection	= Spring.GetCameraDirection
local spSetCameraTarget		= Spring.SetCameraTarget

local abs	= math.abs
local min 	= math.min
local max	= math.max
local sqrt	= math.sqrt
local sin	= math.sin
local cos	= math.cos
local modf  = math.fmod
local asin  = math.asin

local echo = Spring.Echo

local helpText = {}
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local ls_x, ls_y, ls_z
local ls_dist, ls_have, ls_onmap
local tilting
local overview_mode, last_rx, last_ls_dist

local numcursors = 0
local cursors = {}
local lastSinglePoint = {}
local lastTwoCursors = {}
local lastThreeFingerDx = nil
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local vsx, vsy = widgetHandler:GetViewSizes()
local cx,cy = vsx * 0.5,vsy * 0.5
function widget:ViewResize(viewSizeX, viewSizeY)
	vsx = viewSizeX
	vsy = viewSizeY
	cx = vsx * 0.5
	cy = vsy * 0.5
end

local PI 			= 3.1415
--local TWOPI			= PI*2	
local HALFPI		= PI/2
--local HALFPIPLUS	= HALFPI+0.01
local HALFPIMINUS	= HALFPI-0.01


local fpsmode = false
local mx, my = 0,0
local msx, msy = 0,0
local smoothscroll = false
local springscroll = false
local lockspringscroll = false
local rotate, movekey
local move, rot = {}, {}
local key_code = {
	left 		= 276,
	right 		= 275,
	up 			= 273,
	down 		= 274,
	pageup 		= 280,
	pagedown 	= 281,
}
local keys = {
	[276] = 'left',
	[275] = 'right',
	[273] = 'up',
	[274] = 'down',
}
local icon_size = 20
local cycle = 1
local camcycle = 1
local trackcycle = 1
local hideCursor = false


local mwidth, mheight = Game.mapSizeX, Game.mapSizeZ
local mcx, mcz 	= mwidth / 2, mheight / 2
local mcy 		= Spring.GetGroundHeight(mcx, mcz)
local maxDistY = max(mheight, mwidth) * 2


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function GetDist(x1,y1,z1, x2,y2,z2)
	local d1 = x2-x1
	local d2 = y2-y1
	local d3 = z2-z1
	
	return sqrt(d1*d1 + d2*d2 + d3*d3)
end

local function explode(div,str)
  if (div=='') then return false end
  local pos,arr = 0,{}
  -- for each divider found
  for st,sp in function() return string.find(str,div,pos,true) end do
    table.insert(arr,string.sub(str,pos,st-1)) -- Attach chars left of current divider
    pos = sp + 1 -- Jump past current divider
  end
  table.insert(arr,string.sub(str,pos)) -- Attach chars right of last divider
  return arr
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------


local function MoveRotatedCam(cs, mxm, mym)
	if not cs.dy then
		return cs
	end
	
	-- forward, up, right, top, bottom, left, right
	local camVecs = spGetCameraVectors()
	local cf = camVecs.forward
	local len = sqrt((cf[1] * cf[1]) + (cf[3] * cf[3]))
	local dfx = cf[1] / len
	local dfz = cf[3] / len
	local cr = camVecs.right
	local len = sqrt((cr[1] * cr[1]) + (cr[3] * cr[3]))
	local drx = cr[1] / len
	local drz = cr[3] / len
	
	local vecDist = (- cs.py) / cs.dy
	
	local ddx = (mxm * drx) + (mym * dfx)
	local ddz = (mxm * drz) + (mym * dfz)
	
	local gx1, gz1 = cs.px + vecDist*cs.dx,			cs.pz + vecDist*cs.dz
	local gx2, gz2 = cs.px + vecDist*cs.dx + ddx,	cs.pz + vecDist*cs.dz + ddz
	
	local extra = 500
	
	if gx2 > mwidth + extra then
		ddx = mwidth + extra - gx1
	elseif gx2 < 0 - extra then
		ddx = -gx1 - extra
	end
	
	if gz2 > mheight + extra then
		ddz = mheight - gz1 + extra
	elseif gz2 < 0 - extra then
		ddz = -gz1 - extra
	end
	
	cs.px = cs.px + ddx
	cs.pz = cs.pz + ddz
	return cs
end

--Note: If the x,y is not pointing at an onmap point, this function traces a virtual ray to an
--          offmap position using the camera direction and disregards the x,y parameters. Fixme.
local function VirtTraceRay(x,y, cs)
	local _, gpos = spTraceScreenRay(x, y, true)
	
	
	if gpos then
		local gx, gy, gz = gpos[1], gpos[2], gpos[3]
		
		--gy = Spring.GetSmoothMeshHeight (gx,gz)
		
		if gx < 0 or gx > mwidth or gz < 0 or gz > mheight then
			return false, gx, gy, gz	
		else
			return true, gx, gy, gz
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

local function SetLockSpot2(cs, x, y)
	if ls_have then
		return
	end
	
	local x, y = x, y
	if not x then
		x, y = cx, cy
	end

	--local gpos
	--_, gpos = spTraceScreenRay(x, y, true)
	local onmap, gx,gy,gz = VirtTraceRay(x, y, cs)
	
	if gx then
		ls_x,ls_y,ls_z = gx,gy,gz
		local px,py,pz = cs.px,cs.py,cs.pz
		local dx,dy,dz = ls_x-px, ls_y-py, ls_z-pz
		ls_onmap = onmap
		ls_dist = sqrt(dx*dx + dy*dy + dz*dz)
		ls_have = true
	end
end


local function UpdateCam(cs)
	local cs = cs
	if not (cs.rx and cs.ry and ls_dist) then
		--return cs
		return false
	end
	
	local opp = sin(cs.rx) * ls_dist
	local alt = sqrt(ls_dist * ls_dist - opp * opp)
	cs.px = ls_x - sin(cs.ry) * alt
	cs.py = ls_y - opp
	cs.pz = ls_z - cos(cs.ry) * alt
	
	if not options.freemode.value then
		local gndheight = Spring.GetGroundHeight(cs.px, cs.pz)+5
		--gndheight = Spring.GetSmoothMeshHeight(cs.px, cs.pz)+5
		if cs.py < gndheight then
			if options.groundrot.value then
				cs.py = gndheight
			else
				return false
			end
		end
	end
	
	return cs
end

local function Zoom(zoomin, s, forceCenter)
	local zoomin = zoomin
	if options.invertzoom.value then
		zoomin = not zoomin
	end

	local cs = spGetCameraState()
	-- [[
	if
		(not forceCenter) and
		(
			(zoomin and options.zoomintocursor.value)
			or ((not zoomin) and options.zoomoutfromcursor.value)
		)
		then
		
		local onmap, gx,gy,gz = VirtTraceRay(mx, my, cs)
		
		if onmap then
			if gx then
				dx = gx - cs.px
				dy = gy - cs.py
				dz = gz - cs.pz
			else
				return false
			end
            
			local sp = (zoomin and options.zoominfactor.value or -options.zoomoutfactor.value) * (s and 3 or 1)
			
			local new_px = cs.px + dx * sp
			local new_py = cs.py + dy * sp
			local new_pz = cs.pz + dz * sp
			
			if not options.freemode.value then
                if new_py < Spring.GetGroundHeight(cs.px, cs.pz)+5 then
                    sp = (Spring.GetGroundHeight(cs.px, cs.pz)+5 - cs.py) / dy
                    new_px = cs.px + dx * sp
                    new_py = cs.py + dy * sp
                    new_pz = cs.pz + dz * sp
                elseif (not zoomin) and new_py > maxDistY then
                    sp = (maxDistY - cs.py) / dy
                    new_px = cs.px + dx * sp
                    new_py = cs.py + dy * sp
                    new_pz = cs.pz + dz * sp
                end
                
            end
			
            cs.px = new_px
            cs.py = new_py
            cs.pz = new_pz
            
			spSetCameraState(cs, options.smoothness.value)
			ls_have = false
			return
		end
		
	end
	--]]
	ls_have = false
	SetLockSpot2(cs)
	if not ls_have then
		return
	end
    
	if zoomin and not ls_onmap then
		return
	end
    
	local sp = (zoomin and -options.zoominfactor.value or options.zoomoutfactor.value) * (s and 3 or 1)
	
	local ls_dist_new = ls_dist + ls_dist*sp
	ls_dist_new = math.max(ls_dist_new, 20)
	ls_dist_new = math.min(ls_dist_new, maxDistY)
	
	ls_dist = ls_dist_new

	local cstemp = UpdateCam(cs)
	if cstemp then cs = cstemp; end
	if zoomin or ls_dist < maxDistY then
		spSetCameraState(cs, options.smoothness.value)
	end

	return true
end


local function Altitude(up, s)
	ls_have = false
	
	local up = up
	if options.invertalt.value then
		up = not up
	end
	
	local cs = spGetCameraState()
	local py = max(1, abs(cs.py) )
	local dy = py * (up and 1 or -1) * (s and 0.3 or 0.1)
	local new_py = py + dy
	if not options.freemode.value then
        if new_py < Spring.GetGroundHeight(cs.px, cs.pz)+5  then
            new_py = Spring.GetGroundHeight(cs.px, cs.pz)+5  
        elseif new_py > maxDistY then
            new_py = maxDistY 
        end
	end
    cs.py = new_py
	spSetCameraState(cs, options.smoothness.value)
	return true
end


local function ResetCam()
	local cs = spGetCameraState()
	cs.px = Game.mapSizeX/2
	cs.py = maxDistY
	cs.pz = Game.mapSizeZ/2
	cs.rx = -HALFPI
	cs.ry = PI
	spSetCameraState(cs, 1)
end
options.resetcam.OnChange = ResetCam

OverviewAction = function()
	if not overview_mode then
		
		local cs = spGetCameraState()
		SetLockSpot2(cs)
		last_ls_dist = ls_dist
		last_rx = cs.rx
		
		cs.px = Game.mapSizeX/2
		cs.py = maxDistY
		cs.pz = Game.mapSizeZ/2
		cs.rx = -HALFPI
		spSetCameraState(cs, 1)
	else
		mx, my = spGetMouseState()
		local onmap, gx, gy, gz = VirtTraceRay(mx,my)
		if gx and onmap then
			local cs = spGetCameraState()			
			cs.rx = last_rx
			ls_dist = last_ls_dist 
			ls_x = gx
			ls_z = gz
			ls_y = Spring.GetGroundHeight(ls_x, ls_z)
			ls_have = true
			local cstemp = UpdateCam(cs)
			if cstemp then cs = cstemp; end
			spSetCameraState(cs, 1)
		end
	end
	
	overview_mode = not overview_mode
end




local function RotateCamera(x, y, dx, dy, smooth, lock)
	local cs = spGetCameraState()
	local cs1 = cs
	if cs.rx then
		
		cs.rx = cs.rx + dy * options.rotfactor.value
		cs.ry = cs.ry - dx * options.rotfactor.value
		
		--local max_rx = options.restrictangle.value and -0.1 or HALFPIMINUS
		local max_rx = HALFPIMINUS
		
		if cs.rx < -HALFPIMINUS then
			cs.rx = -HALFPIMINUS
		elseif cs.rx > max_rx then
			cs.rx = max_rx 
		end
		
        -- [[
        if trackmode then
            lock = true
            ls_have = false
            SetLockSpot2(cs)
        end
		--]]
		if lock and ls_onmap then
			local cstemp = UpdateCam(cs)
			if cstemp then
				cs = cstemp;
			else
				return
			end
		else
			ls_have = false
		end
		spSetCameraState(cs, smooth and options.smoothness.value or 0)
	end
end


local function Tilt(s, dir)
	if not tilting then
		ls_have = false	
	end
	tilting = true
	local cs = spGetCameraState()
	SetLockSpot2(cs)
	if not ls_have then
		return
	end
    local dir = dir * (options.inverttilt.value and -1 or 1)
    

	local speed = dir * (s and 30 or 10)
	RotateCamera(vsx * 0.5, vsy * 0.5, 0, speed, true, true) --smooth, lock

	return true
end

local function ScrollCam(cs, mxm, mym, smoothlevel)
	SetLockSpot2(cs)
	if not cs.dy or not ls_have then
		--echo "<COFC> scrollcam fcn fail"
		return
	end
	if not ls_onmap then
		smoothlevel = 0.5
	end

	-- forward, up, right, top, bottom, left, right
	local camVecs = spGetCameraVectors()
	local cf = camVecs.forward
	local len = sqrt((cf[1] * cf[1]) + (cf[3] * cf[3]))
	local dfx = cf[1] / len
	local dfz = cf[3] / len
	local cr = camVecs.right
	local len = sqrt((cr[1] * cr[1]) + (cr[3] * cr[3]))
	local drx = cr[1] / len
	local drz = cr[3] / len
	
	local vecDist = (- cs.py) / cs.dy
	
	local ddx = (mxm * drx) + (mym * dfx)
	local ddz = (mxm * drz) + (mym * dfz)
	
	ls_x = ls_x + ddx
	ls_z = ls_z + ddz
	
	ls_x = math.min(ls_x, mwidth-3)
	ls_x = math.max(ls_x, 3)
	
	ls_z = math.min(ls_z, mheight-3)
	ls_z = math.max(ls_z, 3)
	
	if options.smoothmeshscroll.value then
		ls_y = Spring.GetSmoothMeshHeight(ls_x, ls_z) or 0
	else
		ls_y = Spring.GetGroundHeight(ls_x, ls_z) or 0
	end
	
	
	local csnew = UpdateCam(cs)
	if csnew then
        spSetCameraState(csnew, smoothlevel)
    end
	
end

local function PeriodicWarning()
	local c_widgets, c_widgets_list = '', {}
	for name,data in pairs(widgetHandler.knownWidgets) do
		if data.active and
			(
			name:find('SmoothScroll')
			or name:find('Hybrid Overhead')
			or name:find('Complete Control Camera')
			)
			then
			c_widgets_list[#c_widgets_list+1] = name
		end
	end
	for _,wname in ipairs(c_widgets_list) do
		c_widgets = c_widgets .. wname .. ', '
	end
	if c_widgets ~= '' then
		echo('<COFCam> *Periodic warning* Please disable other camera widgets: ' .. c_widgets)
	end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:Update(dt)

    if hideCursor then
        spSetMouseCursor('%none%')
    end

	if options.follow.value then
		camcycle = camcycle %(32*6) + 1
		if camcycle == 1 then
			if WG.alliedCursorsPos then 
				local teamID = Spring.GetLocalTeamID()
				local _, playerID = Spring.GetTeamInfo(teamID)
				local pp = WG.alliedCursorsPos[ playerID ]
				if pp then 
					Spring.SetCameraTarget(pp[1], 0, pp[2], 5)
				end 
			end 
		end
	end
	cycle = cycle %(32*15) + 1
	-- Periodic warning
	if cycle == 1 then
		PeriodicWarning()
	end
	
	trackcycle = trackcycle %(4) + 1
	if trackcycle == 1 and trackmode then
		local selUnits = Spring.GetSelectedUnits()
		if selUnits and selUnits[1] then
			local x,y,z = Spring.GetUnitPosition( selUnits[1] )
			Spring.SetCameraTarget(x,y,z, 0.2)
		end
	end
	

	local cs = spGetCameraState()
	
	local use_lockspringscroll = lockspringscroll and not springscroll

	local a,c,m,s = spGetModKeyState()
	
	if rot.right or rot.left or rot.up or rot.down then
		local speed = options.rotfactor.value * (s and 400 or 150)
		if rot.right then
			RotateCamera(vsx * 0.5, vsy * 0.5, speed, 0, true)
		elseif rot.left then
			RotateCamera(vsx * 0.5, vsy * 0.5, -speed, 0, true)
		end
		
		if rot.up then
			RotateCamera(vsx * 0.5, vsy * 0.5, 0, speed, true)
		elseif rot.down then
			RotateCamera(vsx * 0.5, vsy * 0.5, 0, -speed, true)
		end
		
	end
	
	if smoothscroll
		or move.right or move.left or move.up or move.down
		or use_lockspringscroll
		then
		
		local x, y, lmb, mmb, rmb = spGetMouseState()
		
		if (c) then
			return
		end
		
		local smoothlevel = 0
		
		-- clear the velocities
		cs.vx  = 0; cs.vy  = 0; cs.vz  = 0
		cs.avx = 0; cs.avy = 0; cs.avz = 0
				
		local mxm, mym = 0,0
		
		local heightFactor = (cs.py/1000)
		if smoothscroll then
			--local speed = dt * options.speedFactor.value * heightFactor 
			local speed = math.max( dt * options.speedFactor.value * heightFactor, 0.005 )
			mxm = speed * (x - cx)
			mym = speed * (y - cy)
		elseif use_lockspringscroll then
			--local speed = options.speedFactor.value * heightFactor / 10
			local speed = math.max( options.speedFactor.value * heightFactor / 10, 0.05 )
			local dir = options.invertscroll.value and -1 or 1
			mxm = speed * (x - mx) * dir
			mym = speed * (y - my) * dir
			
			spWarpMouse(cx, cy)		
		else
			--local speed = options.speedFactor_k.value * (s and 3 or 1) * heightFactor
			local speed = math.max( options.speedFactor_k.value * (s and 3 or 1) * heightFactor, 1 )
			
			if move.right then
				mxm = speed
			elseif move.left then
				mxm = -speed
			end
			
			if move.up then
				mym = speed
			elseif move.down then
				mym = -speed
			end
			smoothlevel = options.smoothness.value
		end
		
		ScrollCam(cs, mxm, mym, smoothlevel)
		
	end
	
	mx, my = spGetMouseState()
	
	if options.edgemove.value then
		if not movekey then
			move = {}
		end
		
		if mx > vsx-2 then 
			move.right = true 
		elseif mx < 2 then
			move.left = true
		end
		if my > vsy-2 then
			move.up = true
		elseif my < 2 then
			move.down = true
		end
		
	elseif options.rotateonedge.value then
		rot = {}
		if mx > vsx-2 then 
			rot.right = true 
		elseif mx < 2 then
			rot.left = true
		end
		if my > vsy-2 then
			rot.up = true
		elseif my < 2 then
			rot.down = true
		end
	end
	
	fpsmode = cs.name == "fps"
	if init or ((cs.name ~= "free") and (cs.name ~= "ov") and not fpsmode) then 
		init = false
		spSendCommands("viewfree") 
		local cs = spGetCameraState()
		cs.tiltSpeed = 0
		cs.scrollSpeed = 0
		--cs.gndOffset = options.mingrounddist.value
		cs.gndOffset = options.freemode.value and 0 or 1
		spSetCameraState(cs,0)
	end
	
end

function widget:MouseMove(x, y, dx, dy, button)
    if fpsmode then return end
	if rotate then
		if abs(dx) > 0 or abs(dy) > 0 then
			RotateCamera(x, y, dx, dy, false, ls_have)
		end
		
		spWarpMouse(msx, msy)
		
	elseif springscroll then
		
		if abs(dx) > 0 or abs(dy) > 0 then
			lockspringscroll = false
		end
		local dir = options.invertscroll.value and -1 or 1
					
		local cs = spGetCameraState()
		
		local speed = options.speedFactor.value * cs.py/1000 / 10
		local mxm = speed * dx * dir
		local mym = speed * dy * dir
		ScrollCam(cs, mxm, mym, 0)
	end
end

function totalTouchReset()
    lastSinglePoint = {}
    
    lastTwoCursors.lastMidpoint = {}
    lastTwoCursors.prevDist = nil
    lastTwoCursors.prevRot = nil
    
    lastThreeFingerDx = nil
end

function resetCursors()
    numcursors = 0
    for i, v in pairs(cursors) do
        numcursors = numcursors + 1
    end
    
    if numcursors > 0 then
        WG.MultiTouchCameraStatus.touching = true
    else
        WG.MultiTouchCameraStatus.touching = false
    end
    
    if numcursors ~= 1 then
        lastSinglePoint = {}
    end
    
    if numcursors ~= 2 then
        lastTwoCursors.lastMidpoint = {}
        lastTwoCursors.prevDist = nil
        lastTwoCursors.prevRot = nil
    end
    
    if numcursors ~= 3 then
        lastThreeFingerDx = nil
    end
end

function widget:AddCursor(x, y, id)
    if(WG.MultiTouchCameraStatus.enabled) then
        cursors[id] = {}
        cursors[id].x = x;
        --invert for screen coordinates
        cursors[id].y = vsy - y;
        cursors[id].dx = 0;
        cursors[id].dy = 0;
        resetCursors()
        return true
    end
    return false
end

function widget:UpdateCursor(x, y, dx, dy, id)
    cursors[id].x = x;
    --invert for screen coordinates
    cursors[id].y = vsy - y;
    cursors[id].dx = dx;
    --invert for screen coordinates
    cursors[id].dy = -dy;
end

function widget:RemoveCursor(x, y, dx, dy, id)
--    echo('Remove Cursor at: ' .. x .. ', ' .. y)
    cursors[id] = nil
    resetCursors()
end

function widget:RefreshCursors(seconds)
--    echo('Refresh Cursors: ' .. seconds)
    if(not WG.MultiTouchCameraStatus.enabled) then
        totalTouchReset()
    end
    
    numcursors = 0
    local firstCursor
    local secondCursor
    for i, v in pairs(cursors) do
        numcursors = numcursors + 1
        if(numcursors == 1) then
            firstCursor = v
        end
        if(numcursors == 2) then
            secondCursor = v
        end
    end
    
    --with one cursor just do panning
    if numcursors == 1 then
        if lastSinglePoint.x then
            local dx = lastSinglePoint.x - firstCursor.x
            local dy = lastSinglePoint.y - firstCursor.y
            local dist = sqrt(dx * dx + dy * dy)
            if dist >= 5 then
                translate(lastSinglePoint, firstCursor)
                lastSinglePoint.x = firstCursor.x
                lastSinglePoint.y = firstCursor.y
            end
        end
        
        if lastSinglePoint.x == nil then
            lastSinglePoint.x = firstCursor.x
            lastSinglePoint.y = firstCursor.y
        end
    end
    
    --with two cursors do pinch/rotate
    if numcursors == 2 then
        pinchZoom(firstCursor, secondCursor)
    end
    
    if numcursors == 3 then
        touchTilt()
    end
    
end

function getAngle(one, two)
    local side = one.x - two.x
    local height = one.y - two.y
    local distance = GetDist(one.x, one.y, 0, two.x, two.y, 0)
    
    local angle = asin(side / distance) + HALFPI
    
    if (height < 0) then
        angle = 2.0 * PI - angle
    end
    
    return angle
end

function normalize(vect)
    local mag = sqrt(vect.x * vect.x + vect.y * vect.y + vect.z * vect.z)
    vect.x = vect.x / mag
    vect.y = vect.y / mag
    vect.z = vect.z / mag
    vect.mag = mag
end

function CalcPixelDir(pnt, cs)
    local pos = {}
    local lastOnMap, px, py, pz = VirtTraceRay(pnt.x, pnt.y, cs);
    
    pos.x = px - cs.px
    pos.y = py - cs.py
    pos.z = pz - cs.pz
    
    normalize(pos)
    
    return pos
end

function Dot(one, two)
    return one.x * two.x + one.y * two.y + one.z * two.z
end

function makeRotationMatrix(rot, u)
    local cs = cos(rot)
    local sn = sin(rot)
    
    normalize(u)
    local mat = {}
    mat[1] = {}
    mat[2] = {}
    mat[3] = {}
    
    mat[1][1] = cs + u.x * u.x * (1 - cs)
    mat[1][2] = u.y * u.x * (1 - cs) - u.z * sn
    mat[1][3] = u.x * u.z * (1 - cs) + u.y * sn
    
    mat[2][1] = u.y * u.x * (1 - cs) + u.z * sn
    mat[2][2] = cs + u.y * u.y * (1 - cs)
    mat[2][3] = u.y * u.z * (1 - cs) - u.x * sn
    
    mat[3][1] = u.x * u.z * (1 - cs) - u.y * sn
    mat[3][2] = u.y * u.z * (1 - cs) + u.x * sn
    mat[3][3] = cs + u.z * u.z * (1 - cs)
    
    return mat
end

function mulMV(mat, vec)
    local nv = {}
    nv.x = mat[1][1] * vec.x + mat[1][2] * vec.y + mat[1][3] * vec.z
    nv.y = mat[2][1] * vec.x + mat[2][2] * vec.y + mat[2][3] * vec.z
    nv.z = mat[3][1] * vec.x + mat[3][2] * vec.y + mat[3][3] * vec.z
    
    return nv
end

--angular clamps for touchTilt
local upperRotLimit = -30.0 * PI / 180.0
local lowerRotLimit = -(PI * 0.49999)

function touchTilt()
    local lowest = {}
    local highest = {}
    lowest.y = -1
    lowest.x = 0
    highest.y = 10000
    highest.x = 0
    
    -- find lowest and highest (physically) touches
    for i, v in pairs(cursors) do
        if(v.y >= lowest.y) then
            lowest.x = v.x
            lowest.y = v.y
        end
        if(v.y <= highest.y) then
            highest.x = v.x
            highest.y = v.y
        end
    end
    
    local dX = lowest.y - highest.y
    
    if lastThreeFingerDx then
        local dDx = dX - lastThreeFingerDx
        local thresh = 5
        
        if (abs(dDx) > thresh) then
            local usedChange = math.floor(dDx / thresh) * thresh
            
            lastThreeFingerDx = lastThreeFingerDx + usedChange
            
            local rot = -1.0 / 180.0 * PI * usedChange / 5.0
            
            local cs = spGetCameraState()
            
            local rotArm = CalcPixelDir(lowest, cs)
            
            local currentRot = cs.rx
            
            --angular clamps
            if(currentRot + rot > upperRotLimit) then
                rot = upperRotLimit - currentRot
            elseif (currentRot + rot < lowerRotLimit) then
                rot = lowerRotLimit - currentRot
            end
            
            --get camera right vector
            local camVecs = spGetCameraVectors()
            local cameraRight = {}
            cameraRight.x = camVecs.right[1]; cameraRight.y = camVecs.right[2]; cameraRight.z = camVecs.right[3];
            
            local rotate = makeRotationMatrix(rot, cameraRight)
            
            local grnDist = rotArm.mag
            
            if(grnDist > 0) then
                rotArm.x = -grnDist * rotArm.x;
                rotArm.y = -grnDist * rotArm.y;
                rotArm.z = -grnDist * rotArm.z;
                
                local rotatedArm = mulMV(rotate, rotArm)
                
                cs.px = cs.px + (rotatedArm.x - rotArm.x)
                cs.py = cs.py + (rotatedArm.y - rotArm.y)
                cs.pz = cs.pz + (rotatedArm.z - rotArm.z)
                
                cs.rx = cs.rx + rot
                
                spSetCameraState(cs, 0.05);
            end
            
        end
    else
        lastThreeFingerDx = dX
    end
end

function pinchZoom(one, two)
    local midPoint = {}
    midPoint.x = (one.x + two.x) / 2
    midPoint.y = (one.y + two.y) / 2
    
    
    if lastTwoCursors.lastMidpoint.x then
        local midDist = GetDist(lastTwoCursors.lastMidpoint.x, lastTwoCursors.lastMidpoint.y, 0, midPoint.x, midPoint.y, 0)
        if(midDist > 5) then
            translate(lastTwoCursors.lastMidpoint, midPoint)
            lastTwoCursors.lastMidpoint.x = midPoint.x;
            lastTwoCursors.lastMidpoint.y = midPoint.y;
        end
    else
    
    end
    
    lastTwoCursors.lastMidpoint.x = midPoint.x;
    lastTwoCursors.lastMidpoint.y = midPoint.y;
    
    local xdif = one.x - two.x
    local ydif = one.y - two.y
    
    local dist = sqrt(xdif * xdif + ydif * ydif)
    local curRot = getAngle(one, two)
    
    if(lastTwoCursors.prevDist) then
        if midPoint.x > 0 and midPoint.x < vsx and midPoint.y > 0 and midPoint.y < vsy then
            local dDist = dist - lastTwoCursors.prevDist
            local changeUnit = 5
            local midLoc = {}
            
            local cs = spGetCameraState()
            
            local midOnMap
            midOnMap, midLoc.x, midLoc.y, midLoc.z = VirtTraceRay(midPoint.x, midPoint.y, cs)
            
            local midDir = CalcPixelDir(midPoint, cs)
            local grnDist = midDir.mag
            local newGrnDist = midDir.mag
            
            if midOnMap and abs(dDist) > changeUnit then
                
                
                local oneDir = CalcPixelDir(one, cs)
                local oldOnePos = {}
                oldOnePos.x = midPoint.x + (one.x - midPoint.x) * (lastTwoCursors.prevDist / dist)
                oldOnePos.y = midPoint.y + (one.y - midPoint.y) * (lastTwoCursors.prevDist / dist)
                
                local oldOneDir = CalcPixelDir(oldOnePos, cs)
                
                local oneDist = grnDist / Dot(oneDir, midDir)
                local oldOneDist = grnDist / Dot(oldOneDir, midDir)
                
                --float3 oneLoc = pos + oneDir * oneDist;
                local oneLoc = {}
                oneLoc.x = cs.px + oneDir.x * oneDist
                oneLoc.y = cs.py + oneDir.y * oneDist
                oneLoc.z = cs.pz + oneDir.z * oneDist
                
                --float3 oldOneLoc = pos + oldOneDir * oldOneDist;
                local oldOneLoc = {}
                oldOneLoc.x = cs.px + oldOneDir.x * oldOneDist
                oldOneLoc.y = cs.py + oldOneDir.y * oldOneDist
                oldOneLoc.z = cs.pz + oldOneDir.z * oldOneDist
                
                --float newD = (midLoc - oneLoc).Length();
                local newD = GetDist(midLoc.x, midLoc.y, midLoc.z, oneLoc.x, oneLoc.y, oneLoc.z)
                --float oldD = (midLoc - oldOneLoc).Length();
                local oldD = GetDist(midLoc.x, midLoc.y, midLoc.z, oldOneLoc.x, oldOneLoc.y, oldOneLoc.z)
                
                --float newGrnDist = oldD * grnDist / newD;
                newGrnDist = oldD * grnDist / newD
                
                cs.px = cs.px + midDir.x * (grnDist - newGrnDist)
                cs.py = cs.py + midDir.y * (grnDist - newGrnDist)
                cs.pz = cs.pz + midDir.z * (grnDist - newGrnDist)
                
                --update prevDist
                lastTwoCursors.prevDist = dist
                
            end
            
            --float dRot = curRot - prevRot;
            --dRot = streflop::fmodf(dRot, M_PI * 2);
            --float angularChangeThresh = 1.0f * M_PI / 180.0f;
            
            local dRot = curRot - lastTwoCursors.prevRot
            drot = modf(dRot, PI * 2)
            local angularChangeThresh = 3 * PI / 180.0
            
            
            
            if abs(drot) > angularChangeThresh and grnDist >= 0 then
                --float3 grndOffset = midDir * -grnDist;
                local grndOffset = {}
                grndOffset.x = midDir.x * grnDist
                grndOffset.y = midDir.y * grnDist
                grndOffset.z = midDir.z * grnDist
                
                local css = cos(dRot)
                local sn = sin(dRot)
                
                --CMatrix44f rotate;
                --rotate.Rotate(dRot, UpVector);
                --float3 newOffset = rotate.Mul(grndOffset);
                local newOffset = {}
                newOffset.x = grndOffset.x * css - grndOffset.z * sn
                newOffset.y = grndOffset.y
                newOffset.z = grndOffset.x * sn + grndOffset.z * css
                
                --newOffset -= grndOffset;
                newOffset.x = newOffset.x - grndOffset.x
                newOffset.y = newOffset.y - grndOffset.y
                newOffset.z = newOffset.z - grndOffset.z
                
                --pos += newOffset;
                cs.px = cs.px + newOffset.x
                cs.py = cs.py + newOffset.y
                cs.pz = cs.pz + newOffset.z
                
                --camera->rot.y += dRot;
                cs.ry = cs.ry + dRot
                
                --prevRot = curRot;
                lastTwoCursors.prevRot = curRot    
            end
            spSetCameraState(cs, 0.08);
        end
    else
        lastTwoCursors.prevDist = dist
        lastTwoCursors.prevRot = curRot
    end
end

function translate(last, now)
    local cs = spGetCameraState()
    
    local lastOnMap, lgx, lgy, lgz = VirtTraceRay(last.x, last.y, cs);
    local nowOnMap, ngx, ngy, ngz = VirtTraceRay(now.x, now.y, cs);
    
    if lastOnMap and nowOnMap then
        local oldDir = {}
        oldDir.x = lgx - cs.px
        oldDir.y = lgy - cs.py
        oldDir.z = lgz - cs.pz
        normalize(oldDir)
        
        local newDir = {}
        newDir.x = ngx - cs.px
        newDir.y = ngy - cs.py
        newDir.z = ngz - cs.pz
        normalize(newDir)
        
        local oldDist = -cs.py / oldDir.y
        local newDist = -cs.py / newDir.y
        
        local dpos = {}
        dpos.x = -newDir.x * newDist + oldDir.x * oldDist
        dpos.y = 0
        dpos.z = -newDir.z * newDist + oldDir.z * oldDist
        
        cs.px = cs.px + dpos.x
        cs.py = cs.py + dpos.y
        cs.pz = cs.pz + dpos.z
        
        spSetCameraState(cs, 0.03)
        
        return dpos.x, dpos.y, dpos.z
    
    
    end

end

function widget:MousePress(x, y, button)
	ls_have = false
	overview_mode = false
    --if fpsmode then return end
	if lockspringscroll then
		lockspringscroll = false
		return true
	end
	
	-- Not Middle Click --
	if (button ~= 2) then
		return false
	end
	Spring.SendCommands('trackoff')
    spSendCommands('viewfree')
	trackmode = false
	
	local a,c,m,s = spGetModKeyState()
	
	-- Reset --
	if a and c and s then
		ResetCam()
		return true
	end
	
	-- Above Minimap --
	if (spIsAboveMiniMap(x, y)) then
		return false
	end
	
	local cs = spGetCameraState()
	
	msx = x
	msy = y
	
	spSendCommands({'trackoff'})
	
	rotate = false
	-- Rotate --
	if a then
		spWarpMouse(cx, cy)
		ls_have = false
		rotate = true
		return true
	end
	-- Rotate World --
	if c then
	
		if options.targetmouse.value then
			
			local onmap, gx, gy, gz = VirtTraceRay(x,y, cs)
			if gx and onmap then
				SetLockSpot2(cs,x,y)
				
				spSetCameraTarget(gx,gy,gz, 1)
			end
		end
		spWarpMouse(cx, cy)
		SetLockSpot2(cs)
		rotate = true
		msx = cx
		msy = cy
		return true
	end
	
	-- Scrolling --
	if options.smoothscroll.value then
		spWarpMouse(cx, cy)
		smoothscroll = true
	else
		springscroll = true
		lockspringscroll = not lockspringscroll
	end
	
	return true
	
end

function widget:MouseRelease(x, y, button)
	if (button == 2) then
		rotate = nil
		smoothscroll = false
		springscroll = false
		return -1
	end
end

function widget:MouseWheel(up, value)
    if fpsmode then return end
	local a,c,m,s = spGetModKeyState()
	
	if c then
		return Tilt(s, up and 1 or -1)
	elseif a then
		return Altitude(up, s)
	end
	
	return Zoom(not up, s)
end

function widget:KeyPress(key, modifier, isRepeat)
	--ls_have = false
	tilting = false
	
	if fpsmode then return end
	if keys[key] then
		if modifier.ctrl or modifier.alt then
		
			local cs = spGetCameraState()
			SetLockSpot2(cs)
			if not ls_have then
				return
			end
			
		
			local speed = modifier.shift and 30 or 10 
			
			if key == key_code.right then 		RotateCamera(vsx * 0.5, vsy * 0.5, speed, 0, true, not modifier.alt)
			elseif key == key_code.left then 	RotateCamera(vsx * 0.5, vsy * 0.5, -speed, 0, true, not modifier.alt)
			elseif key == key_code.down then 	RotateCamera(vsx * 0.5, vsy * 0.5, 0, -speed, true, not modifier.alt)
			elseif key == key_code.up then 		RotateCamera(vsx * 0.5, vsy * 0.5, 0, speed, true, not modifier.alt)
			end
			return
		else
			movekey = true
			move[keys[key]] = true
		end
	elseif key == key_code.pageup then
		if modifier.ctrl then
			Tilt(modifier.shift, 1)
			return
		elseif modifier.alt then
			Altitude(true, modifier.shift)
			return
		else
			Zoom(true, modifier.shift, true)
			return
		end
	elseif key == key_code.pagedown then
		if modifier.ctrl then
			Tilt(modifier.shift, -1)
			return
		elseif modifier.alt then
			Altitude(false, modifier.shift)
			return
		else
			Zoom(false, modifier.shift, true)
			return
		end
	end
	tilting = false
end
function widget:KeyRelease(key)
	if keys[key] then
		move[keys[key]] = nil
	end
	if not (move.up or move.down or move.left or move.right) then
		movekey = nil
	end
end

local function DrawLine(x0, y0, c0, x1, y1, c1)
  glColor(c0); glVertex(x0, y0)
  glColor(c1); glVertex(x1, y1)
end

local function DrawPoint(x, y, c, s)
  --FIXME reenable later - ATIBUG glPointSize(s)
  glColor(c)
  glBeginEnd(GL_POINTS, glVertex, x, y)
end

function widget:DrawScreen()
    hideCursor = false
	if not cx then return end
    
	local x, y
	if smoothscroll then
		x, y = spGetMouseState()
		glLineWidth(2)
		glBeginEnd(GL_LINES, DrawLine, x, y, green, cx, cy, red)
		glLineWidth(1)
		
		DrawPoint(cx, cy, black, 14)
		DrawPoint(cx, cy, white, 11)
		DrawPoint(cx, cy, black,  8)
		DrawPoint(cx, cy, red,    5)
	
		DrawPoint(x, y, { 0, 1, 0 },  5)
	end
	
	local filefound	
	if smoothscroll or (rotate and ls_have) then
		filefound = glTexture(LUAUI_DIRNAME .. 'Images/ccc/arrows-dot.png')
	elseif rotate or lockspringscroll or springscroll then
		filefound = glTexture(LUAUI_DIRNAME .. 'Images/ccc/arrows.png')
	end
	
	if filefound then
	
		if smoothscroll then
			glColor(0,1,0,1)
		elseif (rotate and ls_have) then
			glColor(1,0.6,0,1)
		elseif rotate then
			glColor(1,1,0,1)
		elseif lockspringscroll then
			glColor(1,0,0,1)
		elseif springscroll then
			if options.invertscroll.value then
				glColor(1,0,1,1)
			else
				glColor(0,1,1,1)
			end
		end
		
		glAlphaTest(GL_GREATER, 0)
		
		if not (springscroll and not lockspringscroll) then
		    hideCursor = true
		end
		if smoothscroll then
			local icon_size2 = icon_size
			glTexRect(x-icon_size, y-icon_size2, x+icon_size, y+icon_size2)
		else
			glTexRect(cx-icon_size, cy-icon_size, cx+icon_size, cy+icon_size)
		end
		glTexture(false)

		glColor(1,1,1,1)
		glAlphaTest(false)		
	end
end


function widget:Initialize()
	helpText = explode( '\n', options.helpwindow.value )
	cx = vsx * 0.5
	cy = vsy * 0.5
	
    WG.MultiTouchCameraStatus = {}
    WG.MultiTouchCameraStatus.enabled = true
    
	spSendCommands( 'unbindaction toggleoverview' )
	spSendCommands( 'unbindaction trackmode' )
	spSendCommands( 'unbindaction track' )
end

function widget:Shutdown()
    WG.MultiTouchCameraStatus = nil
	spSendCommands{"viewta"}
	spSendCommands( 'bind any+tab toggleoverview' )
	spSendCommands( 'bind any+t track' )
	spSendCommands( 'bind ctrl+t trackmode' )
end

function widget:TextCommand(command)
	
	if command == "cofc help" then
		for i, text in ipairs(helpText) do
			echo('<COFCam['.. i ..']> '.. text)
		end
		return true
	elseif command == "cofc reset" then
		ResetCam()
		return true
	end
	return false
end   

--------------------------------------------------------------------------------
