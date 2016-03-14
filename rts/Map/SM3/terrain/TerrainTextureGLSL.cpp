/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "TerrainBase.h"
#include "TerrainTexture.h"
#include "Terrain.h"

#include "TerrainTextureGLSL.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GlobalRendering.h"
#include "System/bitops.h"
#include "System/Util.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileQueryFlags.h"

#include <fstream>
#include <list>
#include <assert.h>

namespace terrain {

static void ShowInfoLog(GLhandleARB handle)
{
	LOG_L(L_ERROR, "Shader failed to compile, showing info log:");
	GLint infoLogLen;
	GLsizei actualLength;
	glGetObjectParameterivARB(handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &infoLogLen);
	char* infoLog = new char[infoLogLen];
	glGetInfoLogARB(handle, infoLogLen, &actualLength, infoLog);
	LOG_L(L_ERROR, "%s", infoLog);
	delete[] infoLog;
}




struct Shader {
	std::list<std::string> texts;
	GLhandleARB handle;

	Shader() { handle = 0; }

	void AddFile(const std::string& file)
	{
		CFileHandler fh(file);
		if (!fh.FileExists())
			throw content_error("Can not load shader " + file);

		std::string text;
		text.resize(fh.FileSize());
		fh.Read(&text[0], text.length());

		texts.push_back(text);
	}

