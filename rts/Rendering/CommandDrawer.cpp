/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CommandDrawer.h"
#include "LineDrawer.h"
#include "Game/GameHelper.h"
#include "Game/UI/CommandColors.h"
#include "Game/WaitCommandsAI.h"
#include "Map/Ground.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/CommandAI/CommandQueue.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/AirCAI.h"
#include "Sim/Units/CommandAI/BuilderCAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Units/CommandAI/MobileCAI.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "System/myMath.h"
#include "System/Log/ILog.h"

static const CUnit* GetTrackableUnit(const CUnit* caiOwner, const CUnit* cmdUnit)
{
	if (cmdUnit == nullptr)
		return nullptr;
	if ((cmdUnit->losStatus[caiOwner->allyteam] & (LOS_INLOS | LOS_INRADAR)) == 0)
		return nullptr;

	return cmdUnit;
}

CommandDrawer* CommandDrawer::GetInstance() {
	// luaQueuedUnitSet gets cleared each frame, so this is fine wrt. reloading
	static CommandDrawer drawer;
	return &drawer;
}



void CommandDrawer::Draw(const CCommandAI* cai) const {
	// note: {Air,Builder}CAI inherit from MobileCAI, so test that last
	if ((dynamic_cast<const     CAirCAI*>(cai)) != nullptr) {     DrawAirCAICommands(static_cast<const     CAirCAI*>(cai)); return; }
	if ((dynamic_cast<const CBuilderCAI*>(cai)) != nullptr) { DrawBuilderCAICommands(static_cast<const CBuilderCAI*>(cai)); return; }
	if ((dynamic_cast<const CFactoryCAI*>(cai)) != nullptr) { DrawFactoryCAICommands(static_cast<const CFactoryCAI*>(cai)); return; }
	if ((dynamic_cast<const  CMobileCAI*>(cai)) != nullptr) {  DrawMobileCAICommands(static_cast<const  CMobileCAI*>(cai)); return; }

	DrawCommands(cai);
}



void CommandDrawer::AddLuaQueuedUnit(const CUnit* unit) {
	// needs to insert by id, pointers can become dangling
	luaQueuedUnitSet.insert(unit->id);
}

void CommandDrawer::DrawLuaQueuedUnitSetCommands() const
{
	if (luaQueuedUnitSet.empty())
		return;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	lineDrawer.Configure(cmdColors.UseColorRestarts(),
	                     cmdColors.UseRestartColor(),
	                     cmdColors.restart,
	                     cmdColors.RestartAlpha());
	lineDrawer.SetupLineStipple();

	glEnable(GL_BLEND);
	glBlendFunc((GLenum)cmdColors.QueuedBlendSrc(),
	            (GLenum)cmdColors.QueuedBlendDst());

	glLineWidth(cmdColors.QueuedLineWidth());

	for (auto ui = luaQueuedUnitSet.cbegin(); ui != luaQueuedUnitSet.cend(); ++ui) {
		const CUnit* unit = unitHandler->GetUnit(*ui);

		if (unit == nullptr || unit->commandAI == nullptr)
			continue;

		Draw(unit->commandAI);
	}

	glLineWidth(1.0f);
	glEnable(GL_DEPTH_TEST);
}

void CommandDrawer::DrawCommands(const CCommandAI* cai) const
{
	const CUnit* owner = cai->owner;
	const CCommandQueue& commandQue = cai->commandQue;

	lineDrawer.StartPath(owner->GetObjDrawMidPos(), cmdColors.start);

	if (owner->selfDCountdown != 0) {
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);
	}

	for (auto ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		const int cmdID = ci->GetID();

		switch (cmdID) {
			case CMD_ATTACK:
			case CMD_MANUALFIRE: {
				if (ci->params.size() == 1) {
					const CUnit* unit = GetTrackableUnit(owner, unitHandler->GetUnit(ci->params[0]));

					if (unit != nullptr) {
						lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.attack);
					}
				} else {
					assert(ci->params.size() >= 3);

					const float x = ci->params[0];
					const float z = ci->params[2];
					const float y = CGround::GetHeightReal(x, z, false) + 3.0f;

					lineDrawer.DrawLineAndIcon(cmdID, float3(x, y, z), cmdColors.attack);
				}
				break;
			}
			case CMD_WAIT: {
				DrawWaitIcon(*ci);
				break;
			}
			case CMD_SELFD: {
				lineDrawer.DrawIconAtLastPos(cmdID);
				break;
			}
			default: {
				DrawDefaultCommand(*ci, owner);
				break;
			}
		}
	}

	lineDrawer.FinishPath();
}



