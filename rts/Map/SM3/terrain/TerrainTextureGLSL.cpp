/*
---------------------------------------------------------------------
   Terrain Renderer using texture splatting and geomipmapping
   Copyright (c) 2006 Jelmer Cnossen

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any
   purpose, including commercial applications, and to alter it and
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
      distribution.

   Jelmer Cnossen
   j.cnossen at gmail dot com
---------------------------------------------------------------------
*/
#include "StdAfx.h"

#include "TerrainBase.h"
#include "TerrainTexture.h"

#include "TerrainTextureGLSL.h"
#include "Rendering/GL/IFramebuffer.h"
#include "FileSystem/FileHandler.h"
#include "bitops.h"

#include <fstream>

namespace terrain {
using namespace std;

static void ShowInfoLog(GLhandleARB handle)
{
	d_trace("Shader failed to compile, showing info log:\n");
	int infoLogLen, actualLength;
	glGetObjectParameterivARB(handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &infoLogLen);
	char *infoLog = new char[infoLogLen];
	glGetInfoLogARB(handle, infoLogLen, &actualLength, infoLog);
	d_puts(infoLog);
	delete[] infoLog;
}

struct Shader
{
	list<string> texts;
	GLhandleARB handle;

	Shader() { handle = 0; }

	void AddFile(const std::string& file)
	{
		CFileHandler fh(file);
		if (!fh.FileExists())
			throw content_error("Can't load shader " + file);

		string text;
		text.resize(fh.FileSize());
		fh.Read(&text[0], text.length());

		texts.push_back(text);
	}

	void Build(GLenum shaderType)
	{
		handle = glCreateShaderObjectARB(shaderType);

		vector<int> lengths(texts.size());
		vector<const GLcharARB*> strings(texts.size());
		int index=0;
		for (list<string>::iterator i=texts.begin(); i != texts.end(); ++i, index++) {
			lengths[index] = i->length();
			strings[index] = i->c_str();
		}

		//if (shaderType == GL_FRAGMENT_SHADER_ARB)
		//	DebugOutput(shaderType);

		glShaderSourceARB(handle, strings.size(), &strings.front(), &lengths.front());
		glCompileShaderARB(handle);

		// Check compile status and show info log if failed
		int isCompiled;
		glGetObjectParameterivARB(handle, GL_OBJECT_COMPILE_STATUS_ARB, &isCompiled);
		if (!isCompiled)
		{
			ShowInfoLog(handle);

			string errMsg = "Failed to build ";
			throw std::runtime_error (errMsg + (shaderType == GL_VERTEX_SHADER_ARB ? "vertex shader" : "fragment shader"));
		}
	}
	void DebugOutput(GLenum shaderType)
	{
		char fn[20];
		static int fpc=0;
		static int vpc=0;
		if (shaderType == GL_FRAGMENT_SHADER_ARB) sprintf (fn, "shader%dfp.txt", fpc++);
		else sprintf (fn, "shader%dvp.txt", vpc++);
		FILE *f = fopen(fn, "w");
		if(f) {
			for (list<string>::iterator i=texts.begin();i!=texts.end();++i)
				fputs(i->c_str(), f);
			fclose(f);
		}
	}
};

static int closest_pot(int i)
{
	int next = next_power_of_2(i);
	return (next - i < i - next/2) ? next : next/2;
}

// A framebuffer enabled as texture
class BufferTexture : public BaseTexture
{
public:
	BufferTexture() 
	{ 
	// ATI has GL_EXT_texture_rectangle, but that has no support for GLSL texture2DRect
	// nVidia: Use RECT,  ati: use POT
		width = gu->screenx;
		height = gu->screeny;
		if (GLEW_ARB_texture_rectangle) 
			target = GL_TEXTURE_RECTANGLE_ARB;
		else {
			target = GL_TEXTURE_2D;
			width = closest_pot(width);
			height = closest_pot(height);
		}

		framebuffer = instantiate_fb(width, height, FBO_NEED_COLOR | FBO_NEED_DEPTH); 
		name = "_buffer";

		glGenTextures(1, &id);
		glBindTexture(target, id);
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexImage2D(target, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

		framebuffer->attachTexture(id, target, FBO_ATTACH_COLOR);

		assert (framebuffer->valid());
	}
	~BufferTexture() 
	{
		delete framebuffer;
		// texture is deleted by ~BaseTexture
	}
	bool IsRect() {	return target == GL_TEXTURE_RECTANGLE_ARB; }

	int width, height;
	uint target;
	IFramebuffer* framebuffer;
};

struct ShaderBuilder
{
	RenderSetup *renderSetup;
	int maxTexUnits, maxTexCoords;
	NodeGLSLShader *nodeShader;
	TextureUsage texUsage;
	vector<BufferTexture*> *buffers;

