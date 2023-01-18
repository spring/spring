// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef LUA_RENDER_H
#define LUA_RENDER_H

#include "LuaDefs.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/ShaderHandler.h"

#include <vector>

/**
 * @brief Basic drawing functions for Lua.
 * @details The interface is designed to be an abstraction of the graphics
 * backend. It does use the current OpenGL context, though. For example the
 * bound texture and blending settings are used and even the shader programs
 * can be overridden by enabling a custom program prior to calling a draw
 * function.
 */
class LuaRender {
public:
	/**
	 * @brief Initialize.
	 * @details Allocate OpenGL resources, compile shader programs.
	 */
	static void Init();

	/**
	 * @brief Shutdown.
	 * @details Free OpenGL resources and shader programs.
	 */
	static void Free();

	/**
	 * @brief Make functions available to Lua.
	 * @param L Lua state.
	 * @return true on success.
	 */
	static bool PushEntries(lua_State* L);

private:
#ifdef HEADLESS
	static int Vertices(lua_State* L) { return 1; };
	static int Lines(lua_State* L) { return 1; };
	static int Triangle(lua_State* L) { return 1; };
	static int Rectangle(lua_State* L) { return 1; };
	static int ReloadShaders(lua_State* L) { return 1; };
#else
	/**
	 * @brief Texture transformations.
	 */
	enum class TextureTransform {
		ROTATE0,
		ROTATE90,
		ROTATE180,
		ROTATE270,
		XFLIP,
		YFLIP,
		DUFLIP,
		DDFLIP,
	};

	/**
	 * @brief Load shader programs.
	 */
	static void LoadPrograms();

	/**
	 * @brief Unload shader programs.
	 */
	static void UnloadPrograms();

	/**
	 * @brief Check if the current context allows drawing.
	 * @param L Lua state.
	 * @param caller Name of the calling function.
	 */
	static void CheckDrawingEnabled(lua_State* L, const char* caller);

	/**
	 * @brief Check OpenGL error state.
	 */
	static void CheckGLError();

	/**
	 * @brief Enable shader program and bind vertex buffer.
	 * @param defaultProgram Shader program to use if there is no active program.
	 */
	static void Bind(Shader::IProgramObject* defaultProgram=nullptr);

	/**
	 * @brief Disable shader program and unbind vertex buffer.
	 */
	static void Unbind();

	/**
	 * @brief Map a portion of the vertex buffer.
	 * @param numBytes Number of bytes to map.
	 * @param buffer Pointer to start of mapped buffer. Is set to nullptr on failure.
	 * @return Offset of the mapped portion in the vertex buffer.
	 */
	static GLuint MapBuffer(GLuint numBytes, GLvoid** buffer);

	/**
	 * @brief Unmap vertex buffer.
	 */
	static void UnmapBuffer();

	/**
	 * @brief Check if spring is currently in the intro stage.
	 * @return true iff spring has not finished loading.
	 */
	static bool IsIntro();

	/**
	 * @brief Get number of spacial dimensions.
	 * @return 2 when drawing to screen, 3 when drawing to world or minimap.
	 */
	static int GetDimensions();

	/**
	 * @brief Parse texture coordinates and texture transformation from Lua state.
	 * @param L Lua state.
	 * @param textureTransform Parsed texture transformation.
	 * @param texcoords Parsed texture coordinates.
	 * @return true iff parsing was successful.
	 */
	static bool ParseTexcoords(lua_State* L, TextureTransform& textureTransform, float texcoords[8]);

	/**
	 * @brief Transform coordinates from/to relative screen coordinates when appropriate.
	 * @param pos Vector of 2D coordinates.
	 * @param relative true iff pos are relative screen coordinates ([0-1],[0-1]).
	 */
	static void TransformCoords(std::vector<float>& pos, bool relative);

	/**
	 * @brief Transform parameters from/to relative screen coordinates when appropriate.
	 * @param bevel Bevel parameter of rectangle.
	 * @param radius Radius parameter of rectangle.
	 * @param border Border parameter of rectangle.
	 * @param relative true iff bevel, radius and border are given in relative screen coordinates.
	 */
	static void TransformParameters(float& bevel, float& radius, float& border, bool relative);

