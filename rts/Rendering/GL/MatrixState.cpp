#include "MatrixState.hpp"
#include "Rendering/GL/myGL.h"
#include "System/MainDefines.h"

static GL::MatrixState matrixStates[2];
static _threadlocal GL::MatrixState* matrixState = nullptr;


void GL::SetMatrixStatePointer(bool mainThread) {
	matrixState = &matrixStates[mainThread];
}


// TODO: remove
void GL::MatrixState::Load(const CMatrix44f& m) { glLoadMatrixf(m); }


void GL::MatrixMode(unsigned int glMode) {
	switch (glMode) {
		case GL_MODELVIEW : { matrixState->SetMode(0); } break;
		case GL_PROJECTION: { matrixState->SetMode(1); } break;
		case GL_TEXTURE   : { matrixState->SetMode(2); } break;
		default           : {                          } break;
	}

	// TODO: remove
	glMatrixMode(glMode);
}


const CMatrix44f& GL::GetMatrix() { return (matrixState->Top()); }
const CMatrix44f& GL::GetMatrix(unsigned int glMode) {
	switch (glMode) {
		case GL_MODELVIEW : { return (matrixState->Top(0)); } break;
		case GL_PROJECTION: { return (matrixState->Top(1)); } break;
		case GL_TEXTURE   : { return (matrixState->Top(2)); } break;
		default           : {                               } break;
	}

	// fallback
	return (matrixState->Top());
}

void GL::PushMatrix() { matrixState->Push(matrixState->Top()); }
void GL::PopMatrix() { matrixState->Pop(); }

void GL::MultMatrix(const CMatrix44f& m) { matrixState->Mult(m); }
void GL::LoadMatrix(const CMatrix44f& m) { matrixState->Load(m); }
void GL::LoadIdentity() { LoadMatrix(CMatrix44f::Identity()); }

void GL::Translate(const float3& v) { matrixState->Translate(v); }
void GL::Translate(float x, float y, float z) { matrixState->Translate({x, y, z}); }
void GL::Scale(const float3& s) { matrixState->Scale(s); }
void GL::Scale(float x, float y, float z) { matrixState->Scale({x, y, z}); }
void GL::RotateX(float a) { matrixState->RotateX(a); }
void GL::RotateY(float a) { matrixState->RotateY(a); }
void GL::RotateZ(float a) { matrixState->RotateZ(a); }

