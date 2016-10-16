--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    specialCallinHandlers.lua
--  brief:
--  author:  jK
--
--  Copyright (C) 2007-2013.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--  Some CallIns need custom handlers

local hCallInLists = handler.callInLists
local hHookFuncs   = handler.callInHookFuncs

function hHookFuncs.Shutdown()
	handler:SaveOrderList()
	handler:SaveConfigData()
	for _,f in hCallInLists.Shutdown:iter() do
		f()
	end
end


function hHookFuncs.ConfigureLayout(command)
	if (command == 'reconf') then
		handler:SendConfigData()
		return true
	elseif (command:find('togglewidget') == 1) then
		handler:Toggle(string.sub(command, 14))
		return true
	elseif (command:find('enablewidget') == 1) then
		handler:Enable(string.sub(command, 14))
		return true
	elseif (command:find('disablewidget') == 1) then
		handler:Disable(string.sub(command, 15))
		return true
	elseif (command:find('callins') == 1) then
		Spring.Log(LUA_NAME, "info", "known callins are:")
		Spring.Log(LUA_NAME, "info", "  (NOTE: This list contains a few (e.g. cause of LOS checking) unhandled CallIns, too.)")
		local o = {}
		for i,v in pairs(handler.knownCallIns) do
			local t = {}
			for j,w in pairs(v) do
				t[#t+1] = j .. "=" .. tostring(w)
			end
			o[#o+1] = ("  %-25s "):format(i .. ":") .. table.concat(t, ", ")
		end
		table.sort(o)
		for i=1,#o do
			Spring.Log(LUA_NAME, "info", o[i])
		end
		return true
	end

	if (actionHandler.TextAction(command)) then
		return true
	end

	for _,f in hCallInLists.TextCommand:iter() do
		if (f(command)) then
			return true
		end
	end

	return false
end


function hHookFuncs.Update()
	local deltaTime = (LUA_NAME == "LuaUI") and Spring.GetLastUpdateSeconds() or nil

	for _,f in hCallInLists.Update:iter() do
		f(deltaTime)
	end
end


function hHookFuncs.CommandNotify(id, params, options)
	for _,f in hCallInLists.CommandNotify:iter() do
		if (f(id, params, options)) then
			return true
		end
	end

	return false
end


function hHookFuncs.CommandsChanged()
	handler:UpdateSelection() --// for selectionchanged
	handler.inCommandsChanged = true
	handler.customCommands = {}

	for _,f in hCallInLists.CommandsChanged:iter() do
		f()
	end

	handler.inCommandsChanged = false
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--  Drawing call-ins

function hHookFuncs.ViewResize(viewGeometry)
	local vsx = viewGeometry.viewSizeX
	local vsy = viewGeometry.viewSizeY
	for _,f in hCallInLists.ViewResize:iter() do
		f(vsx, vsy, viewGeometry)
	end
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--  Keyboard call-ins

function hHookFuncs.KeyPress(key, mods, isRepeat, label, unicode)
	if (actionHandler.KeyAction(true, key, mods, isRepeat)) then
		return true
	end

	for _,f in hCallInLists.KeyPress:iter() do
		if f(key, mods, isRepeat, label, unicode) then
			return true
		end
	end

	return false
end


function hHookFuncs.KeyRelease(key, mods, label, unicode)
	if (actionHandler.KeyAction(false, key, mods, false)) then
		return true
	end

	for _,f in hCallInLists.KeyRelease:iter() do
		if f(key, mods, label, unicode) then
			return true
		end
	end

	return false
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--  Mouse call-ins

do
	local lastDrawFrame = 0
	local lastx,lasty = 0,0
	local lastWidget

	local spGetDrawFrame = Spring.GetDrawFrame

	--// local helper
	function handler:WidgetAt(x, y)
		local drawframe = spGetDrawFrame()
		if (lastDrawFrame == drawframe)and(lastx == x)and(lasty == y) then
			return lastWidget
		end

		lastDrawFrame = drawframe
		lastx = x
		lasty = y

		for it,f in hCallInLists.IsAbove:iter() do
			if f(x, y) then
				lastWidget = it.owner
				return lastWidget
			end
		end

		lastWidget = nil
		return nil
	end
end


function hHookFuncs.MousePress(x, y, button)
	local mo = handler.mouseOwner
	if (mo and mo.MousePress__) then
		SafeCallAddon(mo, "MousePress__", x, y, button)
		return true  --// already have an active press
	end

	for it,f in hCallInLists.MousePress:iter() do
		if f(x, y, button) then
			handler.mouseOwner = it.owner
			return true
		end
	end
	return false
end


function hHookFuncs.MouseMove(x, y, dx, dy, button)
	--FIXME send this event to all widgets (perhaps via a new callin PassiveMouseMove?)

	local mo = handler.mouseOwner
	if (mo) then
		return SafeCallAddon(mo, "MouseMove__", x, y, dx, dy, button)
	end
end


function hHookFuncs.MouseRelease(x, y, button)
	local mo = handler.mouseOwner
	local mx, my, lmb, mmb, rmb = Spring.GetMouseState()
	if (not (lmb or mmb or rmb)) then
		handler.mouseOwner = nil
	end

	if (not mo) then
		return -1
	end

	return SafeCallAddon(mo, "MouseRelease__", x, y, button) or -1
end


function hHookFuncs.MouseWheel(up, value)
	for _,f in hCallInLists.MouseWheel:iter() do
		if (f(up, value)) then
			return true
		end
	end
	return false
end


function hHookFuncs.IsAbove(x, y)
	return (handler:WidgetAt(x, y) ~= nil)
end


function hHookFuncs.GetTooltip(x, y)
	for it,f in hCallInLists.GetTooltip:iter() do
		if (SafeCallAddon(it.owner, "IsAbove__", x, y)) then
			local tip = f(x, y)
			if ((type(tip) == 'string') and (#tip > 0)) then
				return tip
			end
		end
	end
	return ""
end



--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--  Game call-ins

function hHookFuncs.WorldTooltip(ttType, ...)
	for _,f in hCallInLists.WorldTooltip:iter() do
		local tt = f(ttType, ...)
		if ((type(tt) == 'string') and (#tt > 0)) then
			return tt
		end
	end
end


function hHookFuncs.MapDrawCmd(playerID, cmdType, px, py, pz, ...)
	local retval = false
	for _,f in hCallInLists.MapDrawCmd:iter() do
		local takeEvent = f(playerID, cmdType, px, py, pz, ...)
		if (takeEvent) then
			retval = true
		end
	end
	return retval
end


function hHookFuncs.GameSetup(state, ready, playerStates)
	for _,f in hCallInLists.GameSetup:iter() do
		local success, newReady = f(state, ready, playerStates)
		if (success) then
			return true, newReady
		end
	end
	return false
end


function hHookFuncs.DefaultCommand(...)
	for _,f in hCallInLists.DefaultCommand:iter() do
		local result = f(...)
		if (type(result) == 'number') then
			return result
		end
	end
	return nil  --// not a number, use the default engine command
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--  RecvLuaMsg

function hHookFuncs.RecvLuaMsg(msg, playerID)
	local retval = false
	--FIXME: another actionHandler type?
	--if (actionHandler.RecvLuaMsg(msg, playerID)) then
	--	retval = true
	--end

	for _,f in hCallInLists.RecvLuaMsg:iter() do
		if f(msg, playerID) then
			retval = true
		end
	end
	return retval
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--  BlockAddon

function hHookFuncs.BlockAddon(addonName, knownInfo)
	local retval = false
	for _,f in hCallInLists.BlockAddon:iter() do
		if f(addonName, knownInfo) then
			retval = true
		end
	end
	return retval
end


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Custom SelectionChanged callin

--// local helper
local oldSelection = {}
function handler:UpdateSelection()
	local changed = false
	local newSelection = Spring.GetSelectedUnits()
	if (#newSelection == #oldSelection) then
		for i=1, #newSelection do
			if (newSelection[i] ~= oldSelection[i]) then --// it seems the order stays
				changed = true
				break
			end
		end
	else
		changed = true
	end
	if (changed) then
		handler:SelectionChanged(newSelection)
	end
	oldSelection = newSelection
end


function hHookFuncs.SelectionChanged(selectedUnits)
	for _,f in hCallInLists.SelectionChanged:iter() do
		local unitArray = f(selectedUnits)
		if (unitArray) then
			Spring.SelectUnitArray(unitArray)
			selectedUnits = unitArray
		end
	end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- LuaMenu callin

function hHookFuncs.AllowDraw()
	for _,f in hCallInLists.AllowDraw:iter() do
		if not f() then
			return false
		end
	end

	return true
end