	/**
	 * @brief Submit vertices with custom vertex attributes.
	 * @details Lua call: Vertices(numVertices, {vertexAttribute}, ...)
	 * vertexAttribute = {v1,v2,v3,...}|{{v1},{v2},{v3},...}
	 *
	 * - All vertex attributes are of type 'float'.
	 * - Vertex attributes are flattened during parsing.
	 * - The sizes of the attributes are determined by dividing by numVertices.
	 *
	 * This can be used to submit vertex attributes to a custom Lua shader.
	 * Note that you have to manage your shader completely from Lua.
	 * No uniforms are set by Spring.Draw and no screen/world context is evaluated.
	 *
	 * Example: Spring.Draw.Vertices(3,
	 *                              {{0, 0, 0}, {100, 0, 0}, {100, 0, 100}},
	 *                              {{1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}})
	 *
	 * @param L Lua state.
	 * @return Always 0.
	 */
	static int Vertices(lua_State* L);

	/**
	 * @brief Draw connected line segments.
	 * @details Lua call: Lines(coords, {options})
	 * coords = coords2d | coords3d (* depending on context *)
	 * coords2d = x1,y1,x2,y2,...  |
	 *            {x1,y1},{x2,y2},... |
	 *            {{x1,y1},{x2,y2},...}
	 * coords3d = x1,y1,z1,x2,y2,z2,... |
	 *            {x1,y1,z1},{x2,y2,z2},... |
	 *            {{x1,y1,z1},{x2,y2,z2},...}
	 * options = [relative=bool,]
	 *           [width=float,]
	 *           [color={r,g,b,a}, |
	 *            colors={r1,g1,b1,a1,r2,g2,b2,a2,...}, |
	 *            colors={{r1,g1,b1,a1},{r2,g2,b2,a2},...},]
	 *           [loop=bool,]
	 *           [texcoords={s,t,p,q},]
	 *           [animationspeed=float,]
	 *
	 * - The texture bound to texture unit 0 is used.
	 * - 'relative' specifies if coords is interpreted as relative screen
	 *   coordinates ([0,1],[0,1]). Defaults to true in intro, false in game.
	 * - Texture coordinate p actually defines the texture coordinate at the
	 *   length of one width.
	 * - 'animationspeed' is given in texture coordinate units per second.
	 *
	 * Example: Spring.Draw.Lines({10, 10}, {10, 100}, {100, 100}, {width=5, color={0.5,0,0,1}, loop=true})
	 *
	 * @param L Lua state.
	 * @return Always 0.
	 */
	static int Lines(lua_State* L);

	/**
	 * @brief Draw one triangle.
	 * @details Lua call: Triangle(coords, {options})
	 * coords = coords2d | coords3d (* depending on context *)
	 * coords2d = x1,y1,x2,y2,x3,y3 |
	 *            {x1,y1},{x2,y2},{x3,y3} |
	 *            {{x1,y1},{x2,y2},{x3,y3}}
	 * coords3d = x1,y1,z1,x2,y2,z2,x3,y3,z3 |
	 *            {x1,y1,z1},{x2,y2,z2},{x3,y3,z3} |
	 *            {{x1,y1,z1},{x2,y2,z2},{x3,y3,z3}}
	 * options = [relative=bool,]
	 *           [color={r,g,b,a}, |
	 *            colors={r1,g1,b1,a1,r2,g2,b2,a2,r3,g3,b3,a3}, |
	 *            colors={{r1,g1,b1,a1},{r2,g2,b2,a2},{r3,g3,b3,a3}},]
	 *           [texcoords={s1,t1,s2,t2,s3,t3},]
	 *
	 * - Vertices are given in counter-clockwise order.
	 * - The texture bound to texture unit 0 is used.
	 * - 'relative' specifies if coords is interpreted as relative screen
	 *   coordinates ([0,1],[0,1]). Defaults to true in intro, false in game.
	 *
	 * Example: Spring.Draw.Triangle({10, 10}, {10, 100}, {100, 100}, {color={0.5,0,0,1}})
	 *
	 * @param L Lua state.
	 * @return Always 0.
	 */
	static int Triangle(lua_State* L);

