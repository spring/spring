/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_GL_H
#define LUA_GL_H

#include <string>
#include <set>


struct lua_State;


class LuaOpenGL {
	public:
		enum DrawMode {
			DRAW_NONE               = 0,
			DRAW_GENESIS            = 1,
			DRAW_WORLD              = 2,
			DRAW_WORLD_SHADOW       = 3,
			DRAW_WORLD_REFLECTION   = 4,
			DRAW_WORLD_REFRACTION   = 5,
			DRAW_SCREEN             = 6,
			DRAW_MINIMAP            = 7,
			DRAW_LAST_MODE          = DRAW_MINIMAP
		};

	public:
		static void Init();
		static void Free();

		static bool PushEntries(lua_State* L);

		static bool IsDrawingEnabled() { return drawingEnabled; }
		static void SetDrawingEnabled(bool value) { drawingEnabled = value; }

		static bool CanUseShaders() { return canUseShaders; }

		static bool GetSafeMode() { return safeMode; }
		static void SetSafeMode(bool value) { safeMode = value; }

		static void EnableCommon(DrawMode);
		static void ResetCommon(DrawMode);
		static void DisableCommon(DrawMode);

		static void EnableDrawGenesis();
		static void ResetDrawGenesis();
		static void DisableDrawGenesis();

		static void EnableDrawWorld();
		static void ResetDrawWorld();
		static void DisableDrawWorld();

		static void EnableDrawWorldPreUnit();
		static void ResetDrawWorldPreUnit();
		static void DisableDrawWorldPreUnit();

		static void EnableDrawWorldShadow();
		static void ResetDrawWorldShadow();
		static void DisableDrawWorldShadow();

		static void EnableDrawWorldReflection();
		static void ResetDrawWorldReflection();
		static void DisableDrawWorldReflection();

		static void EnableDrawWorldRefraction();
		static void ResetDrawWorldRefraction();
		static void DisableDrawWorldRefraction();

		static void EnableDrawScreenEffects();
		static void ResetDrawScreenEffects();
		static void DisableDrawScreenEffects();

		static void EnableDrawScreen();
		static void ResetDrawScreen();
		static void DisableDrawScreen();

		static void EnableDrawInMiniMap();
		static void ResetDrawInMiniMap();
		static void DisableDrawInMiniMap();

	protected:
		static void ResetGLState();

		static void ClearMatrixStack(int);

		static void ResetGenesisMatrices();
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
		static bool canUseShaders;
		static float screenWidth;
		static float screenDistance;
		static void (*resetMatrixFunc)(void);
		static unsigned int resetStateList;
		static std::set<unsigned int> occlusionQueries;

	private:
		static void CheckDrawingEnabled(lua_State* L, const char* caller);

	private:
		static int HasExtension(lua_State* L);
		static int GetNumber(lua_State* L);
		static int GetString(lua_State* L);
	
		static int ConfigScreen(lua_State* L);

		static int GetViewSizes(lua_State* L);

		static int DrawMiniMap(lua_State* L);
		static int SlaveMiniMap(lua_State* L);
		static int ConfigMiniMap(lua_State* L);

		static int ResetState(lua_State* L);
		static int ResetMatrices(lua_State* L);
		static int Clear(lua_State* L);

		static int Lighting(lua_State* L);
		static int ShadeModel(lua_State* L);
		static int Scissor(lua_State* L);
		static int Viewport(lua_State* L);
		static int ColorMask(lua_State* L);
		static int DepthMask(lua_State* L);
		static int DepthTest(lua_State* L);
		static int DepthClamp(lua_State* L);
		static int Culling(lua_State* L);
		static int LogicOp(lua_State* L);
		static int Fog(lua_State* L);
		static int Smoothing(lua_State* L);
		static int AlphaTest(lua_State* L);
		static int LineStipple(lua_State* L);
		static int Blending(lua_State* L);
		static int BlendEquation(lua_State* L);
		static int BlendFunc(lua_State* L);
		static int BlendEquationSeparate(lua_State* L);
		static int BlendFuncSeparate(lua_State* L);

		static int Material(lua_State* L);
		static int Color(lua_State* L);

		static int PolygonMode(lua_State* L);
		static int PolygonOffset(lua_State* L);

