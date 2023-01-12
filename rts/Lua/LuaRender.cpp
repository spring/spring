// SPDX-License-Identifier: GPL-2.0-or-later

#include "LuaRender.h"

#include "Game/Camera.h"
#include "Game/Game.h"
#include "Game/GlobalUnsynced.h"
#include "Game/UI/MiniMap.h"
#include "LuaOpenGL.h"
#include "LuaUtils.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Shaders/Shader.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"

#define LOG_SECTION_LUA_RENDER "LuaRender"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_LUA_RENDER)

#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_LUA_RENDER

Shader::IProgramObject* LuaRender::sLineProgram = nullptr;
Shader::IProgramObject* LuaRender::sPolygonProgram = nullptr;
Shader::IProgramObject* LuaRender::sScreenRectProgram = nullptr;

GLuint LuaRender::sVertexArray = 0;
GLuint LuaRender::sVertexBuffer = 0;
const GLsizeiptr LuaRender::sVertexBufferSize;
GLuint LuaRender::sVertexBufferOffset = 0;

void LuaRender::Init()
{
	CheckGLError();

	LoadPrograms();

	glGenVertexArrays(1, &sVertexArray);

	glBindVertexArray(sVertexArray);
	glGenBuffers(1, &sVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, sVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sVertexBufferSize, nullptr, GL_STREAM_DRAW);
	sVertexBufferOffset = 0;
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CheckGLError();
}

void LuaRender::Free()
{
	// FIXME: Do not drop initial error state.
	glGetError();

	UnloadPrograms();

	glDeleteVertexArrays(1, &sVertexArray);
	sVertexArray = 0;

	glDeleteBuffers(1, &sVertexBuffer);
	sVertexBuffer = 0;
	sVertexBufferOffset = 0;

	CheckGLError();
}

bool LuaRender::PushEntries(lua_State* L)
{
	lua_pushstring(L, "Draw");
	lua_newtable(L);

	REGISTER_LUA_CFUNC(Vertices);
	REGISTER_LUA_CFUNC(Lines);
	REGISTER_LUA_CFUNC(Triangle);
	REGISTER_LUA_CFUNC(Rectangle);
	REGISTER_LUA_CFUNC(ReloadShaders);

	lua_rawset(L, -3);

	return true;
}

void LuaRender::LoadPrograms()
{
	sLineProgram = shaderHandler->CreateProgramObject("[LuaRenderLine]", "LuaRenderLine");
	sLineProgram->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/LuaRenderLineVertProg.glsl", "", GL_VERTEX_SHADER));
	sLineProgram->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/LuaRenderLineGeoProg.glsl", "", GL_GEOMETRY_SHADER));
	sLineProgram->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/LuaRenderLineFragProg.glsl", "", GL_FRAGMENT_SHADER));
	sLineProgram->Link();

	sLineProgram->Enable();
	sLineProgram->SetUniform("miterLimit", 0.5f);
	sLineProgram->SetUniform("texSampler", 0);
	sLineProgram->SetUniformLocation("viewMatrix");    // idx 0
	sLineProgram->SetUniformLocation("projMatrix");    // idx 1
	sLineProgram->SetUniformLocation("textureFlag");   // idx 2
	sLineProgram->SetUniformLocation("thickness");     // idx 3
	sLineProgram->SetUniformLocation("viewport");      // idx 4
	sLineProgram->SetUniformLocation("textureCoords"); // idx 5
	sLineProgram->SetUniformLocation("texturePhase");  // idx 6
	sLineProgram->SetUniformLocation("textureSpeed");  // idx 7
	sLineProgram->Disable();

	sLineProgram->Validate();
	if (!sLineProgram->IsValid()) {
		throw opengl_error(LOG_SECTION_LUA_RENDER ": failed to compile lines shader");
	}
	sPolygonProgram = shaderHandler->CreateProgramObject("[LuaRenderPolygon]", "LuaRenderPolygon");

	sPolygonProgram->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/LuaRenderPolygonVertProg.glsl", "", GL_VERTEX_SHADER));
	sPolygonProgram->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/LuaRenderPolygonFragProg.glsl", "", GL_FRAGMENT_SHADER));
	sPolygonProgram->Link();

	sPolygonProgram->Enable();
	sPolygonProgram->SetUniform("texSampler", 0);
	sPolygonProgram->SetUniformLocation("viewMatrix");  // idx 0
	sPolygonProgram->SetUniformLocation("projMatrix");  // idx 1
	sPolygonProgram->SetUniformLocation("textureFlag"); // idx 2
	sPolygonProgram->Disable();

	sPolygonProgram->Validate();
	if (!sPolygonProgram->IsValid()) {
		throw opengl_error(LOG_SECTION_LUA_RENDER ": failed to compile polygon shader");
	}

	sScreenRectProgram = shaderHandler->CreateProgramObject("[LuaRenderScreenRect]", "LuaRenderScreenRect");
	sScreenRectProgram->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/LuaRenderPolygonVertProg.glsl", "", GL_VERTEX_SHADER));
	sScreenRectProgram->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/LuaRenderScreenRectFragProg.glsl", "", GL_FRAGMENT_SHADER));
	sScreenRectProgram->Link();

	sScreenRectProgram->Enable();
	sScreenRectProgram->SetUniform("texSampler", 0);
	sScreenRectProgram->SetUniformLocation("viewMatrix");  // idx 0
	sScreenRectProgram->SetUniformLocation("projMatrix");  // idx 1
	sScreenRectProgram->SetUniformLocation("textureFlag"); // idx 2
	sScreenRectProgram->SetUniformLocation("viewport");    // idx 3
	sScreenRectProgram->SetUniformLocation("rect");        // idx 4
	sScreenRectProgram->SetUniformLocation("bevel");       // idx 5
	sScreenRectProgram->SetUniformLocation("radius");      // idx 6
	sScreenRectProgram->SetUniformLocation("border");      // idx 7
	sScreenRectProgram->Disable();

	sScreenRectProgram->Validate();
	if (!sScreenRectProgram->IsValid()) {
		throw opengl_error(LOG_SECTION_LUA_RENDER ": failed to compile screen rect shader");
	}
}

