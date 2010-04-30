--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    scanutils.lua
--  brief:   contains functions for use when scanning for modinfo tdfs
--  author:  Dave Rodgers, Craig Lawrence
--
--  Copyright (C) 2008.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local function MakeArray(t, prefix)
	if (type(t) ~= 'table') then
		return nil
	end

	local sorted = {}
	for k,v in pairs(t) do
		if ((type(k) == 'string') and (type(v) == 'string')) then
			local s, e, p, num = k:find('(%D*)(%d*)')
			num = tonumber(num)
			if ((p == prefix) and num) then
				table.insert(sorted, { num, v })
			end
		end
	end
	table.sort(sorted, function(a, b)
		if (a[1] < b[1]) then return true  end
		if (b[1] < a[1]) then return false end
		return (a[2] < b[2])
	end)

	local newTable = {}
	for _,numVal in ipairs(sorted) do
		table.insert(newTable, numVal[2])
	end
	return newTable
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