	enum PassType { P_Diffuse, P_Bump, P_Combined };

	ShaderBuilder(RenderSetup *rs,vector<BufferTexture*>* buffers) :
		renderSetup(rs), buffers(buffers)
	{
		glGetIntegerv (GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &maxTexUnits);
		glGetIntegerv (GL_MAX_TEXTURE_COORDS_ARB, &maxTexCoords);

		nodeShader = 0;
	}

	std::string GenTextureRead (int tu, int tc) {
		char tcstr[6];
		sprintf (tcstr,"%d", tc);
		return "texture2D(" + texUsage.texUnits[tu]->name + ", gl_TexCoord[" + tcstr + "].st)";
	}

	NodeGLSLShader* EndPass(ShaderDef* sd, PassType type, const std::string &operations, bool depthWrite)
	{
		if (type == P_Diffuse)
			nodeShader->vertBufReq = VRT_Normal;
		else
			nodeShader->vertBufReq = VRT_TangentSpaceMatrix;
		//nodeShader->vertBufReq = VRT_Normal | VRT_TangentSpaceMatrix;

		nodeShader->texCoordGen = texUsage.coordUnits;
		nodeShader->texUnits = texUsage.texUnits;

		BuildVertexShader(type);
		BuildFragmentShader(type, operations, sd);

		nodeShader->program = glCreateProgramObjectARB();
		glAttachObjectARB(nodeShader->program, nodeShader->vertexShader);
		glAttachObjectARB(nodeShader->program, nodeShader->fragmentShader);

		glLinkProgramARB(nodeShader->program);
		int isLinked;
		glGetObjectParameterivARB(nodeShader->program, GL_OBJECT_LINK_STATUS_ARB, &isLinked);
		if (!isLinked)
		{
			d_trace ("Failed to link shaders. Showing info log:\n");
			ShowInfoLog (nodeShader->program);
			throw std::runtime_error("Failed to link shaders");
		}

		glUseProgramObjectARB(nodeShader->program);

		// set texture image units to texture samplers in the shader
		for (int a=0;a<nodeShader->texUnits.size();a++)
		{
			BaseTexture *tex = nodeShader->texUnits[a];
			GLint location = glGetUniformLocationARB(nodeShader->program, tex->name.c_str());
			glUniform1iARB(location, a);
		}

		if (nodeShader->vertBufReq & VRT_TangentSpaceMatrix)
			nodeShader->tsmAttrib = glGetAttribLocationARB(nodeShader->program,"TangentSpaceMatrix");

		nodeShader->wsLightDirLocation = glGetUniformLocationARB(nodeShader->program, "wsLightDir");
		nodeShader->wsEyePosLocation = glGetUniformLocationARB(nodeShader->program, "wsEyePos");

		if (buffers && !buffers->empty()) {
			BufferTexture *bt = buffers->front();
			GLint invScreenDim = glGetUniformLocationARB(nodeShader->program, "invScreenDim");
			glUniform2fARB(invScreenDim, 1.0f/gu->screenx, 1.0f/gu->screeny);
		}

		glUseProgramObjectARB(0);

		renderSetup->passes.push_back (RenderPass());
		RenderPass& rp = renderSetup->passes.back();
		rp.shaderSetup = nodeShader;
		rp.operation = Pass_Replace;
		rp.depthWrite = depthWrite;
		nodeShader->debugstr = operations;
		NodeGLSLShader *ns = nodeShader;
		nodeShader = 0;
		texUsage = TextureUsage();
		return ns;
	}

