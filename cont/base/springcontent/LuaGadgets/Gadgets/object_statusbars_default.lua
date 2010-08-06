if (gadgetHandler:IsSyncedCode()) then

		local GetAllFeatures     = Spring.GetAllFeatures
		local GetFeatureAllyTeam = Spring.GetFeatureAllyTeam

		function gadget:Initialize()
		end

		function gadget:UnitCreated(unitID, unitDefID, teamID, builderID)
			SendToUnsynced("UnitCreated", unitID)
		end
		function gadget:UnitDestroyed(unitID, unitDefID, teamID, attackerID, attackerDefID, attackerTeamID)
			SendToUnsynced("UnitDestroyed", unitID)
		end

		function gadget:FeatureCreated(featureID, allyTeamID)
			SendToUnsynced("FeatureCreated", featureID)
		end
		function gadget:FeatureDestroyed(featureID, allyTeamID)
			SendToUnsynced("FeatureDestroyed", featureID)
		end

		function gadget:GameFrame(n)
			if (n == 0) then
				-- map features get created before the LuaRules environment is up, so
				-- we trigger the events ourselves (we can not do this in Initialize)
				local features = GetAllFeatures()
				local featureID = -1

				for i = 1, #features do
					featureID = features[i]
					self:FeatureCreated(featureID, GetFeatureAllyTeam(featureID))
				end
			end
		end
