/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_SYNCED_MOVE_CTRL_H
#define LUA_SYNCED_MOVE_CTRL_H

class CUnit;
struct lua_State;


class LuaSyncedMoveCtrl {
	public:
		static bool PushMoveCtrl(lua_State* L);

	private:
		static bool CreateMetaTable(lua_State* L);
		static bool CreateMoveCtrl(lua_State* L, CUnit* unit);

	private: // call-outs
		static int IsEnabled(lua_State* L);

		static int Enable(lua_State* L);
		static int Disable(lua_State* L);

		static int SetTag(lua_State* L);
		static int GetTag(lua_State* L);

		static int SetProgressState(lua_State* L);

		static int SetExtrapolate(lua_State* L);
		static int SetRelativeVelocity(lua_State* L);

		static int SetPhysics(lua_State* L);
		static int SetPosition(lua_State* L);
		static int SetVelocity(lua_State* L);
		static int SetRotation(lua_State* L);
		static int SetRotationOffset(lua_State* L);
		static int SetRotationVelocity(lua_State* L);
		static int SetHeading(lua_State* L);

		static int SetTrackSlope(lua_State* L);
		static int SetTrackGround(lua_State* L);
		static int SetGroundOffset(lua_State* L);
		static int SetGravity(lua_State* L);
		static int SetDrag(lua_State* L);

		static int SetWindFactor(lua_State* L);

		static int SetLimits(lua_State* L);

		static int SetNoBlocking(lua_State* L);

		static int SetShotStop(lua_State* L);
		static int SetSlopeStop(lua_State* L);
		static int SetCollideStop(lua_State* L);

		// *MoveType-specific setters
		static int SetGroundMoveTypeData(lua_State* L);
		static int SetAirMoveTypeData(lua_State* L);
		static int SetGunshipMoveTypeData(lua_State* L);
		static int SetBaseMoveTypeData(lua_State* L);

		static int SetMoveDef(lua_State* L);
};


#endif /* LUA_SYNCED_MOVE_CTRL_H */