	void BuildFragmentShader(PassType type, const std::string& operations, ShaderDef* sd)
	{
		// insert texture samplers
		string textureSamplers;
		for (int a=0;a<nodeShader->texUnits.size();a++) {
			BaseTexture *tex = nodeShader->texUnits[a];
			if (tex->IsRect())
				textureSamplers += "uniform sampler2DRect " + tex->name + ";\n";
			else
				textureSamplers += "uniform sampler2D " + tex->name + ";\n";
		}

		Shader fragmentShader;
		if (type != P_Diffuse) fragmentShader.texts.push_back("#define UseBumpmapping\n");
		if (GLEW_ARB_texture_rectangle) fragmentShader.texts.push_back("#define UseTextureRECT\n");
		if (type == P_Bump) fragmentShader.texts.push_back("#define DiffuseFromBuffer\n");
		fragmentShader.AddFile("shaders/terrainCommon.glsl");
		fragmentShader.texts.push_back(textureSamplers);
		char specularExponentStr[20];
		SNPRINTF(specularExponentStr, 20, "%5.3f", sd->specularExponent);
		fragmentShader.texts.push_back(string("const float specularExponent = ") + specularExponentStr + ";\n");
		fragmentShader.AddFile("shaders/terrainFragmentShader.glsl");

		string gentxt = "vec4 CalculateColor()  { vec4 color; float curalpha; \n" + operations;
		if (type == P_Diffuse) // Diffuse color only
			gentxt += "return color; }\n";
		else if (type == P_Combined)  // Diffuse + bumpmap calculation all done in 1 shader
			gentxt += "return CalcFinal(diffuse, color);}\n";
		else if (type == P_Bump) // final bump calculation, diffuse comes from buffer
			gentxt += "return CalcFinal(ReadDiffuseColor(), color);}\n";

		fragmentShader.texts.push_back (gentxt);
		fragmentShader.Build(GL_FRAGMENT_SHADER_ARB);
		nodeShader->fragmentShader = fragmentShader.handle;
	}