else

		local callinHandlers = {}

		local drawDistSq            = 0
		local drawUnitStatusBars    = false
		local drawFeatureStatusBars = false

		local drawnUnitIDs    = {}
		local drawnFeatureIDs = {}

		local m_min    = math.min
		local m_max    = math.max
		local m_bit_or = math.bit_or

		local Echo               = Spring.Echo
		local GetConfigInt       = Spring.GetConfigInt
		local SetConfigInt       = Spring.SetConfigInt
		local GetCameraPosition  = Spring.GetCameraPosition
		local SetUnitLuaDraw     = Spring.UnitRendering.SetUnitLuaDraw
		local SetFeatureLuaDraw  = Spring.UnitRendering.SetFeatureLuaDraw
		local GetUnitPosition    = Spring.GetUnitPosition
		local GetFeaturePosition = Spring.GetFeaturePosition
		local GetUnitVelocity    = Spring.GetUnitVelocity
		local GetFrameTimeOffset = Spring.GetFrameTimeOffset
		local GetUnitTeam        = Spring.GetUnitTeam
		local GetUnitAllyTeam    = Spring.GetUnitAllyTeam
		local GetLocalTeamID     = Spring.GetLocalTeamID
		local GetMyAllyTeamID    = Spring.GetMyAllyTeamID
		local GetSpectatingState = Spring.GetSpectatingState

		local GetUnitDefID        = Spring.GetUnitDefID
		local GetUnitHealth       = Spring.GetUnitHealth
		local GetUnitExperience   = Spring.GetUnitExperience
		local GetUnitIsStunned    = Spring.GetUnitIsStunned
		local GetUnitStockpile    = Spring.GetUnitStockpile
		local GetUnitGroup        = Spring.GetUnitGroup
		local GetUnitHeight       = Spring.GetUnitHeight
		local GetFeatureHeight    = Spring.GetFeatureHeight
		local GetFeatureResources = Spring.GetFeatureResources
		local GetFeatureHealth    = Spring.GetFeatureHealth

		local glPushMatrix   = gl.PushMatrix
		local glPopMatrix    = gl.PopMatrix
		local glLoadIdentity = gl.LoadIdentity
		local glTranslate    = gl.Translate
		local glBillboard    = gl.Billboard
		local glPushAttrib   = gl.PushAttrib
		local glPopAttrib    = gl.PopAttrib
		local glDepthTest    = gl.DepthTest
		local glColor        = gl.Color
		local glRect         = gl.Rect
		local glText         = gl.Text
		local glFont         = gl.Font -- table
		local GL_CURRENT_BIT = GL.CURRENT_BIT
		local GL_ENABLE_BIT  = GL.ENABLE_BIT


		local function ___ToggleDrawUnitStatusBars()
			drawUnitStatusBars = not drawUnitStatusBars
			drawStatusString = (drawUnitStatusBars and "enabled") or "disabled"

			Echo("[object_statusbars_default] unit status-bar drawing " .. drawStatusString)
		end
		local function ___ToggleDrawFeatureStatusBars()
			drawFeatureStatusBars = not drawFeatureStatusBars
			drawStatusString = (drawFeatureStatusBars and "enabled") or "disabled"

			Echo("[object_statusbars_default] feature status-bar drawing " .. drawStatusString)
		end

		local function ___UnitCreated(_asa_, unitID)   SetUnitLuaDraw(unitID, true)   end
		local function ___UnitDestroyed(_asa_, unitID)   SetUnitLuaDraw(unitID, false)   end
		local function ___FeatureCreated(_asa_, featureID)   SetFeatureLuaDraw(featureID, true)   end
		local function ___FeatureDestroyed(_asa_, featureID)   SetFeatureLuaDraw(featureID, false)   end


		local function ___DrawUnitStatusBars(unitID)
			local unitDefID = GetUnitDefID(unitID)
			local unitDef = UnitDefs[unitDefID or -1]

			local hideDamage = (unitDef and unitDef.hideDamage) or false
			local _, specFullView, _ = GetSpectatingState()

			-- hide bars for non-allied units in LOS if not a global spectator
			if ((GetMyAllyTeamID() ~= GetUnitAllyTeam(unitID)) and (not specFullView) and hideDamage) then
				return
			end

			local exp, limExp = GetUnitExperience(unitID)
			local health, maxHealth, paraDmg, captProg, buildProg = GetUnitHealth(unitID)

			if (health < maxHealth or paraDmg > 0.0) then
				-- black background for healthbar
				glColor(0.0, 0.0, 0.0)
				glRect(-5.0, 4.0, 5.0, 6.0)

				-- health & stun level
				local rhealth = m_max(0.0, health / maxHealth)
				local rstun = m_min(1.0, paraDmg / maxHealth)
				local hsmin = m_min(rhealth, rstun)

				colR = m_max(0.0, 2.0 - 2.0 * rhealth)
				colG = m_min(2.0 * rhealth, 1.0)

				if (hsmin > 0.0) then
					hscol = 0.8 - 0.5 * hsmin
					hsmin = hsmin * 10.0

					glColor(colR * hscol, colG * hscol, 1.0)
					glRect(-5.0, 4.0, hsmin - 5.0, 6.0)
				end
				if (rhealth >= rstun) then
					glColor(colR, colG, 0.0)
					glRect(hsmin - 5.0, 4.0, rhealth * 10.0 - 5.0, 6.0)
				else
					glColor(0.0, 0.0, 1.0)
					glRect(hsmin - 5.0, 4.0, rstun * 10.0 - 5.0, 6.0)
				end
			end

			-- skip the rest of the indicators if it isn't a local unit
			if ((GetLocalTeamID() ~= GetUnitTeam(unitID)) and (not specFullView)) then
				return
			end

			local groupID = GetUnitGroup(unitID)
			local _, _, beingBuilt = GetUnitIsStunned(unitID)
			local _, _, stockPileBuildPercent = GetUnitStockpile(unitID)

			-- experience bar
			local eEnd = (limExp * 0.8) * 10.0
			local bEnd = (buildProg and ((buildProg * 0.8) * 10.0)) or 0.0
			local sEnd = (stockPileBuildPercent and ((stockPileBuildPercent * 0.8) * 10.0)) or 0.0

			glColor(1.0, 1.0, 1.0)
			glRect(6.0, -2.0, 8.0, eEnd - 2.0)

			if (beingBuilt) then
				glColor(1.0, 0.0, 0.0)
				glRect(-8.0, -2.0, -6.0, bEnd - 2.0)
			elseif (stockPileBuildPercent ~= nil) then
				glColor(1.0, 0.0, 0.0)
				glRect(-8.0, -2.0, -6.0, sEnd - 2.0)
			end

			if (groupID ~= nil) then
				glColor(1.0, 1.0, 1.0, 1.0)
				glText(("" .. groupID), 8.0, 10.0,  12.0, "x")   -- "x" == FONT_BASELINE
			end
		end

		local function ___DrawFeatureStatusBars(featureID)
			-- reclaimLeft, resurrectProgress
			local _, _, _, _, recl = GetFeatureResources(featureID)
			local _, _, resp = GetFeatureHealth(featureID)

			if (recl < 1.0 or resp > 0.0) then
				-- black background for the bar
				glColor(0.0, 0.0, 0.0)
				glRect(-5.0, 4.0, 5.0, 6.0)

				-- rez/metalbar
				rmin = m_min(recl, resp) * 10.0
				if (rmin > 0.0) then
					glColor(1.0, 0.0, 1.0)
					glRect(-5.0, 4.0, rmin - 5.0, 6.0)
				end
				if (recl > resp) then
					col = 0.8 - 0.3 * recl
					glColor(col, col, col)
					glRect(rmin - 5.0, 4.0, recl * 10.0 - 5.0, 6.0)
				end
				if (recl < resp) then
					glColor(0.5, 0.0, 1.0)
					glRect(rmin - 5.0, 4.0, resp * 10.0 - 5.0, 6.0)
				end

				glColor(1.0, 1.0, 1.0)
			end
		end



		function gadget:GetInfo()
			return {
				name    = "object_statusbars_default (v1.0)",
				desc    = "draws default unit and feature status-bars",
				author  = "Kloot",
				date    = "August 2, 2010",
				license = "GPL v2",
				layer   = -99999999, -- other gadgets could block the Draw* callins
				enabled = true,
			}
		end

		function gadget:Initialize()
			drawUnitStatusBars    = (GetConfigInt("ShowHealthBars", 1) ~= 0)
			drawFeatureStatusBars = (GetConfigInt("ShowRezBars",    1) ~= 0)

			drawDistSq = GetConfigInt("UnitLodDist", 1000)
			drawDistSq = drawDistSq * drawDistSq

			callinHandlers["UnitCreated"]      = ___UnitCreated
			callinHandlers["UnitDestroyed"]    = ___UnitDestroyed
			callinHandlers["FeatureCreated"]   = ___FeatureCreated
			callinHandlers["FeatureDestroyed"] = ___FeatureDestroyed

			-- listen to "/luarules show*"
			gadgetHandler:AddChatAction("showhealthbars", ___ToggleDrawUnitStatusBars, "toggle whether unit status-bars are drawn")
			gadgetHandler:AddChatAction("showrezbars", ___ToggleDrawFeatureStatusBars, "toggle whether feature status-bars are drawn")

			for funcName, func in pairs(callinHandlers) do
				gadgetHandler:AddSyncAction(funcName, func, "")
			end

			Spring.SendCommands({"showhealthbars 0", "showrezbars 0"})
		end

		function gadget:Shutdown()
			SetConfigInt("ShowHealthBars", (drawUnitStatusBars    and 1) or 0)
			SetConfigInt("ShowRezBars",    (drawFeatureStatusBars and 1) or 0)

			for funcName, func in pairs(callinHandlers) do
				gadgetHandler:RemoveSyncAction(funcName)
			end
		end


		--[[
		-- less efficient than the AddSyncAction route
		function gadget:RecvFromSynced(callinName, objectID)
			local handler = callinHandlers[callinName]

			if (handler) then
				handler(objectID)
			end
		end
		--]]



		function gadget:DrawUnit(unitID, drawMode)
			-- skip the reflection pass
			if (drawUnitStatusBars and drawMode ~= 3) then
				drawnUnitIDs[unitID] = true
			end

			return false
		end

		function gadget:DrawFeature(featureID, drawMode)
			-- skip the reflection pass
			if (drawFeatureStatusBars and drawMode ~= 3) then
				drawnFeatureIDs[featureID] = true
			end

			return false
		end



		function gadget:DrawWorld()
			if ((not drawUnitStatusBars) and (not drawFeatureStatusBars)) then
				return
			end

			local cx, cy, cz = GetCameraPosition()

			-- we can't draw directly in the Draw{Unit, Feature}
			-- callins (engine shaders are active at that point)
			glPushAttrib(m_bit_or(GL_CURRENT_BIT, GL_ENABLE_BIT))
			glDepthTest(true)
			glPushMatrix()
				if (drawUnitStatusBars) then
					for unitID, _ in pairs(drawnUnitIDs) do
						local px, py, pz = GetUnitPosition(unitID)
						local vx, vy, vz = GetUnitVelocity(unitID)
						local dx, dy, dz = ((px - cx) * (px - cx)), ((py - cy) * (py - cy)), ((pz - cz) * (pz - cz))

						if ((dx + dy + dz) < (drawDistSq * 500)) then
							-- note: better add a GetUnitDrawPosition callout
							local t = GetFrameTimeOffset()
							local h = GetUnitHeight(unitID) + 5.0

							glPushMatrix()
								glTranslate(px + vx * t, py + h + vy * t, pz + vz * t)
								glBillboard()
								___DrawUnitStatusBars(unitID)
							glPopMatrix()
						end

						drawnUnitIDs[unitID] = nil
					end
				end

				if (drawFeatureStatusBars) then
					for featureID, _ in pairs(drawnFeatureIDs) do
						local px, py, pz = GetFeaturePosition(featureID)
						local h = GetFeatureHeight(featureID) + 5.0

						glPushMatrix()
							glTranslate(px, py + h, pz)
							glBillboard()
							___DrawFeatureStatusBars(featureID)
						glPopMatrix()

						drawnFeatureIDs[featureID] = nil
					end
				end

			glPopMatrix()
			glPopAttrib()
		end
end