void LuaRender::UnloadPrograms()
{
	shaderHandler->ReleaseProgramObjects("[LuaRenderLine]");
	sLineProgram = nullptr;

	shaderHandler->ReleaseProgramObjects("[LuaRenderPolygon]");
	sPolygonProgram = nullptr;

	shaderHandler->ReleaseProgramObjects("[LuaRenderScreenRect]");
	sScreenRectProgram = nullptr;
}

void LuaRender::CheckDrawingEnabled(lua_State* L, const char* caller)
{
	if (LuaOpenGL::IsDrawingEnabled(L)) {
		return;
	}

	luaL_error(L, "%s(): Render calls can only be used in Draw() call-ins", caller);
}

void LuaRender::CheckGLError()
{
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		LOG_L(L_ERROR, "OpenGL error: 0x%X", error);
		assert(false);
	}
}

void LuaRender::Bind(Shader::IProgramObject* defaultProgram)
{
	// FIXME: Do not drop initial error state.
	glGetError();

	// Use default program if there is no active program.
	GLuint currentProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&currentProgram));
	if (currentProgram == 0 && defaultProgram != nullptr) {
		defaultProgram->Enable();

		GLint glTexture = 0;
		glActiveTexture(GL_TEXTURE0);
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &glTexture);

		// Use specified texture if there is one bound.
		if (glTexture == 0) {
			defaultProgram->SetUniform1f(2, 0.0f);
		} else {
			defaultProgram->SetUniform1f(2, 1.0f);
		}

		switch (LuaOpenGL::GetDrawMode()) {
			case LuaOpenGL::DRAW_SCREEN:
				defaultProgram->SetUniformMatrix4fv(0, false, LuaOpenGL::GetScreenViewMatrix());
				defaultProgram->SetUniformMatrix4fv(1, false, LuaOpenGL::GetScreenProjMatrix());
				break;

			case LuaOpenGL::DRAW_MINIMAP_BACKGROUND:
			case LuaOpenGL::DRAW_MINIMAP:
				defaultProgram->SetUniformMatrix4fv(0, false, minimap->GetViewMat(0));
				defaultProgram->SetUniformMatrix4fv(1, false, minimap->GetProjMat(0));
				break;

			default:
				defaultProgram->SetUniformMatrix4fv(0, false, camera->GetViewMatrix());
				defaultProgram->SetUniformMatrix4fv(1, false, camera->GetProjectionMatrix());
		}
	}

	// Shaders can use gl_PointSize.
	glEnable(GL_PROGRAM_POINT_SIZE);

	glBindVertexArray(sVertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, sVertexBuffer);

	CheckGLError();
}

