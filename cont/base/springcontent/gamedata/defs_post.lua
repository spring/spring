-- filter out unused movedefs

local usedMovedefs = {}
for name, def in pairs (DEFS.unitDefs) do
	local movedef = def.movementclass
	if movedef then
		usedMovedefs[movedef] = true
	end
end

local i = 1
while i <= #DEFS.moveDefs do
	if not usedMovedefs[DEFS.moveDefs[i].name] and not DEFS.moveDefs[i].dont_remove then
		DEFS.moveDefs[i] = DEFS.moveDefs[#DEFS.moveDefs]
		DEFS.moveDefs[#DEFS.moveDefs] = nil
	else
		i = i + 1
	end
end

return {}

