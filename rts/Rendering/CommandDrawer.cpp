/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CommandDrawer.h"
#include "LineDrawer.h"
#include "Game/GameHelper.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/CursorIcons.h"
#include "Game/WaitCommandsAI.h"
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
#include "Sim/Units/CommandAI/TransportCAI.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "System/myMath.h"
#include "System/Log/ILog.h"

static bool IsUnitTrackable(const CUnit* unit, const CUnit* owner)
{
	if ((unit->losStatus[owner->allyteam] & (LOS_INLOS | LOS_INRADAR)) != 0) return true;
	if (unit->unitDef->speed <= 0.0f) return true;

	return false;
}

CommandDrawer* CommandDrawer::GetInstance() {
	static CommandDrawer drawer;
	return &drawer;
}



void CommandDrawer::Draw(const CCommandAI* cai) const {
	// note: {Air, Builder, Transport}CAI all inherit from MobileCAI, so test that last
	const       CAirCAI* aCAI; if ((aCAI = dynamic_cast<const       CAirCAI*>(cai)) != NULL) {        DrawAirCAICommands(aCAI); return; }
	const   CBuilderCAI* bCAI; if ((bCAI = dynamic_cast<const   CBuilderCAI*>(cai)) != NULL) {    DrawBuilderCAICommands(bCAI); return; }
	const   CFactoryCAI* fCAI; if ((fCAI = dynamic_cast<const   CFactoryCAI*>(cai)) != NULL) {    DrawFactoryCAICommands(fCAI); return; }
	const CTransportCAI* tCAI; if ((tCAI = dynamic_cast<const CTransportCAI*>(cai)) != NULL) {  DrawTransportCAICommands(tCAI); return; }
	const    CMobileCAI* mCAI; if ((mCAI = dynamic_cast<const    CMobileCAI*>(cai)) != NULL) {     DrawMobileCAICommands(mCAI); return; }

	DrawCommands(cai);
}



void CommandDrawer::DrawCommands(const CCommandAI* cai) const
{
	const CUnit* owner = cai->owner;
	const CCommandQueue& commandQue = cai->commandQue;

	lineDrawer.StartPath(owner->drawMidPos, cmdColors.start);

	if (owner->selfDCountdown != 0) {
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);
	}

	CCommandQueue::const_iterator ci;
	for (ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		const int cmdID = ci->GetID();

		switch (cmdID) {
			case CMD_ATTACK:
			case CMD_MANUALFIRE: {
				if (ci->params.size() == 1) {
					const CUnit* unit = unitHandler->GetUnit(ci->params[0]);

					if ((unit != NULL) && IsUnitTrackable(unit, owner)) {
						const float3& endPos = CGameHelper::GetUnitErrorPos(unit, owner->allyteam);
						lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.attack);
					}
				} else {
					const float x = ci->params[0];
					const float z = ci->params[2];
					const float y = ground->GetHeightReal(x, z, false) + 3.0f;

					lineDrawer.DrawLineAndIcon(cmdID, float3(x, y, z), cmdColors.attack);
				}
				break;
			}
			case CMD_WAIT:{
				DrawWaitIcon(*ci);
				break;
			}
			case CMD_SELFD:{
				lineDrawer.DrawIconAtLastPos(cmdID);
				break;
			}
			default:{
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

	lineDrawer.StartPath(owner->drawMidPos, cmdColors.start);

	if (owner->selfDCountdown != 0) {
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);
	}

	CCommandQueue::const_iterator ci;
	for (ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		const int cmdID = ci->GetID();

		switch (cmdID) {
			case CMD_MOVE: {
				const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.move);
				break;
			}
			case CMD_FIGHT: {
				const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.fight);
				break;
			}
			case CMD_PATROL: {
				const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.patrol);
				break;
			}
			case CMD_ATTACK: {
				if (ci->params.size() == 1) {
					const CUnit* unit = unitHandler->GetUnit(ci->params[0]);

					if ((unit != NULL) && IsUnitTrackable(unit, owner)) {
						const float3& endPos = CGameHelper::GetUnitErrorPos(unit, owner->allyteam);
						lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.attack);
					}
				} else {
					const float x = ci->params[0];
					const float z = ci->params[2];
					const float y = ground->GetHeightReal(x, z, false) + 3.0f;

					lineDrawer.DrawLineAndIcon(cmdID, float3(x, y, z), cmdColors.attack);
				}
				break;
			}
			case CMD_AREA_ATTACK: {
				const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);

				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.attack);
				lineDrawer.Break(endPos, cmdColors.attack);
				glColor4fv(cmdColors.attack);
				glSurfaceCircle(endPos, ci->params[3], 20);
				lineDrawer.RestartWithColor(cmdColors.attack);
				break;
			}
			case CMD_GUARD: {
				const CUnit* unit = unitHandler->GetUnit(ci->params[0]);

				if ((unit != NULL) && IsUnitTrackable(unit, owner)) {
					const float3& endPos = CGameHelper::GetUnitErrorPos(unit, owner->allyteam);
					lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.guard);
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
			default:
				DrawDefaultCommand(*ci, owner);
				break;
		}
	}
	lineDrawer.FinishPath();
}