void LuaRender::Unbind()
{
	if (sLineProgram->IsBound()) {
		sLineProgram->Disable();
	}
	if (sPolygonProgram->IsBound()) {
		sPolygonProgram->Disable();
	}
	if (sScreenRectProgram->IsBound()) {
		sScreenRectProgram->Disable();
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CheckGLError();
}

GLuint LuaRender::MapBuffer(GLuint numBytes, GLvoid** buffer)
{
	*buffer = nullptr;

	if (numBytes > sVertexBufferSize) {
		return 0;
	}

	GLbitfield access = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
	if (sVertexBufferOffset + numBytes > sVertexBufferSize) {
		// Request new buffer, orphan the old one.
		access |= GL_MAP_INVALIDATE_BUFFER_BIT;
		sVertexBufferOffset = 0;
	} else {
		// Stream to existing buffer.
		access |= GL_MAP_INVALIDATE_RANGE_BIT;
	}

	*buffer = glMapBufferRange(GL_ARRAY_BUFFER, sVertexBufferOffset, numBytes, access);

	GLuint offset = sVertexBufferOffset;
	sVertexBufferOffset += numBytes;

	return offset;
}

void LuaRender::UnmapBuffer()
{
	GLboolean success = glUnmapBuffer(GL_ARRAY_BUFFER);
	assert(success);
}

bool LuaRender::IsIntro()
{
	return !game->IsDoneLoading();
}

int LuaRender::GetDimensions()
{
	switch (LuaOpenGL::GetDrawMode()) {
		case LuaOpenGL::DRAW_SCREEN:
			return 2;

		case LuaOpenGL::DRAW_WORLD:
		case LuaOpenGL::DRAW_MINIMAP_BACKGROUND:
		case LuaOpenGL::DRAW_MINIMAP:
			return 3;

		default:
			return 3;
	}
}

bool LuaRender::ParseTexcoords(lua_State* L, TextureTransform& textureTransform, float texcoords[])
{
	if (lua_isnumber(L, -1)) {
		switch (lua_tointeger(L, -1)) {
			case 0:
				break;

			case 90:
				texcoords[1] = 1 - texcoords[1];
				texcoords[2] = 1 - texcoords[2];
				texcoords[4] = 1 - texcoords[4];
				texcoords[7] = 1 - texcoords[7];
				break;

			case 180:
				for (int i = 0; i < 8; ++i) {
					texcoords[i] = 1 - texcoords[i];
				}
				break;

			case 270:
				texcoords[0] = 1 - texcoords[0];
				texcoords[3] = 1 - texcoords[3];
				texcoords[5] = 1 - texcoords[5];
				texcoords[6] = 1 - texcoords[6];
				break;

			default:
				return false;
		}
	} else if (lua_isstring(L, -1)) {
		switch (hashString(lua_tostring(L, -1))) {
			case hashString("xflip"):
				for (int i = 0; i < 8; i += 2) {
					texcoords[i] = 1 - texcoords[i];
				}
				break;

			case hashString("yflip"):
				for (int i = 1; i < 8; i += 2) {
					texcoords[i] = 1 - texcoords[i];
				}
				break;

			case hashString("duflip"):
				for (int i = 2; i < 6; ++i) {
					texcoords[i] = 1 - texcoords[i];
				}
				break;

			case hashString("ddflip"):
				texcoords[0] = 1 - texcoords[0];
				texcoords[1] = 1 - texcoords[1];
				texcoords[6] = 1 - texcoords[6];
				texcoords[7] = 1 - texcoords[7];
				break;

			default:
				return false;
		}
	} else {
		// Parse custom texture coordinates.
		if (LuaUtils::ParseFloatArray(L, -1, texcoords, 8) < 8) {
			return false;
		}

		// Flip t coordinates to compensate for flipped textures.
		for (int i = 1; i < 8; i += 2) {
			texcoords[i] = 1 - texcoords[i];
		}

		// Swap last two vertices.
		std::swap(texcoords[4], texcoords[6]);
		std::swap(texcoords[5], texcoords[7]);
	}

	return true;
}

void LuaRender::TransformCoords(std::vector<float>& pos, bool relative)
{
	float vsx = static_cast<float>(globalRendering->viewSizeX);
	float vsy = static_cast<float>(globalRendering->viewSizeY);

	if (!IsIntro() && relative) {
		for (int i = 0; i < pos.size(); i += 2) {
			pos.at(i) *= vsx;
			pos.at(i + 1) *= vsy;
		}
	}

	if (IsIntro() && !relative) {
		for (int i = 0; i < pos.size(); i += 2) {
			pos.at(i) /= vsx;
			pos.at(i + 1) /= vsy;
		}
	}
}

void LuaRender::TransformParameters(float& bevel, float& radius, float& border, bool relative)
{
	if (relative) {
		float vsx = static_cast<float>(globalRendering->viewSizeX);
		float vsy = static_cast<float>(globalRendering->viewSizeY);
		float f = (vsx + vsy) / 2.0f;

		bevel *= f;
		radius *= f;
		border *= f;
	}
}

int LuaRender::Vertices(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	GLuint currentProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&currentProgram));
	if (currentProgram == 0) {
		luaL_error(L, "No shader program enabled");
		return 0;
	}

	const int args = lua_gettop(L);
	if (args < 2) {
		luaL_error(L, "Incorrect arguments to %s: need at least two arguments", __func__);
		return 0;
	}

	const int numVertices = luaL_checkfloat(L, 1);
	int bufferElements = 0;

	std::list<std::vector<float>> attributes;
	for (int i=2; i<=args; ++i) {
		std::vector<float> values;
		values.reserve(numVertices * 4);
		LuaUtils::ParseFloatTableFlattened(L, i, values);
		if (values.size() % numVertices != 0) {
			luaL_error(L, "Incorrect argument %d to %s: vertex attribute length not divisible by %d", i, __func__, numVertices);
			return 0;
		}
		attributes.push_back(values);
		bufferElements += values.size();
	}

	Bind();

	// Update buffer.
	float* buffer;
	GLuint offset = MapBuffer(bufferElements * sizeof(float), reinterpret_cast<GLvoid**>(&buffer));
	if (buffer == nullptr) {
		luaL_error(L, "Failed to map vertex buffer");
		return 0;
	}

	int o = 0;
	for (const auto &values: attributes) {
		memcpy(&buffer[o], values.data(), values.size() * sizeof(float));
		o += values.size();
	}

	UnmapBuffer();

	int index = 0;
	for (const auto &values: attributes) {
		glVertexAttribPointer(index, values.size() / numVertices, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(offset));
		glEnableVertexAttribArray(index);
		offset += values.size() * sizeof(float);
		++index;
	}

	glDrawArrays(GL_POINTS, 0, numVertices);

	for (int i=0; i<attributes.size(); ++i) {
		glDisableVertexAttribArray(i);
	}

	Unbind();

	return 0;
}