	void BuildVertexShader(PassType passType)
	{
		Shader vertexShader;
		if (passType != P_Diffuse) vertexShader.texts.push_back("#define UseBumpmapping\n");
		vertexShader.AddFile("shaders/terrainCommon.glsl");

		// generate texture coords
		std::string tcgen = "void CalculateTexCoords() {\n";
		for (int a=0;a<nodeShader->texCoordGen.size();a++) {
			BaseTexture* tex = nodeShader->texCoordGen[a];
			char buf[160];
			sprintf (buf, "gl_TexCoord[%d].st = vec2(dot(gl_Vertex, gl_ObjectPlaneS[%d]), dot(gl_Vertex,gl_ObjectPlaneT[%d]));\n", a, a, a);
			tcgen += buf;
		}
		tcgen += "}\n";
		vertexShader.texts.push_back (tcgen);

		vertexShader.AddFile("shaders/terrainVertexShader.glsl");
		vertexShader.Build(GL_VERTEX_SHADER_ARB);
		d_trace("Vertex shader build succesfully.");

		nodeShader->vertexShader = vertexShader.handle;
	}

	
	bool ProcessStage(vector<ShaderDef::Stage>& stages, uint &index, std::string& opstr)
	{
		ShaderDef::Stage& stage = stages[index];
		BaseTexture *texture = stage.source;

		TextureUsage tmpUsage = texUsage;
		int tu = tmpUsage.AddTextureRead (maxTexUnits, texture);
		int tc = tmpUsage.AddTextureCoordRead (maxTexCoords, texture);

		if (tu < 0 || tc < 0)
			return false;

		if (index == 0) {  // replace
			texUsage = tmpUsage;
			opstr = "color = " + GenTextureRead(tu,tc) + ";\n";
		}
		else if(stage.operation == ShaderDef::Alpha) {
			// next operation is blend (alpha is autoinserted before blend)
			assert (index < stages.size()-1 && stages[index+1].operation == ShaderDef::Blend);
			ShaderDef::Stage& blendStage = stages[index+1];

			int blendTU = tmpUsage.AddTextureRead(maxTexUnits, blendStage.source);
			int blendTC = tmpUsage.AddTextureCoordRead(maxTexCoords, blendStage.source);

			if (blendTU < 0 || blendTC < 0)
				return false;

			index++;

			texUsage = tmpUsage;
			opstr += "curalpha = " + GenTextureRead(tu, tc) + ".a;\n";
			opstr += "color = mix(color, " + GenTextureRead(blendTU, blendTC) + ", curalpha);\n";
		}
		else if (stage.operation == ShaderDef::Add) {
			texUsage = tmpUsage;
			opstr += "color += " + GenTextureRead(tu, tc) + ";\n";
		} else if (stage.operation == ShaderDef::Mul)  {
			texUsage = tmpUsage;
			opstr += "color *= " + GenTextureRead(tu, tc) + ";\n";
		}
		index++;
		return true;
	}

	void ProcessStagesMultipass(ShaderDef* shaderDef, uint c, vector<NodeGLSLShader*> *diffusePasses)
	{
		std::string opstr;

		// Switch to multipass when the stages couldn't all be handled with multitexturing
		while (c < shaderDef->stages.size()) 
		{
			ShaderDef::Stage& st = shaderDef->stages[c];
			nodeShader = new NodeGLSLShader;

			if (st.operation == ShaderDef::Alpha) {
				ShaderDef::Stage& blendst = shaderDef->stages[c+1];

				opstr = "color=" + GenTextureRead(
					texUsage.AddTextureRead(maxTexUnits, blendst.source),
					texUsage.AddTextureCoordRead(maxTexUnits, blendst.source)) + ";\n";
					
				opstr += "color.a=" + GenTextureRead(
					texUsage.AddTextureRead(maxTexUnits, st.source),
					texUsage.AddTextureCoordRead(maxTexCoords, st.source)) + ".a;\n";

				diffusePasses->push_back(EndPass(shaderDef, P_Diffuse, opstr, false));
				renderSetup->passes.back().operation = Pass_Interpolate;
				c++; // skip the blend stage because we used it here already
			}
			else if (st.operation == ShaderDef::Add || st.operation == ShaderDef::Mul)
			{
				opstr = "color=" + GenTextureRead(
					texUsage.AddTextureRead(maxTexUnits, st.source),
					texUsage.AddTextureCoordRead(maxTexCoords, st.source)) + ";\n";

				diffusePasses->push_back(EndPass(shaderDef, P_Diffuse, opstr, false));
				renderSetup->passes.back().operation = 
					(st.operation == ShaderDef::Add) ? Pass_Add : Pass_Mul;
			}

			c++;
		}
	}