	void Build(GLenum shaderType)
	{
		handle = glCreateShaderObjectARB(shaderType);

		std::vector<GLint> lengths(texts.size());
		std::vector<const GLcharARB*> strings(texts.size());
		int index = 0;
		for (std::list<std::string>::iterator i = texts.begin(); i != texts.end(); ++i, index++) {
			lengths[index] = i->length();
			strings[index] = i->c_str();
		}

//		if (shaderType == GL_FRAGMENT_SHADER_ARB)
//			DebugOutput(shaderType);

		glShaderSourceARB(handle, strings.size(), &strings.front(), &lengths.front());
		glCompileShaderARB(handle);

		// Check compile status and show info log if failed
		GLint isCompiled;
		glGetObjectParameterivARB(handle, GL_OBJECT_COMPILE_STATUS_ARB, &isCompiled);
		if (!isCompiled)
		{
			WriteToFile("sm3_failed_shader.glsl");
			ShowInfoLog(handle);

			std::string errMsg = "Failed to build ";
			throw std::runtime_error(errMsg + (shaderType == GL_VERTEX_SHADER_ARB ? "vertex shader" : "fragment shader"));
		}
	}
	void DebugOutput(GLenum shaderType)
	{
		char fn[20];
		if (shaderType == GL_FRAGMENT_SHADER_ARB) {
			static int fpc = 0;
			sprintf(fn, "shader%dfp.txt", fpc++);
		} else {
			static int vpc = 0;
			sprintf(fn, "shader%dvp.txt", vpc++);
		}
		WriteToFile(fn);
	}
	void WriteToFile(const char* fn)
	{
		std::string n = dataDirsAccess.LocateFile(fn, FileQueryFlags::WRITE);

		FILE* f = fopen(n.c_str(), "w");

		if (f) {
			for (std::list<std::string>::iterator i=texts.begin();i!=texts.end();++i)
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
		assert(framebuffer.IsValid());

		width = globalRendering->viewSizeX;
		height = globalRendering->viewSizeY;
		if (GLEW_ARB_texture_rectangle)
			target = GL_TEXTURE_RECTANGLE_ARB;
		else {
			target = GL_TEXTURE_2D;
			width = closest_pot(width);
			height = closest_pot(height);
		}

		name = "_buffer";

		glGenTextures(1, &id);
		glBindTexture(target, id);
		glTexParameteri(target,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(target,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexImage2D(target, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		framebuffer.Bind();
		framebuffer.CreateRenderBuffer(GL_DEPTH_ATTACHMENT_EXT, GL_DEPTH_COMPONENT24, width, height);
		framebuffer.AttachTexture(id, target);
		bool status = framebuffer.CheckStatus("sm3");
		framebuffer.Unbind();
		assert(status);
	}
	~BufferTexture()
	{
		// texture is deleted by ~BaseTexture
	}
	bool IsRect() {	return target == GL_TEXTURE_RECTANGLE_ARB; }

	int width, height;
	uint target;
	FBO framebuffer;
};

struct ShaderBuilder
{
	RenderSetup* renderSetup;
	TextureUsage texUsage;
	BufferTexture* buffer;
	bool ShadowMapping() const { return renderSetup->shaderDef.useShadowMapping; }
	Shader lastFragmentShader, lastVertexShader; // for debugging

	ShaderBuilder(RenderSetup* rs);
	std::string GenTextureRead(int tu, int tc);
	NodeGLSLShader* EndPass(ShaderDef* sd, const std::string &operations, uint passIndex=0);
	void BuildFragmentShader(NodeGLSLShader* ns, uint passIndex, const std::string& operations, ShaderDef* sd);
	void BuildVertexShader(NodeGLSLShader* ns, uint passIndex, ShaderDef* sd);
	bool ProcessStage(std::vector<ShaderDef::Stage>& stages, uint &index, std::string& opstr);
	void Build(ShaderDef* shaderDef);
	void AddPPDefines(ShaderDef* sd, Shader& shader, uint passIndex);

	enum ShadingMethod {
		SM_DiffuseSP, // lit diffuse single pass
		SM_DiffuseBumpmapSP, // full diffuse + bumpmapping in single pass
		SM_DiffuseBumpmapMP, // diffuse pass + bumpmap pass
		SM_Impossible  // massive failure
	};

	ShadingMethod shadingMethod;
	ShadingMethod CalculateShadingMethod(ShaderDef* sd) const;

	struct TexReq {
		TexReq()
			: coords(0)
			, units(0)
		{}

		void GetFromGL();
		bool Fits(TexReq maxrq) const {
			return ((coords <= maxrq.coords) && (units <= maxrq.units));
		}
		TexReq operator + (const TexReq& rq) const {
			TexReq r;
			r.coords = coords + rq.coords;
			r.units = units + rq.units;
			return r;
		}

		GLint coords;
		GLint units;
	};
	// Calculate the texturing requirements for the specified stages
	TexReq CalcStagesTexReq(const std::vector<ShaderDef::Stage>& stages, uint startIndex) const;
};



// Decide how to organise the shading, ie: in how many passes
// depending on maximum number of hardware texture units and
// coordinates
ShaderBuilder::ShadingMethod  ShaderBuilder::CalculateShadingMethod(ShaderDef* sd) const {
	TexReq diffuseRQ = CalcStagesTexReq(sd->stages, 0);
	TexReq bumpmapRQ;
	TexReq special;

	TexReq hwmax;
	hwmax.GetFromGL();


	if (sd->useShadowMapping) {
		// add shadow buffer + shadow texture coord
		special.coords++;
		special.units++;
	}

	LOG("[ShaderBuilder::CalculateShadingMethod]");
	LOG("\t    hwmax.units=%2d,     hwmax.coords=%2d",     hwmax.units,     hwmax.coords);
	LOG("\tdiffuseRQ.units=%2d, diffuseRQ.coords=%2d", diffuseRQ.units, diffuseRQ.coords);
	LOG("\tbumpmapRQ.units=%2d, bumpmapRQ.coords=%2d", bumpmapRQ.units, bumpmapRQ.coords);
	LOG("\t  special.units=%2d,   special.coords=%2d",   special.units,   special.coords);

	if (sd->normalMapStages.empty()) {
		if ((diffuseRQ + special).Fits(hwmax)) {
			LOG("\tno normal-map stages, SM_DiffuseSP");
			return SM_DiffuseSP;
		} else {
			LOG("\tno normal-map stages, SM_Impossible");
			return SM_Impossible;
		}
	} else {
		bumpmapRQ = CalcStagesTexReq(sd->normalMapStages, 0);
		// bumpmapping needs two extra indexable TC's for
		// lightdir + tsEyeDir or for lightdir + wsEyeDir
		special.coords += 2;
	}

	TexReq total = diffuseRQ + bumpmapRQ + special;

	LOG("\t*****************************************");
	LOG("\tbumpmapRQ.units=%2d, bumpmapRQ.coords=%2d", bumpmapRQ.units, bumpmapRQ.coords);
	LOG("\t  special.units=%2d,   special.coords=%2d",   special.units,   special.coords);
	LOG("\t    total.units=%2d,     total.coords=%2d",     total.units,     total.coords);

	// diffuse + bumpmap in one pass?
	if (total.Fits(hwmax)) {
		LOG("\tnormalMapStages.size()=" _STPF_ ", SM_DiffuseBumpmapSP", sd->normalMapStages.size());
		return SM_DiffuseBumpmapSP;
	}


	// for multipass, one extra texture read is required for the diffuse input
	special.units++;

	LOG("\t*****************************************");
	LOG("\t  special.units=%2d,   special.coords=%2d", special.units, special.coords);
	LOG("\t    total.units=%2d,     total.coords=%2d",   total.units,   total.coords);

	// is multipass possible?
	if (diffuseRQ.Fits(hwmax) && (bumpmapRQ + special).Fits(hwmax)) {
		LOG("\tnormalMapStages.size()=" _STPF_ ", SM_DiffuseBumpmapMP", sd->normalMapStages.size());
		return SM_DiffuseBumpmapMP;
	}

	// no options left
	LOG("\tSM_Impossible");
	return SM_Impossible;
}



ShaderBuilder::TexReq  ShaderBuilder::CalcStagesTexReq(const std::vector<ShaderDef::Stage>& stages, uint index) const {
	TextureUsage usage;

	while (index < stages.size()) {
		const ShaderDef::Stage& stage = stages[index];
		BaseTexture* texture = stage.source;
		TextureUsage tmpUsage;

		usage.AddTextureRead(-1, texture);
		usage.AddTextureCoordRead(-1, texture);

		if (stage.operation == ShaderDef::Alpha) {
			// next operation is blend (alpha is autoinserted before blend)
			assert(index < stages.size()-1 && stages[index+1].operation == ShaderDef::Blend);
			const ShaderDef::Stage& blendStage = stages[index+1];

			usage.AddTextureRead(-1, blendStage.source);
			usage.AddTextureCoordRead(-1, blendStage.source);

			index++;
		}
		index++;
	}

	TexReq rq;
		rq.coords = usage.coordUnits.size();
		rq.units = usage.texUnits.size();
	return rq;
}

void ShaderBuilder::TexReq::GetFromGL() {
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &units);
	glGetIntegerv(GL_MAX_TEXTURE_COORDS_ARB, &coords);
}

ShaderBuilder::ShaderBuilder(RenderSetup* rs): renderSetup(rs) {
	shadingMethod = SM_DiffuseSP;
	buffer = 0;
}

std::string ShaderBuilder::GenTextureRead(int tu, int tc) {
	char tcstr[6];
	sprintf(tcstr,"%d", tc);
	return "texture2D(" + texUsage.texUnits[tu]->name + ", gl_TexCoord[" + tcstr + "].st)";
}



NodeGLSLShader* ShaderBuilder::EndPass(ShaderDef* sd, const std::string &operations, uint passIndex)
{
	NodeGLSLShader* nodeShader = new NodeGLSLShader;

	if (shadingMethod == SM_DiffuseSP)
		nodeShader->vertBufReq = VRT_Normal;
	else
		nodeShader->vertBufReq = VRT_TangentSpaceMatrix;
	//nodeShader->vertBufReq = VRT_Normal | VRT_TangentSpaceMatrix;

	nodeShader->texCoordGen = texUsage.coordUnits;
	nodeShader->texUnits = texUsage.texUnits;

	BuildVertexShader(nodeShader, passIndex, sd);
	BuildFragmentShader(nodeShader, passIndex, operations, sd);

	nodeShader->program = glCreateProgramObjectARB();
	glAttachObjectARB(nodeShader->program, nodeShader->vertexShader);
	glAttachObjectARB(nodeShader->program, nodeShader->fragmentShader);

	glLinkProgramARB(nodeShader->program);
	GLint isLinked;
	glGetObjectParameterivARB(nodeShader->program, GL_OBJECT_LINK_STATUS_ARB, &isLinked);

	if (!isLinked) {
		LOG_L(L_ERROR, "Failed to link shaders. Showing info log:");
		lastFragmentShader.WriteToFile("sm3_fragmentshader.txt");
		lastVertexShader.WriteToFile("sm3_vertexshader.txt");
		ShowInfoLog(nodeShader->program);
		throw std::runtime_error("Failed to link shaders");
	}

	glUseProgramObjectARB(nodeShader->program);

	// set texture image units to texture samplers in the shader
	for (size_t a = 0; a < nodeShader->texUnits.size(); a++) {
		BaseTexture* tex = nodeShader->texUnits[a];
		GLint location = glGetUniformLocationARB(nodeShader->program, tex->name.c_str());
		glUniform1iARB(location, (int)a);
	}

	// have bumpmapping?
	if (shadingMethod != SM_DiffuseSP &&
		!(shadingMethod == SM_DiffuseBumpmapMP && passIndex == 0))
	{
		nodeShader->tsmAttrib = glGetAttribLocationARB(nodeShader->program, "TangentSpaceMatrix");
		nodeShader->wsLightDirLocation = glGetUniformLocationARB(nodeShader->program, "wsLightDir");
		nodeShader->wsEyePosLocation = glGetUniformLocationARB(nodeShader->program, "wsEyePos");
	}

	if (ShadowMapping()) {
		nodeShader->shadowMapLocation = glGetUniformLocationARB(nodeShader->program, "shadowMap");
		nodeShader->shadowParamsLocation = glGetUniformLocationARB(nodeShader->program, "shadowParams");
		nodeShader->shadowMatrixLocation = glGetUniformLocationARB(nodeShader->program, "shadowMatrix");
	}

	if (passIndex == 1) {
		// set up uniform to read bumpmap
		GLint invScreenDim = glGetUniformLocationARB(nodeShader->program, "invScreenDim");
		glUniform2fARB(invScreenDim, 1.0f / globalRendering->viewSizeX, 1.0f / globalRendering->viewSizeY);
	}

	glUseProgramObjectARB(0);

	renderSetup->passes.push_back(RenderPass());
	RenderPass& rp = renderSetup->passes.back();
		rp.shaderSetup = nodeShader;
		rp.operation = Pass_Replace;
		rp.depthWrite = true;
	nodeShader->debugstr = operations;
	NodeGLSLShader* ns = nodeShader;
	nodeShader = 0;
	texUsage = TextureUsage();
	return ns;
}


void ShaderBuilder::AddPPDefines(ShaderDef* sd, Shader& shader, uint passIndex)
{
	bool bumpmapping = (shadingMethod != SM_DiffuseSP &&
		!(shadingMethod == SM_DiffuseBumpmapMP && passIndex == 0));

	if (bumpmapping) {
		shader.texts.push_back("#define UseBumpMapping\n");

		if (passIndex == 1) {
			shader.texts.push_back("#define DiffuseFromBuffer\n");

			if (GLEW_ARB_texture_rectangle)
				shader.texts.push_back("#define UseTextureRECT\n");
		}
	}

	if (ShadowMapping())
		shader.texts.push_back("#define UseShadowMapping\n");

	shader.AddFile("shaders/GLSL/terrainCommon.glsl");
	char specularExponentStr[20];
	SNPRINTF(specularExponentStr, 20, "%5.3f", sd->specularExponent);
	shader.texts.push_back(std::string("const float specularExponent = ") + specularExponentStr + ";\n");
}


void ShaderBuilder::BuildFragmentShader(NodeGLSLShader* ns, uint passIndex, const std::string& operations, ShaderDef* sd)
{
	lastFragmentShader = Shader();
	Shader& fragmentShader = lastFragmentShader;

	// insert texture samplers
	std::string textureSamplers;
	for (size_t a = 0; a < ns->texUnits.size(); a++) {
		BaseTexture* tex = ns->texUnits[a];
		if (tex->IsRect())
			textureSamplers += "uniform sampler2DRect " + tex->name + ";\n";
		else
			textureSamplers += "uniform sampler2D " + tex->name + ";\n";
	}

	AddPPDefines(sd, fragmentShader, passIndex);
	fragmentShader.texts.push_back(textureSamplers);

	fragmentShader.AddFile("shaders/GLSL/terrainFragmentShader.glsl");

	std::string gentxt = "vec4 CalculateColor()  { vec4 color; float curalpha; \n" + operations;

	switch (shadingMethod) {
		case SM_DiffuseSP:
			gentxt += "return Light(color); }\n";
			break;
		case SM_DiffuseBumpmapSP:
			gentxt += "return Light(diffuse, color);}\n";
			break;
		case SM_DiffuseBumpmapMP:
			if (passIndex == 0) {
				gentxt += "return color; }\n";
			} else { // passIndex=1
				// ReadDiffuseColor() is #defined conditionally in
				// terrainFragmentShader.glsl if bumpmapping enabled
				gentxt += "return Light(ReadDiffuseColor(), color);}\n";
			}
			break;
		case SM_Impossible:
			break;
	}

	fragmentShader.texts.push_back(gentxt);
	fragmentShader.Build(GL_FRAGMENT_SHADER_ARB);
	ns->fragmentShader = fragmentShader.handle;

	LOG("Fragment shader built successfully.");
}

void ShaderBuilder::BuildVertexShader(NodeGLSLShader* ns, uint passIndex, ShaderDef* sd)
{
	lastVertexShader = Shader();
	Shader& vertexShader = lastVertexShader;

	AddPPDefines(sd, vertexShader, passIndex);

	// generate texture coords
	std::string tcgen = "void CalculateTexCoords() {\n";
	static const size_t buf_sizeMax = 160;
	for (size_t a = 0; a < ns->texCoordGen.size(); a++) {
		char buf[buf_sizeMax];
		SNPRINTF(buf, buf_sizeMax, "gl_TexCoord[" _STPF_ "].st = vec2(dot(gl_Vertex, gl_ObjectPlaneS[" _STPF_ "]), dot(gl_Vertex,gl_ObjectPlaneT[" _STPF_ "]));\n", a, a, a);
		tcgen += buf;
	}
	tcgen += "}\n";
	vertexShader.texts.push_back(tcgen);

	vertexShader.AddFile("shaders/GLSL/terrainVertexShader.glsl");
	vertexShader.Build(GL_VERTEX_SHADER_ARB);
	LOG("Vertex shader built successfully.");

	ns->vertexShader = vertexShader.handle;
}



bool ShaderBuilder::ProcessStage(std::vector<ShaderDef::Stage>& stages, uint &index, std::string& opstr)
{
	ShaderDef::Stage& stage = stages[index];
	BaseTexture* texture = stage.source;

	TexReq hwmax;
	hwmax.GetFromGL();

	TextureUsage tmpUsage = texUsage;
	int tu = tmpUsage.AddTextureRead(hwmax.units, texture);
	int tc = tmpUsage.AddTextureCoordRead(hwmax.coords, texture);

	assert(tu >= 0);
	assert(tc >= 0);

	if (index == 0) {  // replace
		texUsage = tmpUsage;
		opstr = "color = " + GenTextureRead(tu,tc) + ";\n";
	}
	else if(stage.operation == ShaderDef::Alpha) {
		// next operation is blend (alpha is autoinserted before blend)
		assert(index < (stages.size() - 1));
		assert(stages[index + 1].operation == ShaderDef::Blend);
		ShaderDef::Stage& blendStage = stages[index+1];

		int blendTU = tmpUsage.AddTextureRead(hwmax.units, blendStage.source);
		int blendTC = tmpUsage.AddTextureCoordRead(hwmax.coords, blendStage.source);

		assert(blendTU >= 0);
		assert(blendTC >= 0);

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


void ShaderBuilder::Build(ShaderDef* shaderDef) {
	texUsage = TextureUsage();
	shadingMethod = CalculateShadingMethod(shaderDef);

	switch (shadingMethod) {
		case SM_DiffuseSP: {
			std::string opstr;
			for (uint stage = 0; stage < shaderDef->stages.size(); )
				ProcessStage(shaderDef->stages, stage, opstr);
			EndPass(shaderDef, opstr);
			break;
		}

		case SM_DiffuseBumpmapSP: {
			std::string diffusecode, bumpmapcode;

			for (uint stage = 0; stage < shaderDef->stages.size(); )
				ProcessStage(shaderDef->stages, stage, diffusecode);

			for (uint stage = 0; stage < shaderDef->normalMapStages.size(); )
				ProcessStage(shaderDef->normalMapStages, stage, bumpmapcode);

			EndPass(shaderDef, diffusecode + "vec4 diffuse = color;\n" + bumpmapcode);
			break;
		}

		case SM_DiffuseBumpmapMP: {
			std::string diffusecode, bumpmapcode;

			for (uint stage = 0; stage < shaderDef->stages.size(); )
				ProcessStage(shaderDef->stages, stage, diffusecode);


			NodeGLSLShader* diffusePass = EndPass(shaderDef, diffusecode, 0);

			// multipass: let the diffuse pass render to the buffer
			// at this point nodeShader=0 and texUsage is empty
			if (!buffer) {
				buffer = new BufferTexture;
			}
			diffusePass->renderBuffer = buffer;
			// add texture read operation to second pass
			texUsage.AddTextureRead(-1, buffer);


			for (uint stage = 0; stage < shaderDef->normalMapStages.size(); )
				ProcessStage(shaderDef->normalMapStages, stage, bumpmapcode);

			EndPass(shaderDef, bumpmapcode, 1);

			break;
		}

		case SM_Impossible:
			throw content_error("Map has too many layers for bumpmapping on this hardware");
	}
}


NodeGLSLShader::NodeGLSLShader()
{
	vertexShader = program = fragmentShader = 0;

	vertBufReq = 0;
	tsmAttrib = -1;
	wsLightDirLocation = wsEyePosLocation = -1;

	shadowMapLocation = -1;
	shadowMatrixLocation = -1;
	shadowParamsLocation = -1;

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



void NodeGLSLShader::BindTSM(Vector3* buf, uint vertexSize)
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

void NodeGLSLShader::UnbindTSM()
{
	if (tsmAttrib >= 0) {
		for (int a=0;a<3;a++)
			glDisableVertexAttribArrayARB(tsmAttrib+a);
	}
}



void NodeGLSLShader::Setup(NodeSetupParams& params) {
	/*
	if (renderBuffer) { // use a offscreen rendering buffer
		renderBuffer->framebuffer.Bind();
		glViewport(0, 0, renderBuffer->width, renderBuffer->height);
	}
	*/

	glUseProgramObjectARB(program);
	for (size_t a = 0; a < texUnits.size(); a++) {
		glActiveTextureARB(GL_TEXTURE0_ARB + a);

		GLenum target;
		if (texUnits[a]->IsRect())
			target = GL_TEXTURE_RECTANGLE_ARB;
		else
			target = GL_TEXTURE_2D;

		if (texUnits[a]->id)
			glBindTexture(target, texUnits[a]->id);
		glEnable(target);
	}
	for (size_t a = 0; a < texCoordGen.size(); a++) {
		glActiveTextureARB(GL_TEXTURE0_ARB + a);
		texCoordGen[a]->SetupTexGen();
	}
	glActiveTextureARB(GL_TEXTURE0_ARB);

	if (wsLightDirLocation >= 0 && params.wsLightDir)
		glUniform3fARB(wsLightDirLocation, params.wsLightDir->x, params.wsLightDir->y, params.wsLightDir->z);
	if (wsEyePosLocation >= 0 && params.wsEyePos)
		glUniform3fARB(wsEyePosLocation, params.wsEyePos->x, params.wsEyePos->y, params.wsEyePos->z);

	if (params.shadowMapParams) {
		if (shadowMapLocation >= 0) {
			glUniform1i(shadowMapLocation, texUnits.size());
			shadowHandler->SetupShadowTexSampler(GL_TEXTURE0 + texUnits.size() /*, params.shadowMapParams->shadowMap*/);
		}

		ShadowMapParams& smp = *params.shadowMapParams;
		if (shadowMatrixLocation >= 0) glUniformMatrix4fvARB(shadowMatrixLocation, 1, GL_TRUE, smp.shadowMatrix);
		if (shadowParamsLocation >= 0) glUniform4fARB(shadowParamsLocation, smp.f_a, smp.f_b, smp.mid[0], smp.mid[1]);
	}
}

void NodeGLSLShader::Cleanup() {
	for (size_t a = 0; a < texUnits.size(); a++) {
		glActiveTextureARB(GL_TEXTURE0_ARB + a);
		glBindTexture(texUnits[a]->IsRect()? GL_TEXTURE_RECTANGLE_ARB : GL_TEXTURE_2D, 0);
		glDisable(texUnits[a]->IsRect()? GL_TEXTURE_RECTANGLE_ARB : GL_TEXTURE_2D);
	}

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glUseProgramObjectARB(0);
}




std::string NodeGLSLShader::GetDebugDesc()
{
	return debugstr;
}

uint NodeGLSLShader::GetVertexDataRequirements()
{
	return vertBufReq;
}
void NodeGLSLShader::GetTextureUnits(BaseTexture* tex, int &imageUnit, int& coordUnit)
{
}

GLSLShaderHandler::GLSLShaderHandler()
{
	curShader = 0;
	scShader = 0;
	buffer = 0;
}

GLSLShaderHandler::~GLSLShaderHandler()
{
	delete buffer;
	delete scShader;
}



void GLSLShaderHandler::EndTexturing() {
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);

	if (curShader) {
		curShader->Cleanup();
		curShader = NULL;
	}
}

void GLSLShaderHandler::BeginTexturing() {
}



void GLSLShaderHandler::BeginPass(const std::vector<Blendmap*>& blendmaps, const std::vector<TiledTexture*>& textures, int pass)
{
	if (buffer) {
		if ((buffer->width != globalRendering->viewSizeX) || (buffer->height != globalRendering->viewSizeY)) {
			delete buffer;
			buffer = NULL; // to prevent a dead-pointer in case of an out-of-memory exception on the next line
			buffer = new BufferTexture;
		}
	}
	if (buffer) {
		if (pass == 0) {
			buffer->framebuffer.Bind();
			glViewport(0, 0, buffer->width, buffer->height);
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		}
		else if (pass == 1) {
			buffer->framebuffer.Unbind();
			glViewport(globalRendering->viewPosX, globalRendering->viewPosY, globalRendering->viewSizeX, globalRendering->viewSizeY);
		}
	}
}

bool GLSLShaderHandler::SetupShader(IShaderSetup* ps, NodeSetupParams& params)
{
	if (curShader) {
		curShader->Cleanup();
		curShader = NULL;
	}

	GLSLBaseShader* bs = static_cast<GLSLBaseShader*>(ps);
	bs->Setup(params);
	curShader = bs;
	return true;
}


void GLSLShaderHandler::BeginBuild()
{
	if (curShader) {
		curShader->Cleanup();
		curShader = 0;
	}

	delete buffer;
	delete scShader;

	buffer = NULL;
	scShader = NULL;

	renderSetups.clear();
}


void GLSLShaderHandler::EndBuild()
{
	bool multipass = false;
	for (size_t a=0;a<renderSetups.size();a++)
		if (renderSetups[a]->passes.size()>1) {
			multipass = true;
			break;
		}

	if (!multipass)
		return;

	scShader = new SimpleCopyShader(buffer);

	// make sure all rendersetups have 2 passes
	for (size_t a = 0; a < renderSetups.size(); a++) {
		if (renderSetups[a]->passes.size() == 2)
			continue;

		// add a simple pass to add
		renderSetups[a]->passes.push_back(RenderPass());
		RenderPass& pass = renderSetups[a]->passes.back();
		pass.depthWrite = true;
		pass.operation = Pass_Replace;
		pass.shaderSetup = new SimpleCopyNodeShader(scShader);
	}
}

void GLSLShaderHandler::BuildNodeSetup(ShaderDef* shaderDef, RenderSetup* renderSetup)
{
	ShaderBuilder shaderBuilder(renderSetup);

	shaderBuilder.buffer = buffer;
	shaderBuilder.Build(shaderDef);

	// Build() might have changed buffer
	buffer = shaderBuilder.buffer;

	renderSetups.push_back(renderSetup);
}



int GLSLShaderHandler::MaxTextureUnits() {
	GLint n;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &n);
	return n;
}

int GLSLShaderHandler::MaxTextureCoords() {
	GLint n;
	glGetIntegerv(GL_MAX_TEXTURE_COORDS_ARB, &n);
	return n;
}



SimpleCopyShader::SimpleCopyShader(BufferTexture* buf)
{
	Shader fs, vs;

	if (buf->IsRect())
		fs.texts.push_back("#define UseTextureRECT");
	fs.AddFile("shaders/GLSL/terrainSimpleCopyFS.glsl");
	fs.Build(GL_FRAGMENT_SHADER_ARB);

	vs.AddFile("shaders/GLSL/terrainSimpleCopyVS.glsl");
	vs.Build(GL_VERTEX_SHADER_ARB);

	vertexShader = vs.handle;
	fragmentShader = fs.handle;

	program = glCreateProgramObjectARB();
	glAttachObjectARB(program, vertexShader);
	glAttachObjectARB(program, fragmentShader);

	glLinkProgramARB(program);
	GLint isLinked;
	glGetObjectParameterivARB(program, GL_OBJECT_LINK_STATUS_ARB, &isLinked);
	if (!isLinked)
	{
		LOG_L(L_ERROR, "Failed to link shaders. Showing info log:");
		ShowInfoLog(program);
		throw std::runtime_error("Failed to link shaders");
	}

	GLint srcTex = glGetUniformLocationARB(program, "sourceTexture");
	glUseProgramObjectARB(program);
	glUniform1iARB(srcTex, 0);
	if (!buf->IsRect()) {
		GLint invScreenDim = glGetUniformLocationARB(program, "invScreenDim");
		glUniform2fARB(invScreenDim, 1.0f/globalRendering->viewSizeX, 1.0f/globalRendering->viewSizeY);
	}
	glUseProgramObjectARB(0);

	source = buf;
}

SimpleCopyShader::~SimpleCopyShader()
{
	glDetachObjectARB(program, vertexShader);
	glDetachObjectARB(program, fragmentShader);
	glDeleteObjectARB(program);
	glDeleteObjectARB(fragmentShader);
	glDeleteObjectARB(vertexShader);
}

void SimpleCopyShader::Setup()
{
	GLenum target;
	if (source->IsRect())
		target = GL_TEXTURE_RECTANGLE_ARB;
	else
		target = GL_TEXTURE_2D;

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(target, source->id);
	glEnable(target);

	glUseProgramObjectARB(program);
}

void SimpleCopyShader::Cleanup()
{
	glActiveTextureARB(GL_TEXTURE0_ARB);
	if (source->IsRect())
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
	else
		glDisable(GL_TEXTURE_2D);
	glUseProgramObjectARB(0);
}

}