void CommandDrawer::DrawAirCAICommands(const CAirCAI* cai) const
{
	const CUnit* owner = cai->owner;
	const CCommandQueue& commandQue = cai->commandQue;

	lineDrawer.StartPath(owner->GetObjDrawMidPos(), cmdColors.start);

	if (owner->selfDCountdown != 0) {
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);
	}

	for (auto ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		const int cmdID = ci->GetID();

		switch (cmdID) {
			case CMD_MOVE: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.move);
				break;
			}
			case CMD_FIGHT: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.fight);
				break;
			}
			case CMD_PATROL: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.patrol);
				break;
			}
			case CMD_ATTACK: {
				if (ci->params.size() == 1) {
					const CUnit* unit = GetTrackableUnit(owner, unitHandler->GetUnit(ci->params[0]));

					if (unit != nullptr) {
						lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.attack);
					}
				} else {
					assert(ci->params.size() >= 3);

					const float x = ci->params[0];
					const float z = ci->params[2];
					const float y = CGround::GetHeightReal(x, z, false) + 3.0f;

					lineDrawer.DrawLineAndIcon(cmdID, float3(x, y, z), cmdColors.attack);
				}

				break;
			}
			case CMD_AREA_ATTACK: {
				const float3& endPos = ci->GetPos(0);

				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.attack);
				lineDrawer.Break(endPos, cmdColors.attack);
				glColor4fv(cmdColors.attack);
				glSurfaceCircle(endPos, ci->params[3], 20.0f);
				lineDrawer.RestartWithColor(cmdColors.attack);
				break;
			}
			case CMD_GUARD: {
				const CUnit* unit = GetTrackableUnit(owner, unitHandler->GetUnit(ci->params[0]));

				if (unit != nullptr) {
					lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.guard);
				}
				break;
			}
			case CMD_WAIT: {
				DrawWaitIcon(*ci);
				break;
			}
			case CMD_SELFD: {
				lineDrawer.DrawIconAtLastPos(cmdID);
				break;
			}
			default: {
				DrawDefaultCommand(*ci, owner);
				break;
			}
		}
	}

	lineDrawer.FinishPath();
}



