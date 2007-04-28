#ifndef LUA_GL_H
#define LUA_GL_H
// LuaOpenGL.h: interface for the CLuaOpenGL class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif


#include <string>


struct lua_State;


class LuaOpenGL {
	public:
		enum DrawMode {
			DRAW_NONE               = 0,
			DRAW_WORLD              = 1,
			DRAW_WORLD_SHADOW       = 2,
			DRAW_WORLD_REFLECTION   = 3,
			DRAW_WORLD_REFRACTION   = 4,
			DRAW_SCREEN             = 5,
			DRAW_MINIMAP            = 6,
			DRAW_LAST_MODE          = DRAW_MINIMAP
		};

	public:
		static void Init();
		static void Free();

		static bool PushEntries(lua_State* L);

		static void CalcFontHeight();

		static bool IsDrawingEnabled() { return drawingEnabled; }

		static bool GetSafeMode() { return safeMode; }
		static void SetSafeMode(bool value) { safeMode = value; }

		static void EnableCommon(DrawMode);
		static void ResetCommon(DrawMode);
		static void DisableCommon(DrawMode);

		static void EnableDrawWorld();
		static void ResetDrawWorld();
		static void DisableDrawWorld();

		static void EnableDrawWorldShadow();
		static void ResetDrawWorldShadow();
		static void DisableDrawWorldShadow();

		static void EnableDrawWorldReflection();
		static void ResetDrawWorldReflection();
		static void DisableDrawWorldReflection();

		static void EnableDrawWorldRefraction();
		static void ResetDrawWorldRefraction();
		static void DisableDrawWorldRefraction();

		static void EnableDrawScreen();
		static void ResetDrawScreen();
		static void DisableDrawScreen();

		static void EnableDrawInMiniMap();
		static void ResetDrawInMiniMap();
		static void DisableDrawInMiniMap();

	protected:
		static void ResetGLState();

		static void ClearMatrixStack();

		static void ResetWorldMatrices();
		static void ResetWorldShadowMatrices();
		static void ResetScreenMatrices();
		static void ResetMiniMapMatrices();

		static void SetupScreenMatrices();
		static void RevertScreenMatrices();
		static void SetupScreenLighting();
		static void RevertScreenLighting();

		static void SetupWorldLighting();
		static void RevertWorldLighting();

	private:
		static DrawMode drawMode;
		static DrawMode prevDrawMode; // for minimap (when drawn in Screen mode)
		static bool drawingEnabled;
		static bool safeMode;
		static float fontHeight;
		static float screenWidth;
		static float screenDistance;
		static void (*resetMatrixFunc)(void);
		static unsigned int resetStateList;

	private:
		static void CheckDrawingEnabled(lua_State* L, const char* caller);

	private:
		static int ConfigScreen(lua_State* L);

		static int DrawMiniMap(lua_State* L);
		static int SlaveMiniMap(lua_State* L);
		static int ConfigMiniMap(lua_State* L);

		static int ResetState(lua_State* L);
		static int ResetMatrices(lua_State* L);
		static int Clear(lua_State* L);

		static int Lighting(lua_State* L);
		static int ShadeModel(lua_State* L);
		static int Scissor(lua_State* L);
		static int ColorMask(lua_State* L);
		static int DepthMask(lua_State* L);
		static int DepthTest(lua_State* L);
		static int Culling(lua_State* L);
		static int LogicOp(lua_State* L);
		static int Blending(lua_State* L);
		static int AlphaTest(lua_State* L);
		static int LineStipple(lua_State* L);
		static int PolygonMode(lua_State* L);
		static int PolygonOffset(lua_State* L);

		static int Texture(lua_State* L);
		static int Material(lua_State* L);
		static int Color(lua_State* L);

		static int LineWidth(lua_State* L);
		static int PointSize(lua_State* L);

		static int FreeTexture(lua_State* L);
		static int TextureInfo(lua_State* L);

		static int Shape(lua_State* L);
		static int BeginEnd(lua_State* L);
		static int Vertex(lua_State* L);
		static int Normal(lua_State* L);
		static int TexCoord(lua_State* L);
		static int Rect(lua_State* L);
		static int TexRect(lua_State* L);
		static int UnitShape(lua_State* L);
		static int Unit(lua_State* L);
		static int Text(lua_State* L);
		static int GetTextWidth(lua_State* L);

		static int ClipPlane(lua_State* L);

		static int MatrixMode(lua_State* L);
		static int LoadIdentity(lua_State* L);
		static int LoadMatrix(lua_State* L);
		static int MultMatrix(lua_State* L);
		static int Translate(lua_State* L);
		static int Scale(lua_State* L);
		static int Rotate(lua_State* L);
		static int PushMatrix(lua_State* L);
		static int PopMatrix(lua_State* L);

		static int ListCreate(lua_State* L);
		static int ListRun(lua_State* L);
		static int ListDelete(lua_State* L);
};


#endif /* LUA_UNITDEFS_H */