int LuaRender::Lines(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	std::vector<float> pos;
	pos.reserve(24);
	const int args = LuaUtils::ParseFloatsFlattened(L, 1, pos);

	int dim = GetDimensions();
	if (pos.size() % dim != 0) {
		luaL_error(L, "Incorrect arguments to %s: number of coords has to be a multiple of %d", __func__, dim);
		return 0;
	}
	if (pos.size() < 2 * dim) {
		luaL_error(L, "Incorrect arguments to %s: not enough coords given", __func__);
		return 0;
	}

	bool relative = IsIntro();

	float width = 1.0f;

	float color[] = {1.0f, 1.0f, 1.0f, 1.0f};
	std::vector<float> colors;
	colors.reserve(32);

	bool loop = false;

	float texcoords[] = {
		0.0f, 0.0f,
		1.0f, 1.0f
	};

	float animationSpeed = 0.0f;

	if (lua_istable(L, args + 1)) {
		for (lua_pushnil(L); lua_next(L, args + 1) != 0; lua_pop(L, 1)) {
			if (!lua_israwstring(L, -2)) {
				luaL_error(L, "Incorrect arguments to %s", __func__);
				return 0;
			}

			switch (hashString(lua_tostring(L, -2))) {
				case hashString("relative"):
					relative = luaL_checkboolean(L, -1);
					break;

				case hashString("width"):
					width = luaL_checkfloat(L, -1);
					break;

				case hashString("color"):
					if (LuaUtils::ParseFloatArray(L, -1, color, 4) < 3) {
						luaL_error(L, "Invalid color argument to %s", __func__);
						return 0;
					}
					break;

				case hashString("colors"):
					LuaUtils::ParseFloatTableFlattened(L, -1, colors);
					if (colors.size() < 4) {
						luaL_error(L, "Invalid colors argument to %s", __func__);
						return 0;
					}
					break;

				case hashString("loop"):
					loop = luaL_checkboolean(L, -1);
					break;

				case hashString("texcoords"):
					if (LuaUtils::ParseFloatArray(L, -1, texcoords, 4) != 4) {
						luaL_error(L, "Invalid texcoords argument to %s", __func__);
						return 0;
					}
					break;

				case hashString("animationspeed"):
					animationSpeed = luaL_checkfloat(L, -1);
					break;

				default:
					luaL_error(L, "Incorrect arguments to %s", __func__);
					return 0;
			}
		}
	}

	if (relative && dim != 2) {
		luaL_error(L, "Incorrect arguments to %s: relative not supported when drawing to world", __func__);
		return 0;
	}

	if (!colors.empty() && colors.size() / 4 != pos.size() / dim) {
		luaL_error(L, "Incorrect arguments to %s: coord and color counts do not match", __func__);
		return 0;
	}

	if (dim == 2) {
		TransformCoords(pos, relative);
	}

	Bind(sLineProgram);

	if (sLineProgram->IsBound()) {
		sLineProgram->SetUniform1f(3, width);
		sLineProgram->SetUniform2f(4, globalRendering->viewSizeX, globalRendering->viewSizeY);
		sLineProgram->SetUniform4fv(5, texcoords);
		sLineProgram->SetUniform1f(6, gu->gameTime);
		sLineProgram->SetUniform1f(7, animationSpeed);
	}

	int numVertices = pos.size() / dim;
	if (numVertices == 2) {
		loop = false;
	}
	if (loop) {
		++numVertices;
	}
	numVertices += 2;

	int vertexFloats = dim + 4;

	// Update buffer.
	float* buffer;
	GLuint offset = MapBuffer(numVertices * vertexFloats * sizeof(float), reinterpret_cast<GLvoid**>(&buffer));
	if (buffer == nullptr) {
		luaL_error(L, "Failed to map vertex buffer");
		return 0;
	}

	uint32_t o = vertexFloats;
	for (int i = 0; i < pos.size(); i += dim) {
		memcpy(&buffer[o], &pos.at(i), dim * sizeof(float));
		o += dim;

		if (!colors.empty()) {
			memcpy(&buffer[o], &colors.at(i / dim * 4), 4 * sizeof(float));
		} else {
			memcpy(&buffer[o], &color[0], 4 * sizeof(float));
		}
		o += 4;
	}

	if (loop) {
		// Vertex -1 is the last vertex, needed for correct starting miter.
		memcpy(&buffer[0], &buffer[o - vertexFloats], vertexFloats * sizeof(float));

		// Close the loop.
		memcpy(&buffer[o], &buffer[vertexFloats], vertexFloats * sizeof(float));
		o += vertexFloats;

		// Vertex +1 is the second vertex, needed for correct ending miter.
		memcpy(&buffer[o], &buffer[vertexFloats * 2], vertexFloats * sizeof(float));
	} else {
		// Duplicate first vertex.
		memcpy(&buffer[0], &buffer[vertexFloats], vertexFloats * sizeof(float));

		// Duplicate last vertex.
		memcpy(&buffer[o], &buffer[o - vertexFloats], vertexFloats * sizeof(float));
	}

	UnmapBuffer();

	glVertexAttribPointer(0, dim, GL_FLOAT, GL_FALSE, vertexFloats * sizeof(float), reinterpret_cast<GLvoid*>(offset));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, vertexFloats * sizeof(float), reinterpret_cast<GLvoid*>(offset + dim * sizeof(float)));
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_LINE_STRIP_ADJACENCY, 0, numVertices);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	Unbind();

	return 0;
}