void CommandDrawer::DrawBuilderCAICommands(const CBuilderCAI* cai) const
{
	const CUnit* owner = cai->owner;
	const CCommandQueue& commandQue = cai->commandQue;

	lineDrawer.StartPath(owner->GetObjDrawMidPos(), cmdColors.start);

	if (owner->selfDCountdown != 0) {
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);
	}

	for (auto ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		const int cmdID = ci->GetID();

		if (cmdID < 0) {
			if (cai->buildOptions.find(cmdID) != cai->buildOptions.end()) {
				BuildInfo bi;
				bi.def = unitDefHandler->GetUnitDefByID(-(cmdID));

				if (ci->params.size() == 4) {
					bi.buildFacing = abs((int)ci->params[3]) % NUM_FACINGS;
				}

				bi.pos = float3(ci->params[0], ci->params[1], ci->params[2]);
				bi.pos = CGameHelper::Pos2BuildPos(bi, false);

				cursorIcons.AddBuildIcon(cmdID, bi.pos, owner->team, bi.buildFacing);
				lineDrawer.DrawLine(bi.pos, cmdColors.build);

				// draw metal extraction range
				if (bi.def->extractRange > 0) {
					lineDrawer.Break(bi.pos, cmdColors.build);
					glColor4fv(cmdColors.rangeExtract);
					glSurfaceCircle(bi.pos, bi.def->extractRange, 40.0f);
					lineDrawer.Restart();
				}
			}
			continue;
		}

		switch (cmdID) {
			case CMD_MOVE: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.move);
				break;
			}
			case CMD_FIGHT:{
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.fight);
				break;
			}
			case CMD_PATROL: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.patrol);
				break;
			}
			case CMD_GUARD: {
				const CUnit* unit = GetTrackableUnit(owner, unitHandler->GetUnit(ci->params[0]));

				if (unit != nullptr) {
					lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.guard);
				}
				break;
			}
			case CMD_RESTORE: {
				const float3& endPos = ci->GetPos(0);

				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.restore);
				lineDrawer.Break(endPos, cmdColors.restore);
				glColor4fv(cmdColors.restore);
				glSurfaceCircle(endPos, ci->params[3], 20.0f);
				lineDrawer.RestartWithColor(cmdColors.restore);
				break;
			}
			case CMD_ATTACK:
			case CMD_MANUALFIRE: {
				if (ci->params.size() == 1) {
					const CUnit* unit = GetTrackableUnit(owner, unitHandler->GetUnit(ci->params[0]));

					if (unit != nullptr) {
						lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.attack);
					}
				} else {
					assert(ci->params.size() >= 3);

					const float x = ci->params[0];
					const float z = ci->params[2];
					const float y = CGround::GetHeightReal(x, z, false) + 3.0f;

					lineDrawer.DrawLineAndIcon(cmdID, float3(x, y, z), cmdColors.attack);
				}

				break;
			}
			case CMD_RECLAIM:
			case CMD_RESURRECT: {
				const float* color = (cmdID == CMD_RECLAIM) ? cmdColors.reclaim
				                                             : cmdColors.resurrect;
				if (ci->params.size() == 4) {
					const float3& endPos = ci->GetPos(0);

					lineDrawer.DrawLineAndIcon(cmdID, endPos, color);
					lineDrawer.Break(endPos, color);
					glColor4fv(color);
					glSurfaceCircle(endPos, ci->params[3], 20.0f);
					lineDrawer.RestartWithColor(color);
				} else {
					assert(ci->params[0] >= 0.0f);

					const unsigned int id = std::max(0.0f, ci->params[0]);

					if (id >= unitHandler->MaxUnits()) {
						const CFeature* feature = featureHandler->GetFeature(id - unitHandler->MaxUnits());

						if (feature != nullptr) {
							lineDrawer.DrawLineAndIcon(cmdID, feature->GetObjDrawMidPos(), color);
						}
					} else {
						const CUnit* unit = GetTrackableUnit(owner, unitHandler->GetUnit(ci->params[0]));

						if (unit != nullptr && unit != owner) {
							lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), color);
						}
					}
				}
				break;
			}
			case CMD_REPAIR:
			case CMD_CAPTURE: {
				const float* color = (ci->GetID() == CMD_REPAIR) ? cmdColors.repair: cmdColors.capture;
				if (ci->params.size() == 4) {
					const float3& endPos = ci->GetPos(0);

					lineDrawer.DrawLineAndIcon(cmdID, endPos, color);
					lineDrawer.Break(endPos, color);
					glColor4fv(color);
					glSurfaceCircle(endPos, ci->params[3], 20.0f);
					lineDrawer.RestartWithColor(color);
				} else {
					if (ci->params.size() >= 1) {
						const CUnit* unit = GetTrackableUnit(owner, unitHandler->GetUnit(ci->params[0]));

						if (unit != nullptr) {
							lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), color);
						}
					}
				}
				break;
			}
			case CMD_LOAD_ONTO: {
				const CUnit* unit = unitHandler->GetUnitUnsafe(ci->params[0]);
				lineDrawer.DrawLineAndIcon(cmdID, unit->pos, cmdColors.load);
				break;
			}
			case CMD_WAIT: {
				DrawWaitIcon(*ci);
				break;
			}
			case CMD_SELFD: {
				lineDrawer.DrawIconAtLastPos(ci->GetID());
				break;
			}
			default: {
				DrawDefaultCommand(*ci, owner);
				break;
			}
		}
	}

	lineDrawer.FinishPath();
}