void CommandDrawer::DrawBuilderCAICommands(const CBuilderCAI* cai) const
{
	const CUnit* owner = cai->owner;
	const CCommandQueue& commandQue = cai->commandQue;

	lineDrawer.StartPath(owner->drawMidPos, cmdColors.start);

	if (owner->selfDCountdown != 0) {
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);
	}

	CCommandQueue::const_iterator ci;
	for (ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		const int cmdID = ci->GetID();

		if (cmdID < 0) {
			const std::map<int, string>& buildOptions = cai->buildOptions;
			const std::map<int, string>::const_iterator boi = buildOptions.find(cmdID);

			if (boi != buildOptions.end()) {
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
					glSurfaceCircle(bi.pos, bi.def->extractRange, 40);
					lineDrawer.Restart();
				}
			}
			continue;
		}

		switch (cmdID) {
			case CMD_MOVE: {
				const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.move);
				break;
			}
			case CMD_FIGHT:{
				const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.fight);
				break;
			}
			case CMD_PATROL: {
				const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.patrol);
				break;
			}
			case CMD_GUARD: {
				const CUnit* unit = unitHandler->GetUnit(ci->params[0]);

				if ((unit != NULL) && IsUnitTrackable(unit, owner)) {
					const float3& endPos = CGameHelper::GetUnitErrorPos(unit, owner->allyteam);
					lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.guard);
				}
				break;
			}
			case CMD_RESTORE: {
				const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.restore);
				lineDrawer.Break(endPos, cmdColors.restore);
				glColor4fv(cmdColors.restore);
				glSurfaceCircle(endPos, ci->params[3], 20);
				lineDrawer.RestartWithColor(cmdColors.restore);
				break;
			}
			case CMD_ATTACK:
			case CMD_MANUALFIRE: {
				if (ci->params.size() == 1) {
					const CUnit* unit = unitHandler->GetUnit(ci->params[0]);

					if ((unit != NULL) && IsUnitTrackable(unit, owner)) {
						const float3& endPos = CGameHelper::GetUnitErrorPos(unit, owner->allyteam);
						lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.attack);
					}
				} else {
					const float x = ci->params[0];
					const float z = ci->params[2];
					const float y = ground->GetHeightReal(x, z, false) + 3.0f;

					lineDrawer.DrawLineAndIcon(cmdID, float3(x, y, z), cmdColors.attack);
				}
				break;
			}
			case CMD_RECLAIM:
			case CMD_RESURRECT: {
				const float* color = (cmdID == CMD_RECLAIM) ? cmdColors.reclaim
				                                             : cmdColors.resurrect;
				if (ci->params.size() == 4) {
					const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
					lineDrawer.DrawLineAndIcon(cmdID, endPos, color);
					lineDrawer.Break(endPos, color);
					glColor4fv(color);
					glSurfaceCircle(endPos, ci->params[3], 20);
					lineDrawer.RestartWithColor(color);
				} else {
					const int signedId = (int)ci->params[0];
					if (signedId < 0) {
						LOG_L(L_WARNING, "Trying to %s a feature or unit with id < 0 (%i), aborting.",
								(cmdID == CMD_RECLAIM) ? "reclaim" : "resurrect",
								signedId);
						break;
					}

					const unsigned int id = signedId;

					if (id >= unitHandler->MaxUnits()) {
						GML_RECMUTEX_LOCK(feat); // DrawCommands

						CFeature* feature = featureHandler->GetFeature(id - unitHandler->MaxUnits());
						if (feature) {
							const float3 endPos = feature->midPos;
							lineDrawer.DrawLineAndIcon(cmdID, endPos, color);
						}
					} else {
						const CUnit* unit = unitHandler->GetUnitUnsafe(id);

						if ((unit != NULL) && (unit != owner) && IsUnitTrackable(unit, owner)) {
							const float3& endPos = CGameHelper::GetUnitErrorPos(unit, owner->allyteam);
							lineDrawer.DrawLineAndIcon(cmdID, endPos, color);
						}
					}
				}
				break;
			}
			case CMD_REPAIR:
			case CMD_CAPTURE: {
				const float* color = (ci->GetID() == CMD_REPAIR) ? cmdColors.repair
				                                            : cmdColors.capture;
				if (ci->params.size() == 4) {
					const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
					lineDrawer.DrawLineAndIcon(cmdID, endPos, color);
					lineDrawer.Break(endPos, color);
					glColor4fv(color);
					glSurfaceCircle(endPos, ci->params[3], 20);
					lineDrawer.RestartWithColor(color);
				} else {
					if (ci->params.size() >= 1) {
						const CUnit* unit = unitHandler->GetUnit(ci->params[0]);

						if ((unit != NULL) && IsUnitTrackable(unit, owner)) {
							const float3& endPos = CGameHelper::GetUnitErrorPos(unit, owner->allyteam);
							lineDrawer.DrawLineAndIcon(cmdID, endPos, color);
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

	lineDrawer.StartPath(owner->drawMidPos, cmdColors.start);

	if (owner->selfDCountdown != 0) {
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);
	}

	if (!commandQue.empty() && (commandQue.front().GetID() == CMD_WAIT)) {
		DrawWaitIcon(commandQue.front());
	}

	CCommandQueue::const_iterator ci;
	for (ci = newUnitCommands.begin(); ci != newUnitCommands.end(); ++ci) {
		const int cmdID = ci->GetID();

		switch (cmdID) {
			case CMD_MOVE: {
				const float3 endPos(ci->params[0], ci->params[1] + 3, ci->params[2]);
				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.move);
				break;
			}
			case CMD_FIGHT: {
				const float3 endPos(ci->params[0], ci->params[1] + 3, ci->params[2]);
				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.fight);
				break;
			}
			case CMD_PATROL: {
				const float3 endPos(ci->params[0], ci->params[1] + 3, ci->params[2]);
				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.patrol);
				break;
			}
			case CMD_ATTACK: {
				if (ci->params.size() == 1) {
					const CUnit* unit = unitHandler->GetUnit(ci->params[0]);

					if ((unit != NULL) && IsUnitTrackable(unit, owner)) {
						const float3& endPos = CGameHelper::GetUnitErrorPos(unit, owner->allyteam);
						lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.attack);
					}
				} else {
					const float x = ci->params[0];
					const float z = ci->params[2];
					const float y = ground->GetHeightReal(x, z, false) + 3.0f;

					lineDrawer.DrawLineAndIcon(cmdID, float3(x, y, z), cmdColors.attack);
				}
				break;
			}
			case CMD_GUARD: {
				const CUnit* unit = unitHandler->GetUnit(ci->params[0]);

				if ((unit != NULL) && IsUnitTrackable(unit, owner)) {
					const float3& endPos = CGameHelper::GetUnitErrorPos(unit, owner->allyteam);
					lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.guard);
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
			default:
				DrawDefaultCommand(*ci, owner);
				break;
		}

		if ((cmdID < 0) && (ci->params.size() >= 3)) {
			BuildInfo bi;
			bi.def = unitDefHandler->GetUnitDefByID(-(cmdID));
			if (ci->params.size() == 4) {
				bi.buildFacing = int(ci->params[3]);
			}
			bi.pos = float3(ci->params[0], ci->params[1], ci->params[2]);
			bi.pos = CGameHelper::Pos2BuildPos(bi, false);

			cursorIcons.AddBuildIcon(cmdID, bi.pos, owner->team, bi.buildFacing);
			lineDrawer.DrawLine(bi.pos, cmdColors.build);

			// draw metal extraction range
			if (bi.def->extractRange > 0) {
				lineDrawer.Break(bi.pos, cmdColors.build);
				glColor4fv(cmdColors.rangeExtract);
				glSurfaceCircle(bi.pos, bi.def->extractRange, 40);
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

	lineDrawer.StartPath(owner->drawMidPos, cmdColors.start);

	if (owner->selfDCountdown != 0) {
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);
	}

	CCommandQueue::const_iterator ci;
	for (ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		const int cmdID = ci->GetID();

		switch (cmdID) {
			case CMD_MOVE: {
				const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.move);
				break;
			}
			case CMD_PATROL: {
				const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.patrol);
				break;
			}
			case CMD_FIGHT: {
				if (ci->params.size() >= 3) {
					const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
					lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.fight);
				}
				break;
			}
			case CMD_ATTACK:
			case CMD_MANUALFIRE: {
				if (ci->params.size() == 1) {
					const CUnit* unit = unitHandler->GetUnit(ci->params[0]);

					if ((unit != NULL) && IsUnitTrackable(unit, owner)) {
						const float3& endPos = CGameHelper::GetUnitErrorPos(unit, owner->allyteam);
						lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.attack);
					}
				}
				else if (ci->params.size() >= 3) {
					const float x = ci->params[0];
					const float z = ci->params[2];
					const float y = ground->GetHeightReal(x, z, false) + 3.0f;

					lineDrawer.DrawLineAndIcon(cmdID, float3(x, y, z), cmdColors.attack);
				}
				break;
			}
			case CMD_GUARD: {
				const CUnit* unit = unitHandler->GetUnit(ci->params[0]);

				if ((unit != NULL) && IsUnitTrackable(unit, owner)) {
					const float3& endPos = CGameHelper::GetUnitErrorPos(unit, owner->allyteam);
					lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.guard);
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



void CommandDrawer::DrawTransportCAICommands(const CTransportCAI* cai) const
{
	const CUnit* owner = cai->owner;
	const CCommandQueue& commandQue = cai->commandQue;

	lineDrawer.StartPath(owner->drawMidPos, cmdColors.start);

	if (owner->selfDCountdown != 0) {
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);
	}

	CCommandQueue::const_iterator ci;
	for (ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		const int cmdID = ci->GetID();

		switch (cmdID) {
			case CMD_MOVE: {
				const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.move);
				break;
			}
			case CMD_FIGHT: {
				if (ci->params.size() >= 3) {
					const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
					lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.fight);
				}
				break;
			}
			case CMD_PATROL: {
				const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.patrol);
				break;
			}
			case CMD_ATTACK: {
				if (ci->params.size() == 1) {
					const CUnit* unit = unitHandler->GetUnit(ci->params[0]);

					if ((unit != NULL) && IsUnitTrackable(unit, owner)) {
						const float3& endPos = CGameHelper::GetUnitErrorPos(unit, owner->allyteam);
						lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.attack);
					}
				} else {
					const float x = ci->params[0];
					const float z = ci->params[2];
					const float y = ground->GetHeightReal(x, z, false) + 3.0f;

					lineDrawer.DrawLineAndIcon(cmdID, float3(x, y, z), cmdColors.attack);
				}
				break;
			}
			case CMD_GUARD: {
				const CUnit* unit = unitHandler->GetUnit(ci->params[0]);
				if ((unit != NULL) && IsUnitTrackable(unit, owner)) {
					const float3& endPos = CGameHelper::GetUnitErrorPos(unit, owner->allyteam);
					lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.guard);
				}
				break;
			}
			case CMD_LOAD_UNITS: {
				if (ci->params.size() == 4) {
					const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);

					lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.load);
					lineDrawer.Break(endPos, cmdColors.load);
					glColor4fv(cmdColors.load);
					glSurfaceCircle(endPos, ci->params[3], 20);
					lineDrawer.RestartWithColor(cmdColors.load);
				} else {
					const CUnit* unit = unitHandler->GetUnit(ci->params[0]);
					if ((unit != NULL) && IsUnitTrackable(unit, owner)) {
						const float3& endPos = CGameHelper::GetUnitErrorPos(unit, owner->allyteam);
						lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.load);
					}
				}
				break;
			}
			case CMD_UNLOAD_UNITS: {
				if (ci->params.size() == 5) {
					const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);

					lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.unload);
					lineDrawer.Break(endPos, cmdColors.unload);
					glColor4fv(cmdColors.unload);
					glSurfaceCircle(endPos, ci->params[3], 20);
					lineDrawer.RestartWithColor(cmdColors.unload);
				}
				break;
			}
			case CMD_UNLOAD_UNIT: {
				const float3 endPos(ci->params[0], ci->params[1], ci->params[2]);
				lineDrawer.DrawLineAndIcon(cmdID, endPos, cmdColors.unload);
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
			default:
				DrawDefaultCommand(*ci, owner);
				break;
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
	if (dd == NULL) {
		return;
	}
	const int paramsCount = c.params.size();

	if (paramsCount == 1) {
		const CUnit* unit = unitHandler->GetUnit(c.params[0]);

		if ((unit != NULL) && IsUnitTrackable(unit, owner)) {
			const float3& endPos = CGameHelper::GetUnitErrorPos(unit, owner->allyteam);
			lineDrawer.DrawLineAndIcon(dd->cmdIconID, endPos, dd->color);
		}

		return;
	}

	if (paramsCount < 3) {
		return;
	}

	const float3& endPos = c.GetPos(0) + float3(0.0f, 3.0f, 0.0f);

	if (!dd->showArea || (paramsCount < 4)) {
		lineDrawer.DrawLineAndIcon(dd->cmdIconID, endPos, dd->color);
	}
	else {
		const float radius = c.params[3];
		lineDrawer.DrawLineAndIcon(dd->cmdIconID, endPos, dd->color);
		lineDrawer.Break(endPos, dd->color);
		glSurfaceCircle(endPos, radius, 20);
		lineDrawer.RestartWithColor(dd->color);
	}
}