int LuaRender::Triangle(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	std::vector<float> pos;
	pos.reserve(12);
	const int args = LuaUtils::ParseFloatsFlattened(L, 1, pos);

	int dim = GetDimensions();
	if (pos.size() != 3 * dim) {
		luaL_error(L, "Incorrect arguments to %s: number of coords has to be %d", __func__, 3 * dim);
		return 0;
	}

	bool relative = IsIntro();

	float color[] = {1.0f, 1.0f, 1.0f, 1.0f};
	std::vector<float> colors;
	colors.reserve(12);

	float texcoords[] = {
		0.0f, 1.0f,
		1.0f, 1.0f,
		0.0f, 0.0f
	};

	if (lua_istable(L, args + 1)) {
		for (lua_pushnil(L); lua_next(L, args + 1) != 0; lua_pop(L, 1)) {
			if (!lua_israwstring(L, -2)) {
				luaL_error(L, "Incorrect arguments to %s", __func__);
				return 0;
			}

			switch (hashString(lua_tostring(L, -2))) {
				case hashString("relative"):
					relative = luaL_checkboolean(L, -1);
					break;

				case hashString("color"):
					if (LuaUtils::ParseFloatArray(L, -1, color, 4) < 3) {
						luaL_error(L, "Invalid color argument to %s", __func__);
						return 0;
					}
					break;

				case hashString("colors"):
					LuaUtils::ParseFloatTableFlattened(L, -1, colors);
					if (colors.size() != 3 * 4) {
						luaL_error(L, "Invalid colors argument to %s", __func__);
						return 0;
					}
					break;

				case hashString("texcoords"):
					if (LuaUtils::ParseFloatArray(L, -1, texcoords, 6) != 6) {
						luaL_error(L, "Invalid texcoords argument to %s", __func__);
						return 0;
					}
					break;

				default:
					luaL_error(L, "Incorrect arguments to %s", __func__);
					return 0;
			}
		}
	}

	if (relative && dim != 2) {
		luaL_error(L, "Incorrect arguments to %s: relative not supported when drawing to world", __func__);
		return 0;
	}

	if (dim == 2) {
		TransformCoords(pos, relative);
	}

	int numVertices = 3;

	Bind(sPolygonProgram);

	// Update buffer.
	float* buffer;
	GLuint offset = MapBuffer(numVertices * (dim + 4 + 2) * sizeof(float), reinterpret_cast<GLvoid**>(&buffer));
	if (buffer == nullptr) {
		luaL_error(L, "Failed to map vertex buffer");
		return 0;
	}

	int o = 0;
	memcpy(&buffer[o], pos.data(), pos.size() * sizeof(float));
	o += pos.size();

	if (colors.empty()) {
		for (int i = 0; i < numVertices; ++i) {
			memcpy(&buffer[o], &color[0], 4 * sizeof(float));
			o += 4;
		}
	} else {
		memcpy(&buffer[o], &colors.at(0), 12 * sizeof(float));
		o += 12;
	}

	memcpy(&buffer[o], &texcoords[0], 6 * sizeof(float));

	UnmapBuffer();

	glVertexAttribPointer(0, dim, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(offset));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(offset + numVertices * dim * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(offset + numVertices * (dim + 4) * sizeof(float)));
	glEnableVertexAttribArray(2);

	glDrawArrays(GL_TRIANGLES, 0, numVertices);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	Unbind();

	return 0;
}