void CommandDrawer::DrawFactoryCAICommands(const CFactoryCAI* cai) const
{
	const CUnit* owner = cai->owner;
	const CCommandQueue& commandQue = cai->commandQue;
	const CCommandQueue& newUnitCommands = cai->newUnitCommands;

	lineDrawer.StartPath(owner->GetObjDrawMidPos(), cmdColors.start);

	if (owner->selfDCountdown != 0) {
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);
	}

	if (!commandQue.empty() && (commandQue.front().GetID() == CMD_WAIT)) {
		DrawWaitIcon(commandQue.front());
	}

	for (auto ci = newUnitCommands.begin(); ci != newUnitCommands.end(); ++ci) {
		const int cmdID = ci->GetID();

		switch (cmdID) {
			case CMD_MOVE: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0) + UpVector * 3.0f, cmdColors.move);
				break;
			}
			case CMD_FIGHT: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0) + UpVector * 3.0f, cmdColors.fight);
				break;
			}
			case CMD_PATROL: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0) + UpVector * 3.0f, cmdColors.patrol);
				break;
			}
			case CMD_ATTACK: {
				if (ci->params.size() == 1) {
					const CUnit* unit = GetTrackableUnit(owner, unitHandler->GetUnit(ci->params[0]));

					if (unit != nullptr) {
						lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.attack);
					}
				} else {
					assert(ci->params.size() >= 3);

					const float x = ci->params[0];
					const float z = ci->params[2];
					const float y = CGround::GetHeightReal(x, z, false) + 3.0f;

					lineDrawer.DrawLineAndIcon(cmdID, float3(x, y, z), cmdColors.attack);
				}

				break;
			}
			case CMD_GUARD: {
				const CUnit* unit = GetTrackableUnit(owner, unitHandler->GetUnit(ci->params[0]));

				if (unit != nullptr) {
					lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.guard);
				}
				break;
			}
			case CMD_WAIT: {
				DrawWaitIcon(*ci);
				break;
			}
			case CMD_SELFD: {
				lineDrawer.DrawIconAtLastPos(cmdID);
				break;
			}
			default: {
				DrawDefaultCommand(*ci, owner);
				break;
			}
		}

		if ((cmdID < 0) && (ci->params.size() >= 3)) {
			BuildInfo bi;
			bi.def = unitDefHandler->GetUnitDefByID(-(cmdID));

			if (ci->params.size() == 4)
				bi.buildFacing = int(ci->params[3]);

			bi.pos = ci->GetPos(0);
			bi.pos = CGameHelper::Pos2BuildPos(bi, false);

			cursorIcons.AddBuildIcon(cmdID, bi.pos, owner->team, bi.buildFacing);
			lineDrawer.DrawLine(bi.pos, cmdColors.build);

			// draw metal extraction range
			if (bi.def->extractRange > 0) {
				lineDrawer.Break(bi.pos, cmdColors.build);
				glColor4fv(cmdColors.rangeExtract);
				glSurfaceCircle(bi.pos, bi.def->extractRange, 40.0f);
				lineDrawer.Restart();
			}
		}
	}

	lineDrawer.FinishPath();
}



void CommandDrawer::DrawMobileCAICommands(const CMobileCAI* cai) const
{
	const CUnit* owner = cai->owner;
	const CCommandQueue& commandQue = cai->commandQue;

	lineDrawer.StartPath(owner->GetObjDrawMidPos(), cmdColors.start);

	if (owner->selfDCountdown != 0) {
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);
	}

	for (auto ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		const int cmdID = ci->GetID();

		switch (cmdID) {
			case CMD_MOVE: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.move);
				break;
			}
			case CMD_PATROL: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.patrol);
				break;
			}
			case CMD_FIGHT: {
				if (ci->params.size() >= 3) {
					lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.fight);
				}
				break;
			}
			case CMD_ATTACK:
			case CMD_MANUALFIRE: {
				if (ci->params.size() == 1) {
					const CUnit* unit = GetTrackableUnit(owner, unitHandler->GetUnit(ci->params[0]));

					if (unit != nullptr) {
						lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.attack);
					}
				}

				if (ci->params.size() >= 3) {
					const float x = ci->params[0];
					const float z = ci->params[2];
					const float y = CGround::GetHeightReal(x, z, false) + 3.0f;

					lineDrawer.DrawLineAndIcon(cmdID, float3(x, y, z), cmdColors.attack);
				}

				break;
			}
			case CMD_GUARD: {
				const CUnit* unit = GetTrackableUnit(owner, unitHandler->GetUnit(ci->params[0]));

				if (unit != nullptr) {
					lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.guard);
				}
				break;
			}
			case CMD_LOAD_ONTO: {
				const CUnit* unit = unitHandler->GetUnitUnsafe(ci->params[0]);
				lineDrawer.DrawLineAndIcon(cmdID, unit->pos, cmdColors.load);
				break;
			}
			case CMD_LOAD_UNITS: {
				if (ci->params.size() == 4) {
					const float3& endPos = ci->GetPos(0);

					lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.load);
					lineDrawer.Break(endPos, cmdColors.load);
					glColor4fv(cmdColors.load);
					glSurfaceCircle(endPos, ci->params[3], 20.0f);
					lineDrawer.RestartWithColor(cmdColors.load);
				} else {
					const CUnit* unit = GetTrackableUnit(owner, unitHandler->GetUnit(ci->params[0]));

					if (unit != nullptr) {
						lineDrawer.DrawLineAndIcon(cmdID, unit->GetObjDrawErrorPos(owner->allyteam), cmdColors.load);
					}
				}
				break;
			}
			case CMD_UNLOAD_UNITS: {
				if (ci->params.size() == 5) {
					const float3& endPos = ci->GetPos(0);

					lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.unload);
					lineDrawer.Break(endPos, cmdColors.unload);
					glColor4fv(cmdColors.unload);
					glSurfaceCircle(endPos, ci->params[3], 20.0f);
					lineDrawer.RestartWithColor(cmdColors.unload);
				}
				break;
			}
			case CMD_UNLOAD_UNIT: {
				lineDrawer.DrawLineAndIcon(cmdID, ci->GetPos(0), cmdColors.unload);
				break;
			}
			case CMD_WAIT: {
				DrawWaitIcon(*ci);
				break;
			}
			case CMD_SELFD: {
				lineDrawer.DrawIconAtLastPos(cmdID);
				break;
			}
			default: {
				DrawDefaultCommand(*ci, owner);
				break;
			}
		}
	}

	lineDrawer.FinishPath();
}


