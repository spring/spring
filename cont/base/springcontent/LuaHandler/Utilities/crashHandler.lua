--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    crashHandler.lua
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
-- Addon Crash Handlers

--// small helper
local isDrawCallIn = setmetatable({}, {__index = function(self,ciName)
	self[ciName] = ((ciName:sub(1, 4) == 'Draw')or(ciName:sub(1, 9) == 'TweakDraw'));
	return self[ciName];
end})


local function HandleError(addon, funcName, status, ...)
	if (status) then
		--// no error
		return ...
	end

	handler:Remove(addon, "crash")

	local name = addon._info.name
	local err  = select(1,...)
	Spring.Log(LUA_NAME, "error", ('In %s(): %s'):format(funcName, tostring(err)))
	Spring.Log(LUA_NAME, "error", ('Removed %s: %s'):format(handler.addonName, handler:GetFancyString(name)))
	return nil
end


local function HandleErrorGL(addon, funcName, status, ...)
	glPopAttrib()
	--gl.PushMatrix()
	return HandleError(addon, funcName, status, ...)
end


local function SafeWrapFuncNoGL(addon, func, funcName)
	return function(...)
		return HandleError(addon, funcName, pcall(func, ...))
	end
end


local function SafeWrapFuncGL(addon, func, funcName)
	return function(...)
		glPushAttrib()
		--gl.PushMatrix()
		return HandleErrorGL(addon, funcName, pcall(func, ...))
	end
end


function SafeWrapFunc(addon, func, funcName)
	if (SAFEWRAP <= 0) then
		return func
	elseif (SAFEWRAP == 1) then
		if (addon._info.unsafe) then
			return func
		end
	end

	if (not SAFEDRAW) then
		return SafeWrapFuncNoGL(addon, func, funcName)
	else
		if (isDrawCallIn[funcName]) then
			return SafeWrapFuncGL(addon, func, funcName)
		else
			return SafeWrapFuncNoGL(addon, func, funcName)
		end
	end
end


function SafeCallAddon(addon, ciName, ...)
	local f = addon[ciName]
	if (not f) then
		return
	end

	local ki = addon._info
	if (SAFEWRAP <= 0)or
		((SAFEWRAP == 1)and(ki and ki.unsafe))
	then
		return f(addon, ...)
	end

	if (SAFEDRAW and isDrawCallIn[ciName]) then
		glPushAttrib()
		return HandleErrorGL(addon, ciName, pcall(f, addon, ...))
	else
		return HandleError(addon, ciName, pcall(f, addon, ...))
	end
end