	void Build(ShaderDef* shaderDef)
	{
		nodeShader = new NodeGLSLShader;
		texUsage = TextureUsage();
		string opstr;
		vector<NodeGLSLShader*> diffusePasses;

		uint stage = 0;
		while (stage < shaderDef->stages.size())
		{
			if (!ProcessStage (shaderDef->stages, stage, opstr))
			{
				diffusePasses.push_back(EndPass(shaderDef, P_Diffuse, opstr, stage == 0));
				opstr.clear();
				break;
			}
		}

		if (!nodeShader) {
			// finish the stages with multipass
			ProcessStagesMultipass(shaderDef, stage, &diffusePasses);
		}

		// handle bumpmap stages
		if (!shaderDef->normalMapStages.empty()) {
			PassType passType = nodeShader ? P_Combined : P_Bump;

			string colorcalc = opstr; // the remaining diffuse calculations
			opstr.clear();

			// try to see if the bumpmap phase fits in with the diffuse calculation
			if (nodeShader) {
				stage = 0;
				TextureUsage old = texUsage;
				while (stage < shaderDef->normalMapStages.size())
				{
					if (!ProcessStage(shaderDef->normalMapStages, stage, opstr))
						break;
				}
				if (stage < shaderDef->normalMapStages.size()) {
					// It failed, so restore the diffuse phase and end it
					texUsage = old;
					diffusePasses.push_back(EndPass(shaderDef, P_Diffuse, colorcalc, true));
				} else {
					// success
					EndPass(shaderDef, P_Combined, colorcalc + "vec4 diffuse = color;\n" + opstr, true);
					return;
				}
			}

			nodeShader = new NodeGLSLShader;

			// multipass: let the diffuse passes render to the buffer
			// at this point nodeShader=0 and texUsage is empty
			if (buffers->empty()) buffers->push_back(new BufferTexture);
			for (uint a=0;a<diffusePasses.size();a++)
				diffusePasses[a]->renderBuffer = buffers->back();
			texUsage.AddTextureRead(maxTexUnits, buffers->back());

			opstr.clear();
			for (stage=0; stage < shaderDef->normalMapStages.size(); )
			{
				if (!ProcessStage(shaderDef->normalMapStages, stage, opstr))
					throw content_error("Map has too many textures/bumpmaps/different tilesizes");
			}

			EndPass(shaderDef, P_Bump, opstr, true);

		} else if (nodeShader) 
			EndPass(shaderDef, P_Diffuse, opstr, true);
	}
};

NodeGLSLShader::NodeGLSLShader()
{
	vertexShader = program = fragmentShader = 0;

	vertBufReq = 0;
	tsmAttrib = -1;
	wsLightDirLocation = wsEyePosLocation = -1;

	renderBuffer = 0;
}

NodeGLSLShader::~NodeGLSLShader()
{
	if (program) {
		glDetachObjectARB(program,vertexShader);
		glDetachObjectARB(program,fragmentShader);
		glDeleteObjectARB(program);
	}
	if (fragmentShader) glDeleteObjectARB(fragmentShader);
	if (vertexShader) glDeleteObjectARB(vertexShader);
}



void NodeGLSLShader::BindTSM (Vector3* buf, uint vertexSize)
{
// according to the GL_ARB_vertex_shader spec:
// The VertexAttrib*ARB entry points defined earlier can also be used to
// load attributes declared as a 2x2, 3x3 or 4x4 matrix in a vertex shader.
// Each column of a matrix takes up one generic 4-component attribute slot
// out of the MAX_VERTEX_ATTRIBS_ARB available slots. Matrices are loaded
// into these slots in column major order. Matrix columns need to be loaded
// in increasing slot numbers.
	if (tsmAttrib >= 0) {
		for (int a=0;a<3;a++) {
			glEnableVertexAttribArrayARB(tsmAttrib+a);
			glVertexAttribPointerARB(tsmAttrib+a, 3, GL_FLOAT, 0, vertexSize, buf + a);
		}
	}
}

void NodeGLSLShader::UnbindTSM ()
{
	if (tsmAttrib >= 0) {
		for (int a=0;a<3;a++)
			glDisableVertexAttribArrayARB(tsmAttrib+a);
	}
}

void NodeGLSLShader::Setup (NodeSetupParams& params)
{
	if (renderBuffer) { // use a offscreen rendering buffer
		renderBuffer->framebuffer->select();
		glViewport(0, 0, renderBuffer->width, renderBuffer->height);
	}

	glUseProgramObjectARB(program);
	for (int a=0;a<texUnits.size();a++) {
		glActiveTextureARB( GL_TEXTURE0_ARB+a);
		
		GLenum target;
		if (texUnits[a]->IsRect()) target = GL_TEXTURE_RECTANGLE_ARB;
		else target = GL_TEXTURE_2D;

		if (texUnits[a]->id) glBindTexture(target, texUnits[a]->id);
		glEnable (target);
	}
	for (int a=0;a<texCoordGen.size();a++) {
		glActiveTextureARB(GL_TEXTURE0_ARB+a);
		texCoordGen[a]->SetupTexGen ();
	}
	glActiveTextureARB(GL_TEXTURE0_ARB);

	if (wsLightDirLocation >= 0 && params.wsLightDir)
		glUniform3fARB(wsLightDirLocation, params.wsLightDir->x, params.wsLightDir->y, params.wsLightDir->z);
	if (wsEyePosLocation >= 0 && params.wsEyePos)
		glUniform3fARB(wsEyePosLocation, params.wsEyePos->x, params.wsEyePos->y, params.wsEyePos->z);
}

void NodeGLSLShader::Cleanup()
{
	for (int a=0;a<texUnits.size();a++) {
		glActiveTextureARB( GL_TEXTURE0_ARB+a);
		glDisable(texUnits[a]->IsRect() ? GL_TEXTURE_RECTANGLE_ARB : GL_TEXTURE_2D);
	}
	glActiveTextureARB(GL_TEXTURE0_ARB);

	glUseProgramObjectARB(0);

	if (renderBuffer) {
		renderBuffer->framebuffer->deselect();
		glViewport(0, 0, gu->screenx, gu->screeny);
	}
}

std::string NodeGLSLShader::GetDebugDesc ()
{
	return debugstr;
}

uint NodeGLSLShader::GetVertexDataRequirements ()
{
	return vertBufReq;
}
void NodeGLSLShader::GetTextureUnits(BaseTexture* tex, int &imageUnit, int& coordUnit)
{
}

GLSLShaderHandler::GLSLShaderHandler()
{
	curShader = 0;
}

GLSLShaderHandler::~GLSLShaderHandler()
{
	for (uint a=0;a<buffers.size();a++)
		delete buffers[a];
}

void GLSLShaderHandler::EndTexturing ()
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);

