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
#include "TerrainTexEnvCombine.h"
#include <assert.h>

namespace terrain {

using namespace std;

uint NodeTexEnvSetup::GetVertexDataRequirements ()
{
	return VRT_Normal;
}

string NodeTexEnvSetup::GetDebugDesc ()
{
	string str;

	const char *opstr[] = {
		"Replace",
		"Mul",
		"Add",
		"AddSigned",
		"Sub",
		"Previous",
		"Interp",   // interpolate:  color = prev.RGB * prev.Alpha + current.RGB * ( 1-prev.Alpha )
		"InsertAlpha", // same as previous, but does not allow color invert
		"Dot3"   // dot product, requires GL_ARB_texture_env_dot3
	};

	const char *srcstr[] = {
		"None", "TextureSrc", "ColorSrc"
	};

	for (size_t a=0;a<stages.size();a++)
	{
		str += "{" + string(opstr[(int)stages[a].operation]) + ", " + srcstr[(int)stages[a].source];
		if (stages[a].source == TexEnvStage::TextureSrc) str += " = " + string(stages[a].srcTexture->name);
		str += "} ";
	};
	return str;
}

void NodeTexEnvSetup::GetTextureUnits(BaseTexture *tex, int& imageUnit,int& coordUnit)
{
	for (size_t a=0;a<stages.size();a++)
		if (stages[a].source == TexEnvStage::TextureSrc && stages[a].srcTexture == tex) {
			imageUnit = coordUnit = (int)a;
			break;
		}
}

//-----------------------------------------------------------------------
// texture env state builder
//-----------------------------------------------------------------------

TexEnvStage::TexEnvStage ()
{
	operation = Replace;
	srcTexture = 0;
	source=None;
}

TexEnvSetupHandler::TexEnvSetupHandler()
{
	hasDot3 = !!GLEW_ARB_texture_env_dot3;

	// Create white texture
	glGenTextures (1, &whiteTexture);
	glBindTexture (GL_TEXTURE_2D, whiteTexture);
	uint pixel=0xffffffff;
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, 1,1,0,GL_RGBA, GL_UNSIGNED_BYTE, &pixel);

	curSetup=0;
	lastShader=0;
	maxtu = 0;
}

int TexEnvSetupHandler::MaxTextureUnits ()
{
	GLint m;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &m);
	return m;
}
int TexEnvSetupHandler::MaxTextureCoords ()
{
	GLint m;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &m);
	return m;
}

void TexEnvSetupHandler::BuildNodeSetup (ShaderDef* shaderDef, RenderSetup* renderSetup)
{
	int maxTextureUnits = MaxTextureUnits();
	unsigned int c = 0;

	NodeTexEnvSetup *setup = curSetup = new NodeTexEnvSetup;

	renderSetup->passes.push_back(RenderPass());
	renderSetup->passes.back().shaderSetup = setup;
	renderSetup->passes.back().depthWrite = true;

	// Handle the first stages with texture units
	while (c < maxTextureUnits && c < shaderDef->stages.size())
	{
		ShaderDef::Stage& st = shaderDef->stages[c];

		if(st.operation == ShaderDef::Alpha && c==maxTextureUnits-1) // blending needs 2 stages
			break;

		setup->stages.push_back (TexEnvStage());
		TexEnvStage& ts = setup->stages.back();

		ts.source = TexEnvStage::TextureSrc;
		ts.srcTexture = st.source;

		switch (st.operation) {
			case ShaderDef::Blend:
				ts.operation = TexEnvStage::Interp;
				break;
			case ShaderDef::Alpha:
				ts.operation = TexEnvStage::InsertAlpha;
				break;
			case ShaderDef::Add:
				ts.operation = TexEnvStage::Add;
				break;
			case ShaderDef::Mul:
				ts.operation = TexEnvStage::Mul;
				break;
		}

		if (!c)
			ts.operation = TexEnvStage::Replace;

		c++;
	}

	// Switch to multipass when the stages couldn't all be handled with multitexturing
	while (c < shaderDef->stages.size())
	{
		ShaderDef::Stage& st = shaderDef->stages[c];

		renderSetup->passes.push_back(RenderPass());
		RenderPass& rp = renderSetup->passes.back();
		rp.shaderSetup = setup = new NodeTexEnvSetup;

		if (st.operation == ShaderDef::Alpha) {
			assert (shaderDef->stages[c+1].operation == ShaderDef::Blend);
			setup->stages.push_back (TexEnvStage());
			TexEnvStage& interp = setup->stages.back();
			interp.operation = TexEnvStage::Replace;
			interp.source = TexEnvStage::TextureSrc;
			interp.srcTexture = shaderDef->stages[c+1].source;

			setup->stages.push_back (TexEnvStage());
			TexEnvStage& a = setup->stages.back();
			a.operation = TexEnvStage::InsertAlpha;
			a.source = TexEnvStage::TextureSrc;
			a.srcTexture = st.source;
			c++; // skip the blend stage because we used it here already

			rp.operation = Pass_Interpolate;
		}
		else if (st.operation == ShaderDef::Add || st.operation == ShaderDef::Mul)
		{
			setup->stages.push_back (TexEnvStage());
			TexEnvStage& ts = setup->stages.back();
			ts.operation = TexEnvStage::Replace;
			ts.source = TexEnvStage::TextureSrc;
			ts.srcTexture = st.source;

			if (st.operation == ShaderDef::Add)
				rp.operation = Pass_Add;
			else
				rp.operation = Pass_Mul;
		}

		c++;
	}

	if (!shaderDef->hasLighting)
	{
		// multiply with color: this is either done through an extra texture stage, or with another pass
		if (c < maxTextureUnits)
		{
			// extra stage
			setup->stages.push_back (TexEnvStage());
			TexEnvStage& ts = setup->stages.back();

			ts.source = TexEnvStage::ColorSrc;
			ts.operation = TexEnvStage::Mul;
		}
		else
		{
			// extra pass
			renderSetup->passes.push_back(RenderPass());
			renderSetup->passes.back().operation = Pass_Mul;
			renderSetup->passes.back().shaderSetup = setup = new NodeTexEnvSetup;
			setup->stages.push_back (TexEnvStage());
			setup->stages.back().source = TexEnvStage::ColorSrc;
			setup->stages.back().operation = TexEnvStage::Replace;
		}
	}
}