		static int StencilTest(lua_State* L);
		static int StencilMask(lua_State* L);
		static int StencilFunc(lua_State* L);
		static int StencilOp(lua_State* L);
		static int StencilMaskSeparate(lua_State* L);
		static int StencilFuncSeparate(lua_State* L);
		static int StencilOpSeparate(lua_State* L);

		static int LineWidth(lua_State* L);
		static int PointSize(lua_State* L);
		static int PointSprite(lua_State* L);
		static int PointParameter(lua_State* L);

		static int Texture(lua_State* L);
		static int CreateTexture(lua_State* L);
		static int DeleteTexture(lua_State* L);
		static int DeleteTextureFBO(lua_State* L);
		static int TextureInfo(lua_State* L);
		static int CopyToTexture(lua_State* L);
		static int RenderToTexture(lua_State* L);
		static int GenerateMipmap(lua_State* L);
		static int ActiveTexture(lua_State* L);
		static int TexEnv(lua_State* L);
		static int TexGen(lua_State* L);
		static int MultiTexEnv(lua_State* L);
		static int MultiTexGen(lua_State* L);

		static int Shape(lua_State* L);
		static int BeginEnd(lua_State* L);
		static int Vertex(lua_State* L);
		static int Normal(lua_State* L);
		static int TexCoord(lua_State* L);
		static int MultiTexCoord(lua_State* L);
		static int SecondaryColor(lua_State* L);
		static int FogCoord(lua_State* L);
		static int EdgeFlag(lua_State* L);
		
		static int Rect(lua_State* L);
		static int TexRect(lua_State* L);

		static int BeginText(lua_State* L);
		static int Text(lua_State* L);
		static int EndText(lua_State* L);
		static int GetTextWidth(lua_State* L);
		static int GetTextHeight(lua_State* L);

		static int Unit(lua_State* L);
		static int UnitRaw(lua_State* L);
		static int UnitShape(lua_State* L);
		static int UnitMultMatrix(lua_State* L);
		static int UnitPiece(lua_State* L);
		static int UnitPieceMatrix(lua_State* L);
		static int UnitPieceMultMatrix(lua_State* L);
		static int Feature(lua_State* L);
		static int FeatureShape(lua_State* L);
		static int DrawListAtUnit(lua_State* L);
		static int DrawFuncAtUnit(lua_State* L);
		static int DrawGroundCircle(lua_State* L);
		static int DrawGroundQuad(lua_State* L);

		static int Light(lua_State* L);
		static int ClipPlane(lua_State* L);

		static int MatrixMode(lua_State* L);
		static int LoadIdentity(lua_State* L);
		static int LoadMatrix(lua_State* L);
		static int MultMatrix(lua_State* L);
		static int Translate(lua_State* L);
		static int Scale(lua_State* L);
		static int Rotate(lua_State* L);
		static int Ortho(lua_State* L);
		static int Frustum(lua_State* L);
		static int Billboard(lua_State* L);
		static int PushMatrix(lua_State* L);
		static int PopMatrix(lua_State* L);
		static int PushPopMatrix(lua_State* L);
		static int GetMatrixData(lua_State* L);

		static int PushAttrib(lua_State* L);
		static int PopAttrib(lua_State* L);
		static int UnsafeState(lua_State* L);

		static int CreateList(lua_State* L);
		static int CallList(lua_State* L);
		static int DeleteList(lua_State* L);

		static int Flush(lua_State* L);
		static int Finish(lua_State* L);

		static int ReadPixels(lua_State* L);
		static int SaveImage(lua_State* L);

		static int CreateQuery(lua_State* L);
		static int DeleteQuery(lua_State* L);
		static int RunQuery(lua_State* L);
		static int GetQuery(lua_State* L);
		
		static int GetGlobalTexNames(lua_State* L);
		static int GetGlobalTexCoords(lua_State* L);
		static int GetShadowMapParams(lua_State* L);

		static int GetSun(lua_State* L);

		static int RenderMode(lua_State* L);
		static int SelectBuffer(lua_State* L);
		static int SelectBufferData(lua_State* L);
		static int InitNames(lua_State* L);
		static int LoadName(lua_State* L);
		static int PushName(lua_State* L);
		static int PopName(lua_State* L);
};


#endif /* LUA_UNITDEFS_H */