	if (curShader) {
		curShader->Cleanup();
		curShader = 0;
	}
}

void GLSLShaderHandler::BeginTexturing()
{
	if (!buffers.empty()) {
		buffers.front()->framebuffer->select();
		glViewport(0, 0, buffers.front()->width, buffers.front()->height);
		glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
		buffers.front()->framebuffer->deselect();
		glViewport(0, 0, gu->screenx, gu->screeny);
	}
}

void GLSLShaderHandler::BeginPass(const std::vector<Blendmap*>& blendmaps, const std::vector<TiledTexture*>& textures)
{}

bool GLSLShaderHandler::SetupShader (IShaderSetup *ps, NodeSetupParams& params)
{
	NodeGLSLShader* shader=static_cast<NodeGLSLShader*>(ps);

	if (curShader)
		curShader->Cleanup();

	shader->Setup(params);
	curShader = shader;
	return true;
}


void GLSLShaderHandler::BuildNodeSetup (ShaderDef *shaderDef, RenderSetup *renderSetup)
{
	ShaderBuilder shaderBuilder (renderSetup, &buffers);
	shaderBuilder.Build(shaderDef);
}

int GLSLShaderHandler::MaxTextureUnits ()
{
	int n;
	glGetIntegerv (GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &n);
	return n;
}

int GLSLShaderHandler::MaxTextureCoords ()
{
	int n;
	glGetIntegerv (GL_MAX_TEXTURE_COORDS_ARB, &n);
	return n;
}
};

