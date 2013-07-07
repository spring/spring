--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    table.lua
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
-- Table functions

function tcopy(t1, t2)
	--FIXME recursive?
	for i,v in pairs(t2) do
		t1[i] = v
	end
end

function tappend(t1, t2)
	for i=1,#t2 do
		t1[#t1+1] = t2[i]
	end
end

function tfind(t, item)
	if (not t)or(item == nil) then return false end
	for i=1,#t do
		if t[i] == item then
			return true
		end
	end
	return false
end

function tprinttable(t, columns)
	local formatstr = "  " .. string.rep("%-25s, ", columns)
	for i=1, #t, columns do
		if (i+columns > #t) then
			formatstr = "  " .. string.rep("%-25s, ", #t - i - 1) .. "%-25s"
		end
		local s = formatstr:format(select(i,unpack(t)))
		Spring.Echo("  " .. s)
	end
end


local type  = type
local pairs = pairs
function CopyTable(outtable,intable)
	for i,v in pairs(intable) do
		if (type(v)=='table') then
			if (type(outtable[i])~='table') then outtable[i] = {} end
			CopyTable(outtable[i],v)
		else
			outtable[i] = v
		end
	end
end
local CopyTable = CopyTable
tmergein = CopyTable

function tmerge(table1,table2)
	local ret = {}
	CopyTable(ret,table2)
	CopyTable(ret,table1)
	return ret
end
