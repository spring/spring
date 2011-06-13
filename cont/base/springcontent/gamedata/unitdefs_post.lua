--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

for name, ud in pairs(UnitDefs) do
	if (ud.description) then
		-- Fix the EE unit descriptions
		ud.description = string.gsub(ud.description, '^: ', '')
	end

	-- convert from the pre-0.83 representation (pieceTrailCEGTag, pieceTrailCEGRange)
	if (ud.piecetrailcegtag ~= nil) then
		if (ud.sfxtypes == nil) then
			ud.sfxtypes = {}
		end

		if (ud.piecetrailcegrange == nil) then
			ud.piecetrailcegrange = 1
		end

		ud.sfxtypes["pieceExplosionGenerators"] = {}

		for n = 1, ud.piecetrailcegrange do
			ud.sfxtypes["pieceExplosionGenerators"][n] = ud.piecetrailcegtag .. (n - 1)
		end
	end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