void CommandDrawer::DrawWaitIcon(const Command& cmd) const
{
	waitCommandsAI.AddIcon(cmd, lineDrawer.GetLastPos());
}

void CommandDrawer::DrawDefaultCommand(const Command& c, const CUnit* owner) const
{
	// TODO add Lua callin perhaps, for more elaborate needs?
	const CCommandColors::DrawData* dd = cmdColors.GetCustomCmdData(c.GetID());

	if (dd == nullptr)
		return;

	const unsigned int paramsCount = c.params.size();

	if (paramsCount >= 3) {
		const float3 endPos = c.GetPos(0) + UpVector * 3.0f;

		if (!dd->showArea || (paramsCount < 4)) {
			lineDrawer.DrawLineAndIcon(dd->cmdIconID, endPos, dd->color);
		} else {
			lineDrawer.DrawLineAndIcon(dd->cmdIconID, endPos, dd->color);
			lineDrawer.Break(endPos, dd->color);
			glSurfaceCircle(endPos, c.params[3], 20.0f);
			lineDrawer.RestartWithColor(dd->color);
		}

		return;
	}

	if (paramsCount >= 1) {
		// allow a second param (ignored here) for custom commands
		const CUnit* unit = GetTrackableUnit(owner, unitHandler->GetUnit(c.params[0]));

		if (unit != nullptr) {
			lineDrawer.DrawLineAndIcon(dd->cmdIconID, unit->GetObjDrawErrorPos(owner->allyteam), dd->color);
		}
	}
}