int LuaRender::Rectangle(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);

	std::vector<float> pos;
	pos.reserve(12);
	const int args = LuaUtils::ParseFloatsFlattened(L, 1, pos);

	int dim = GetDimensions();
	if (pos.size() % dim != 0) {
		luaL_error(L, "Incorrect arguments to %s: number of coords has to be a multiple of %d", __func__, dim);
		return 0;
	}

	if (dim == 2) {
		if (pos.size() != dim * 2) {
			luaL_error(L, "Incorrect arguments to %s: invalid number of coords given", __func__);
			return 0;
		}
		if (pos.at(0) > pos.at(2)) {
			std::swap(pos.at(0), pos.at(2));
		}
		if (pos.at(1) > pos.at(3)) {
			std::swap(pos.at(1), pos.at(3));
		}
	} else if (pos.size() != dim * 4) {
		luaL_error(L, "Incorrect arguments to %s: invalid number of coords given", __func__);
		return 0;
	}

	bool relative = IsIntro();

	float bevel = 0.0f;

	float radius = 0.0f;

	float color[] = {1.0f, 1.0f, 1.0f, 1.0f};
	std::vector<float> colors;
	colors.reserve(12);

	TextureTransform textureTransform = TextureTransform::ROTATE0;

	// s, t. t coordinates are flipped to compensate for flipped textures.
	float texcoords[] = {
		0.0f, 1.0f,
		1.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
	};

	float border = 0.0f;
	float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
	std::vector<float> borderColors;
	borderColors.reserve(12);

	if (lua_istable(L, args + 1)) {
		for (lua_pushnil(L); lua_next(L, args + 1) != 0; lua_pop(L, 1)) {
			if (!lua_israwstring(L, -2)) {
				luaL_error(L, "Incorrect arguments to %s", __func__);
				return 0;
			}

			switch (hashString(lua_tostring(L, -2))) {
				case hashString("relative"):
					relative = luaL_checkboolean(L, -1);
					break;

				case hashString("bevel"):
					bevel = luaL_checkfloat(L, -1);
					break;

				case hashString("radius"):
					radius = luaL_checkfloat(L, -1);
					break;

				case hashString("color"):
					if (LuaUtils::ParseFloatArray(L, -1, color, 4) < 3) {
						luaL_error(L, "Invalid color argument to %s", __func__);
						return 0;
					}
					break;

				case hashString("colors"):
					LuaUtils::ParseFloatTableFlattened(L, -1, colors);
					if (colors.size() != 4 * 4) {
						luaL_error(L, "Invalid colors argument to %s", __func__);
						return 0;
					}

					// Swap last two vertices.
					for (int i = 0; i < 4; ++i) {
						std::swap(colors[8 + i], colors[12 + i]);
					}
					break;

				case hashString("texcoords"):
					if (!ParseTexcoords(L, textureTransform, texcoords)) {
						luaL_error(L, "Invalid texcoords argument to %s", __func__);
						return 0;
					}
					break;

				case hashString("border"):
					border = luaL_checkfloat(L, -1);
					break;

				case hashString("bordercolor"):
					if (LuaUtils::ParseFloatArray(L, -1, borderColor, 4) < 3) {
						luaL_error(L, "Invalid bordercolor argument to %s", __func__);
						return 0;
					}
					break;

				case hashString("bordercolors"):
					LuaUtils::ParseFloatsFlattened(L, -1, borderColors);
					if (borderColors.size() != 4 * 4) {
						luaL_error(L, "Invalid bordercolors argument to %s", __func__);
						return 0;
					}

					// Swap last two vertices.
					for (int i = 0; i < 4; ++i) {
						std::swap(borderColors[8 + i], borderColors[12 + i]);
					}
					break;

				default:
					luaL_error(L, "Incorrect arguments to %s", __func__);
					return 0;
			}
		}
	}

	if (relative && dim != 2) {
		luaL_error(L, "Incorrect arguments to %s: relative not supported when drawing to world", __func__);
		return 0;
	}

	if (bevel != 0 && radius != 0) {
		luaL_error(L, "Incorrect arguments to %s: bevel and radius incompatible", __func__);
		return 0;
	}

	if (dim == 3 && (border != 0 || bevel != 0 || radius != 0)) {
		luaL_error(L, "Incorrect arguments to %s: border, bevel, radius not supported in world", __func__);
		return 0;
	}

	if (!colors.empty() && colors.size() != 16) {
		luaL_error(L, "Incorrect arguments to %s: invalid number of colors given", __func__);
		return 0;
	}

	if (dim == 2) {
		TransformCoords(pos, relative);
		TransformParameters(bevel, radius, border, relative);
	}

	float x1, y1, x2, y2;
	if (dim == 2) {
		x1 = pos.at(0);
		y1 = pos.at(1);
		x2 = pos.at(2);
		y2 = pos.at(3);
	} else {
		std::swap(pos.at(6), pos.at(9));
		std::swap(pos.at(7), pos.at(10));
		std::swap(pos.at(8), pos.at(11));
	}

	int numVertices = 4;

	if (dim == 2 && (bevel != 0 || radius != 0 || border != 0)) {
		Bind(sScreenRectProgram);

		if (sScreenRectProgram->IsBound()) {
			float vsx = static_cast<float>(globalRendering->viewSizeX);
			float vsy = static_cast<float>(globalRendering->viewSizeY);
			sScreenRectProgram->SetUniform2f(3, vsx, vsy);

			if (IsIntro()) {
				// The intro renders to relative coordinates, but the fragment shader needs screen coordinates.
				sScreenRectProgram->SetUniform4f(4, x1 * vsx, y1 * vsy, x2 * vsx, y2 * vsy);
			} else {
				sScreenRectProgram->SetUniform4f(4, x1, y1, x2, y2);
			}

			sScreenRectProgram->SetUniform1f(5, bevel);
			sScreenRectProgram->SetUniform1f(6, radius);
			sScreenRectProgram->SetUniform1f(7, border);
		}
	} else {
		Bind(sPolygonProgram);
	}

	// Update buffer.
	float* buffer;
	GLuint offset = MapBuffer(numVertices * (dim + 4 + 2 + 4) * sizeof(float), reinterpret_cast<GLvoid**>(&buffer));
	if (buffer == nullptr) {
		luaL_error(L, "Failed to map vertex buffer");
		return 0;
	}

	std::vector<float> posStrip;
	posStrip.reserve(16);
	std::vector<float> borderPos;
	borderPos.reserve(24);

	float tc[8];
	memcpy(&tc[0], &texcoords[0], 8 * sizeof(float));

	switch (textureTransform) {
		case TextureTransform::ROTATE0:
			break;

		case TextureTransform::ROTATE90:
			tc[1] = 1 - tc[1];
			tc[2] = 1 - tc[2];
			tc[4] = 1 - tc[4];
			tc[7] = 1 - tc[7];
			break;

		case TextureTransform::ROTATE180:
			for (int i = 0; i < 8; ++i) {
				tc[i] = 1 - tc[i];
			}
			break;

		case TextureTransform::ROTATE270:
			tc[0] = 1 - tc[0];
			tc[3] = 1 - tc[3];
			tc[5] = 1 - tc[5];
			tc[6] = 1 - tc[6];
			break;

		case TextureTransform::XFLIP:
			for (int i = 0; i < 8; i += 2) {
				tc[i] = 1 - tc[i];
			}
			break;

		case TextureTransform::YFLIP:
			for (int i = 1; i < 8; i += 2) {
				tc[i] = 1 - tc[i];
			}
			break;

		case TextureTransform::DUFLIP:
			for (int i = 2; i < 6; ++i) {
				tc[i] = 1 - tc[i];
			}
			break;

		case TextureTransform::DDFLIP:
			tc[0] = 1 - tc[0];
			tc[1] = 1 - tc[1];
			tc[6] = 1 - tc[6];
			tc[7] = 1 - tc[7];
			break;
	}

	if (dim == 2) {
		buffer[0] = x1; buffer[1] = y1;
		buffer[2] = x2; buffer[3] = y1;
		buffer[4] = x1; buffer[5] = y2;
		buffer[6] = x2; buffer[7] = y2;
	} else {
		memcpy(&buffer[0], pos.data(), pos.size() * sizeof(float));
	}

	if (colors.empty()) {
		for (int i = 0; i < 4; ++i) {
			memcpy(&buffer[4 * dim + i * 4], &color[0], 4 * sizeof(float));
		}
	} else {
		memcpy(&buffer[4 * dim], colors.data(), 16 * sizeof(float));
	}

	memcpy(&buffer[4 * dim + 16], &tc[0], 8 * sizeof(float));

	if (borderColors.empty()) {
		for (int i = 0; i < 4; ++i) {
			memcpy(&buffer[4 * dim + 16 + 8 + i * 4], &borderColor[0], 4 * sizeof(float));
		}
	} else {
		memcpy(&buffer[4 * dim + 16 + 8], borderColors.data(), 16 * sizeof(float));
	}

	UnmapBuffer();

	glVertexAttribPointer(0, dim, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(offset));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(offset + numVertices * dim * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(offset + numVertices * (dim + 4) * sizeof(float)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid*>(offset + numVertices * (dim + 4 + 2) * sizeof(float)));
	glEnableVertexAttribArray(3);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, numVertices);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);

	Unbind();

	return 0;
}

int LuaRender::ReloadShaders(lua_State* L)
{
	for (Shader::IProgramObject* program : {sLineProgram, sPolygonProgram, sScreenRectProgram}) {
		bool changed = false;
		for (auto shader : program->GetShaderObjs()) {
			changed |= shader->ReloadFromTextOrFile();
		}
		if (changed) {
			program->Reload(true, true);
		}
	}
	return 0;
}