void CommandDrawer::DrawQuedBuildingSquares(const CBuilderCAI* cai) const
{
	const CCommandQueue& commandQue = cai->commandQue;
	const std::map<int, string>& buildOptions = cai->buildOptions;

	CCommandQueue::const_iterator ci;

	int buildCommands = 0;
	int underwaterCommands = 0;
	for (ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		if (buildOptions.find(ci->GetID()) != buildOptions.end()) {
			++buildCommands;
			BuildInfo bi(*ci);
			bi.pos = CGameHelper::Pos2BuildPos(bi, false);
			if (bi.pos.y < 0.f)
				++underwaterCommands;
		}
	}

	// worst case - 2 squares per building (when underwater) - 8 vertices * 3 floats
	std::vector<GLfloat> vertices_quads(buildCommands * 12);
	std::vector<GLfloat> vertices_quads_uw(buildCommands * 12); // underwater
	// 4 vertical lines
	std::vector<GLfloat> vertices_lines(underwaterCommands * 24);
	// colors for lines
	std::vector<GLfloat> colors_lines(underwaterCommands * 48);

	int quadcounter = 0;
	int uwqcounter = 0;
	int linecounter = 0;

	for (ci = commandQue.begin(); ci != commandQue.end(); ++ci) {
		if (buildOptions.find(ci->GetID()) != buildOptions.end()) {
			BuildInfo bi(*ci);
			bi.pos = CGameHelper::Pos2BuildPos(bi, false);
			const float xsize = bi.GetXSize()*4;
			const float zsize = bi.GetZSize()*4;

			const float h = bi.pos.y;
			const float x1 = bi.pos.x - xsize;
			const float z1 = bi.pos.z - zsize;
			const float x2 = bi.pos.x + xsize;
			const float z2 = bi.pos.z + zsize;

			vertices_quads[quadcounter + 0] = x1;
			vertices_quads[quadcounter + 1] = h + 1;
			vertices_quads[quadcounter + 2] = z1;
			vertices_quads[quadcounter + 3] = x1;
			vertices_quads[quadcounter + 4] = h + 1;
			vertices_quads[quadcounter + 5] = z2;
			vertices_quads[quadcounter + 6] = x2;
			vertices_quads[quadcounter + 7] = h + 1;
			vertices_quads[quadcounter + 8] = z2;
			vertices_quads[quadcounter + 9] = x2;
			vertices_quads[quadcounter +10] = h + 1;
			vertices_quads[quadcounter +11] = z1;

			quadcounter += 12;

			if (bi.pos.y < 0.0f) {
				const float col[8] = { 0.0f, 0.0f, 1.0f, 0.5f, // start color
						       0.0f, 0.5f, 1.0f, 1.0f }; // end color

				vertices_quads_uw[uwqcounter + 0] = x1;
				vertices_quads_uw[uwqcounter + 1] = 0.f;
				vertices_quads_uw[uwqcounter + 2] = z1;
				vertices_quads_uw[uwqcounter + 3] = x1;
				vertices_quads_uw[uwqcounter + 4] = 0.f;
				vertices_quads_uw[uwqcounter + 5] = z2;
				vertices_quads_uw[uwqcounter + 6] = x2;
				vertices_quads_uw[uwqcounter + 7] = 0.f;
				vertices_quads_uw[uwqcounter + 8] = z2;
				vertices_quads_uw[uwqcounter + 9] = x2;
				vertices_quads_uw[uwqcounter +10] = 0.f;
				vertices_quads_uw[uwqcounter +11] = z1;

				uwqcounter += 12;

				for (int i = 0; i<4; ++i) {
					std::copy(col, col + 8, colors_lines.begin() + linecounter * 2 + i * 8);
				}

				vertices_lines[linecounter + 0] = x1;
				vertices_lines[linecounter + 1] = h;
				vertices_lines[linecounter + 2] = z1;
				vertices_lines[linecounter + 3] = x1;
				vertices_lines[linecounter + 4] = 0;
				vertices_lines[linecounter + 5] = z1;

				vertices_lines[linecounter + 6] = x2;
				vertices_lines[linecounter + 7] = h;
				vertices_lines[linecounter + 8] = z1;
				vertices_lines[linecounter + 9] = x2;
				vertices_lines[linecounter +10] = 0;
				vertices_lines[linecounter +11] = z1;

				vertices_lines[linecounter +12] = x2;
				vertices_lines[linecounter +13] = h;
				vertices_lines[linecounter +14] = z2;
				vertices_lines[linecounter +15] = x2;
				vertices_lines[linecounter +16] = 0;
				vertices_lines[linecounter +17] = z2;

				vertices_lines[linecounter +18] = x1;
				vertices_lines[linecounter +19] = h;
				vertices_lines[linecounter +20] = z2;
				vertices_lines[linecounter +21] = x1;
				vertices_lines[linecounter +22] = 0;
				vertices_lines[linecounter +23] = z2;

				linecounter += 24;
			}
		}
	}

	if (quadcounter) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glVertexPointer(3, GL_FLOAT, 0, &vertices_quads[0]);
		glDrawArrays(GL_QUADS, 0, quadcounter/3);

		if (linecounter) {
			glPushAttrib(GL_CURRENT_BIT);
			glColor4f(0.0f, 0.5f, 1.0f, 1.0f); // same as end color of lines
			glVertexPointer(3, GL_FLOAT, 0, &vertices_quads_uw[0]);
			glDrawArrays(GL_QUADS, 0, uwqcounter/3);
			glPopAttrib();

			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(4, GL_FLOAT, 0, &colors_lines[0]);
			glVertexPointer(3, GL_FLOAT, 0, &vertices_lines[0]);
			glDrawArrays(GL_LINES, 0, linecounter/3);
			glDisableClientState(GL_COLOR_ARRAY);
		}
		glDisableClientState(GL_VERTEX_ARRAY);
	}
}