	/**
	 * @brief Draw one rectangle.
	 * @details Lua call: Rectangle(coords, {options})
	 * coords = coords2d | coords3d (* depending on context *)
	 * coords2d = x1,y1,x2,y2 |
	 *            {x1,y1},{x2,y2} |
	 *            {{x1,y1},{x2,y2}}
	 * coords3d = x1,y1,z1,x2,y2,z2,x3,y3,z3,x4,y4,z4 |
	 *           {x1,y1,z1},{x2,y2,z2},{x3,y3,z3},{x4,y4,z4} |
	 *           {{x1,y1,z1},{x2,y2,z2},{x3,y3,z3},{x4,y4,z4}}
	 * options = [relative=bool,]
	 *           [bevel=float, | radius=float,]
	 *           [color={r,g,b,a}, |
	 *            colors={r1,g1,b1,a1,r2,g2,b2,a2,r3,g3,b3,a3,r4,g4,b4,a4}, |
	 *            colors={{r1,g1,b1,a1},{r2,g2,b2,a2},{r3,g3,b3,a3},{r4,g4,b4,a4}},]
	 *           [texcoords={s1,t1,s2,t2,s3,t3,s4,t4}, |
	 *            texcoords=(0|90|180|270|"xflip"|"yflip"|"duflip"|"ddflip"),]
	 *           [border=float,]
	 *           [bordercolor={r,g,b,a},]
	 *           [bordercolors={r1,g1,b1,a1,r2,g2,b2,a2,r3,g3,b3,a3,r4,g4,b4,a4}, |
	 *            bordercolors={{r1,g1,b1,a1},{r2,g2,b2,a2},{r3,g3,b3,a3},{r4,g4,b4,a4}},]
	 *
	 * - The texture bound to texture unit 0 is used.
	 * - For a screen context, the rectangle is aligned to the screen and defined
	 *   by the points (x1,y1), (x2,y2) with x1<x2 and y1<y2.
	 *   'bordercolors' are given in counter-clockwise order, starting with the
	 *   lower left corner.
	 * - For a world context, all four corners need to be specified in
	 *   counter-clockwise order. If they are not in a plane, a kinked
	 *   rectangle will be drawn. Default texture coordinates assume the first
	 *   vertex is the lower left one at default camera rotation.
	 * - 'relative' specifies that coords should be interpreted as relative
	 *   screen coordinates ([0,1],[0,1]). If true, the options 'bevel',
	 *   'radius' and 'border' are also interpreted as relative lengths.
	 *   Defaults to true in intro, false in game.
	 * - Bevels are not drawn when the rectangle touches the screen edges.
	 * - The border width of a bevelled rectangle is rounded down to the
	 *   nearest integer.
	 *
	 * Example: Spring.Draw.Rectangle({10, 10}, {100, 100}, {radius=5, color={0.5,0,0,1}, border=1, bordercolor={0,1,0,1}})
	 *
	 * @param L Lua state.
	 * @return Always 0.
	 */
	static int Rectangle(lua_State* L);

	/**
	 * @brief Reload and recompile shader programs if the files changed.
	 * @details Note that this will not pick up changes of files in VFS.
	 * @param L Lua state.
	 * @return Always 0.
	 */
	static int ReloadShaders(lua_State* L);

	/**
	 * @brief Shader program used to draw lines.
	 */
	static Shader::IProgramObject* sLineProgram;

	/**
	 * @brief Shader program used to draw polygons.
	 */
	static Shader::IProgramObject* sPolygonProgram;

	/**
	 * @brief Shader program used to draw rectangles on screen.
	 */
	static Shader::IProgramObject* sScreenRectProgram;

	/**
	 * @brief OpenGL name of vertex array.
	 */
	static GLuint sVertexArray;

	/**
	 * @brief OpenGL name of vertex buffer.
	 */
	static GLuint sVertexBuffer;

	/**
	 * @brief Size of vertex buffer in bytes.
	 */
	static const GLsizeiptr sVertexBufferSize = 32768;

	/**
	 * @brief Current offset in vertex buffer.
	 */
	static GLuint sVertexBufferOffset;
#endif
};

#endif /* LUA_RENDER_H */