void TexEnvSetupHandler::BeginPass (const vector<Blendmap*>& blendMaps, const vector<TiledTexture*>& textures, int pass)
{}

bool TexEnvSetupHandler::SetupShader (IShaderSetup *shadercfg, NodeSetupParams& parms)
{
	NodeTexEnvSetup* ns = (NodeTexEnvSetup *)shadercfg;

	int tu = 0;// texture unit
	for (size_t cur = 0; cur < ns->stages.size(); cur ++)
	{
		TexEnvStage& st = ns->stages[cur];

		glActiveTextureARB(GL_TEXTURE0_ARB + tu);
		glEnable (GL_TEXTURE_2D);

		glTexEnvf (GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1.0f);
		glTexEnvf (GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1.0f);
		glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
		glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);

		glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
		glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA);

		// default alpha operation
		glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_CONSTANT_ARB);
		float envcolor[]={1.0f,1.0f,1.0f,1.0f};
		glTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, envcolor);

		// Set source
		glDisable(GL_LIGHTING);
		BaseTexture *texture = 0;
		switch (st.source) {
			case TexEnvStage::ColorSrc:
				glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
				glEnable (GL_LIGHTING);
				break;
			case TexEnvStage::TextureSrc:
				texture = st.srcTexture;
				glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
				break;
			case TexEnvStage::None:
				break;
		}
		if (texture) {
			float v[4];
			texture->CalcTexGenVector (v);
			SetTexCoordGen (v);
			if (texture->id) glBindTexture (GL_TEXTURE_2D, texture->id);
		} else
			glBindTexture (GL_TEXTURE_2D, whiteTexture);

		// set operation
		switch (st.operation) {
			case TexEnvStage::Mul:
				glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
				break;
			case TexEnvStage::Add:
				glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD);
				break;
			case TexEnvStage::Sub:
				glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_SUBTRACT_ARB);
				break;
			case TexEnvStage::Replace:
				glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
				break;
			case TexEnvStage::InsertAlpha:
				glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
				glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
				glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
				break;
			case TexEnvStage::Interp: // color=tex * previous.alpha + tex * previous.alpha
				// Use previous alpha
				glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_PREVIOUS_ARB);
				glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE_ARB);
				break;
			case TexEnvStage::Dot3:
				glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_DOT3_RGB_ARB);
				break;
			case TexEnvStage::AlphaToRGB:
				glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
				glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_ALPHA);
				break;
		}
		tu++;
	}
	ns->usedTexUnits = tu;
	if (maxtu<tu) maxtu = tu;

	// Disable texture units of the last node
	if (lastShader) {
		for (int u = ns->usedTexUnits; u < lastShader->usedTexUnits; u++) {
			glActiveTextureARB (GL_TEXTURE0_ARB + u);
			glDisable (GL_TEXTURE_2D);
		}
	}
	lastShader = ns;

	return true;
}

void TexEnvSetupHandler::BeginTexturing()
{
	lastShader = 0;
	maxtu = 0;
}

void TexEnvSetupHandler::EndTexturing ()
{
	for (int a=0;a<maxtu;a++) {
		glActiveTextureARB(GL_TEXTURE0_ARB+a);
		glDisable (GL_TEXTURE_2D);

		glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);

		glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);

		glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
		glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PREVIOUS_ARB);

		glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

		glDisable (GL_TEXTURE_GEN_S);
		glDisable (GL_TEXTURE_GEN_T);
	}
	maxtu=0;
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisable (GL_LIGHTING);
}

void TexEnvSetupHandler::SetTexCoordGen (float *tgv)
{
	glTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

	Plane s(tgv[0],0,0,tgv[2]);
	glTexGenfv (GL_S, GL_OBJECT_PLANE, (float*)&s);
	Plane t(0,0,tgv[1],tgv[3]);
	glTexGenfv (GL_T, GL_OBJECT_PLANE, (float*)&t);

	glEnable (GL_TEXTURE_GEN_S);
	glEnable (GL_TEXTURE_GEN_T);
}

};