void CommandDrawer::DrawQuedBuildingSquares(const CBuilderCAI* cai) const
{
	const CCommandQueue& commandQue = cai->commandQue;
	const auto& buildOptions = cai->buildOptions;

	unsigned int  buildCommands = 0;
	unsigned int uwaterCommands = 0;

	for (const Command& c: commandQue) {
		if (buildOptions.find(c.GetID()) == buildOptions.end())
			continue;

		BuildInfo bi(c);
		bi.pos = CGameHelper::Pos2BuildPos(bi, false);

		buildCommands += 1;
		uwaterCommands += (bi.pos.y < 0.0f);
	}

	// worst case - 2 squares per building (when underwater) - 8 vertices * 3 floats
	std::vector<GLfloat>   quadVerts(buildCommands * 12);
	std::vector<GLfloat> uwquadVerts(buildCommands * 12); // underwater
	// 4 vertical lines
	std::vector<GLfloat> lineVerts(uwaterCommands * 24);
	// colors for lines
	std::vector<GLfloat> lineColors(uwaterCommands * 48);

	unsigned int   quadcounter = 0;
	unsigned int uwquadcounter = 0;
	unsigned int   linecounter = 0;

	for (const Command& c: commandQue) {
		if (buildOptions.find(c.GetID()) == buildOptions.end())
			continue;

		BuildInfo bi(c);
		bi.pos = CGameHelper::Pos2BuildPos(bi, false);

		const float xsize = bi.GetXSize() * (SQUARE_SIZE >> 1);
		const float zsize = bi.GetZSize() * (SQUARE_SIZE >> 1);

		const float h = bi.pos.y;
		const float x1 = bi.pos.x - xsize;
		const float z1 = bi.pos.z - zsize;
		const float x2 = bi.pos.x + xsize;
		const float z2 = bi.pos.z + zsize;

		quadVerts[quadcounter++] = x1;
		quadVerts[quadcounter++] = h + 1;
		quadVerts[quadcounter++] = z1;
		quadVerts[quadcounter++] = x1;
		quadVerts[quadcounter++] = h + 1;
		quadVerts[quadcounter++] = z2;
		quadVerts[quadcounter++] = x2;
		quadVerts[quadcounter++] = h + 1;
		quadVerts[quadcounter++] = z2;
		quadVerts[quadcounter++] = x2;
		quadVerts[quadcounter++] = h + 1;
		quadVerts[quadcounter++] = z1;

		if (bi.pos.y >= 0.0f)
			continue;

		const float col[8] = {
			0.0f, 0.0f, 1.0f, 0.5f, // start color
			0.0f, 0.5f, 1.0f, 1.0f, // end color
		};

		uwquadVerts[uwquadcounter++] = x1;
		uwquadVerts[uwquadcounter++] = 0.0f;
		uwquadVerts[uwquadcounter++] = z1;
		uwquadVerts[uwquadcounter++] = x1;
		uwquadVerts[uwquadcounter++] = 0.0f;
		uwquadVerts[uwquadcounter++] = z2;
		uwquadVerts[uwquadcounter++] = x2;
		uwquadVerts[uwquadcounter++] = 0.0f;
		uwquadVerts[uwquadcounter++] = z2;
		uwquadVerts[uwquadcounter++] = x2;
		uwquadVerts[uwquadcounter++] = 0.0f;
		uwquadVerts[uwquadcounter++] = z1;

		for (int i = 0; i < 4; ++i) {
			std::copy(col, col + 8, lineColors.begin() + linecounter * 2 + i * 8);
		}

		lineVerts[linecounter++] = x1;
		lineVerts[linecounter++] = h;
		lineVerts[linecounter++] = z1;
		lineVerts[linecounter++] = x1;
		lineVerts[linecounter++] = 0.0f;
		lineVerts[linecounter++] = z1;

		lineVerts[linecounter++] = x2;
		lineVerts[linecounter++] = h;
		lineVerts[linecounter++] = z1;
		lineVerts[linecounter++] = x2;
		lineVerts[linecounter++] = 0.0f;
		lineVerts[linecounter++] = z1;

		lineVerts[linecounter++] = x2;
		lineVerts[linecounter++] = h;
		lineVerts[linecounter++] = z2;
		lineVerts[linecounter++] = x2;
		lineVerts[linecounter++] = 0.0f;
		lineVerts[linecounter++] = z2;

		lineVerts[linecounter++] = x1;
		lineVerts[linecounter++] = h;
		lineVerts[linecounter++] = z2;
		lineVerts[linecounter++] = x1;
		lineVerts[linecounter++] = 0.0f;
		lineVerts[linecounter++] = z2;
	}

	if (quadcounter > 0) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glVertexPointer(3, GL_FLOAT, 0, &quadVerts[0]);
		glDrawArrays(GL_QUADS, 0, quadcounter / 3);

		if (linecounter > 0) {
			glPushAttrib(GL_CURRENT_BIT);
			glColor4f(0.0f, 0.5f, 1.0f, 1.0f); // same as end color of lines
			glVertexPointer(3, GL_FLOAT, 0, &uwquadVerts[0]);
			glDrawArrays(GL_QUADS, 0, uwquadcounter / 3);
			glPopAttrib();

			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(4, GL_FLOAT, 0, &lineColors[0]);
			glVertexPointer(3, GL_FLOAT, 0, &lineVerts[0]);
			glDrawArrays(GL_LINES, 0, linecounter / 3);
			glDisableClientState(GL_COLOR_ARRAY);
		}

		glDisableClientState(GL_VERTEX_ARRAY);
	}
}

