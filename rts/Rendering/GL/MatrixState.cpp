#include "MatrixState.hpp"
#include "Rendering/GL/myGL.h"
#include "System/MainDefines.h"
#include "System/MathConstants.h"
#include "System/myMath.h"

static GL::MatrixState matrixStates[2];
static _threadlocal GL::MatrixState* matrixState = nullptr;


void GL::SetMatrixStatePointer(bool mainThread) {
	matrixState = &matrixStates[mainThread];
}




const CMatrix44f& GL::MatrixState::Top(unsigned int mode) const {
	#ifdef CHECK_MATRIX_STACK_UNDERFLOW
	if (stacks[mode].empty()) {
		assert(false);
		return dummy;
	}
	#endif
	return (stacks[mode].back());
}

CMatrix44f& GL::MatrixState::Top(unsigned int mode) {
	#ifdef CHECK_MATRIX_STACK_UNDERFLOW
	if (stacks[mode].empty()) {
		assert(false);
		return dummy;
	}
	#endif
	return (stacks[mode].back());
}

void GL::MatrixState::Push(const CMatrix44f& m) { stack->push_back(m); }
void GL::MatrixState::Push() { Push(Top()); }
void GL::MatrixState::Pop() {
	#ifdef CHECK_MATRIX_STACK_UNDERFLOW
	if (stack->empty()) {
		assert(false);
		return;
	}
	#endif
	stack->pop_back();
}


void GL::MatrixState::Mult(const CMatrix44f& m) { Top() = Top() * m; }
void GL::MatrixState::Load(const CMatrix44f& m) { Top() = m; }

void GL::MatrixState::Translate(const float3& v) { Load((Top()).Translate(v)); }
void GL::MatrixState::Translate(float x, float y, float z) { Translate({x, y, z}); }
void GL::MatrixState::Scale(const float3& s) { Load((Top()).Scale(s)); }
void GL::MatrixState::Scale(float x, float y, float z) { Scale({x, y, z}); }
void GL::MatrixState::Rotate(float a, float x, float y, float z) { Load((Top()).Rotate(-a * math::DEG_TO_RAD, {x, y, z})); } // FIXME: broken
void GL::MatrixState::RotateX(float a) { Load((Top()).RotateX(-a * math::DEG_TO_RAD)); }
void GL::MatrixState::RotateY(float a) { Load((Top()).RotateY(-a * math::DEG_TO_RAD)); }
void GL::MatrixState::RotateZ(float a) { Load((Top()).RotateZ(-a * math::DEG_TO_RAD)); }




#if 0
void GL::MatrixMode(unsigned int glMode) {
	switch (glMode) {
		case GL_MODELVIEW : { matrixState->SetMode(0); } break;
		case GL_PROJECTION: { matrixState->SetMode(1); } break;
		case GL_TEXTURE   : { matrixState->SetMode(2); } break;
		default           : {           assert(false); } break;
	}
}
#else
void GL::MatrixMode(unsigned int glMode) { glMatrixMode(glMode); }
#endif

const CMatrix44f& GL::GetMatrix() { return (matrixState->Top()); }
const CMatrix44f& GL::GetMatrix(unsigned int glMode) {
	switch (glMode) {
		case GL_MODELVIEW : { return (matrixState->Top(0)); } break;
		case GL_PROJECTION: { return (matrixState->Top(1)); } break;
		case GL_TEXTURE   : { return (matrixState->Top(2)); } break;
		default           : {                assert(false); } break;
	}

	// fallback
	return (matrixState->Top());
}


/*
static bool InDisplayList() {
	GLboolean inListCompile = GL_FALSE;
	glGetBooleanv(GL_LIST_INDEX, &inListCompile);
	return inListCompile;
}
static void CompareMatrices(const CMatrix44f& svm, const CMatrix44f& spm) {
	// calls in display lists will modify the local stack(s)
	// lists need to be completely removed before switching!
	assert(!InDisplayList());

	CMatrix44f cvm;
	CMatrix44f cpm;

	glGetFloatv(GL_MODELVIEW_MATRIX, &cvm.m[0]);
	glGetFloatv(GL_PROJECTION_MATRIX, &cpm.m[0]);

	for (int i = 0; i < 16; i++) {
		assert(epscmp(svm[i], cvm[i], 0.01f));
		assert(epscmp(spm[i], cpm[i], 0.01f));
	}
}
*/

#if 0
void GL::PushMatrix() { matrixState->Push(matrixState->Top()); }
void GL::PopMatrix() { matrixState->Pop(); }

void GL::MultMatrix(const CMatrix44f& m) { matrixState->Mult(m); }
void GL::LoadMatrix(const CMatrix44f& m) { matrixState->Load(m); }
void GL::LoadIdentity() { LoadMatrix(CMatrix44f::Identity()); }

void GL::Translate(const float3& v) { matrixState->Translate(v); }
void GL::Translate(float x, float y, float z) { matrixState->Translate({x, y, z}); }
void GL::Scale(const float3& s) { matrixState->Scale(s); }
void GL::Scale(float x, float y, float z) { matrixState->Scale({x, y, z}); }
void GL::Rotate(float a, float x, float y, float z) { matrixState->Rotate(a, x, y, z); }
void GL::RotateX(float a) { matrixState->RotateX(a); }
void GL::RotateY(float a) { matrixState->RotateY(a); }
void GL::RotateZ(float a) { matrixState->RotateZ(a); }

#elif 0
// sanity-checked versions
void GL::PushMatrix() { matrixState->Push(matrixState->Top());  glPushMatrix(); CompareMatrices(matrixState->Top(0), matrixState->Top(1)); }
void GL::PopMatrix() { matrixState->Pop();  glPopMatrix(); CompareMatrices(matrixState->Top(0), matrixState->Top(1)); }

void GL::MultMatrix(const CMatrix44f& m) { matrixState->Mult(m);  glMultMatrixf(m); CompareMatrices(matrixState->Top(0), matrixState->Top(1)); }
void GL::LoadMatrix(const CMatrix44f& m) { matrixState->Load(m);  glLoadMatrixf(m); CompareMatrices(matrixState->Top(0), matrixState->Top(1)); }
void GL::LoadIdentity() { LoadMatrix(CMatrix44f::Identity());  glLoadIdentity(); CompareMatrices(matrixState->Top(0), matrixState->Top(1)); }

void GL::Translate(const float3& v) { matrixState->Translate(v);  glTranslatef(v.x, v.y, v.z); CompareMatrices(matrixState->Top(0), matrixState->Top(1)); }
void GL::Translate(float x, float y, float z) { matrixState->Translate({x, y, z});  glTranslatef(x, y, z); CompareMatrices(matrixState->Top(0), matrixState->Top(1)); }
void GL::Scale(const float3& s) { matrixState->Scale(s);  glScalef(s.x, s.y, s.z); CompareMatrices(matrixState->Top(0), matrixState->Top(1)); }
void GL::Scale(float x, float y, float z) { matrixState->Scale({x, y, z});  glScalef(x, y, z); CompareMatrices(matrixState->Top(0), matrixState->Top(1)); }
void GL::Rotate(float a, float x, float y, float z) { matrixState->Rotate(a, x, y, z);  glRotatef(a, x, y, z); CompareMatrices(matrixState->Top(0), matrixState->Top(1)); } // FIXME: broken
void GL::RotateX(float a) { matrixState->RotateX(a);  glRotatef(a, 1.0f, 0.0f, 0.0f); CompareMatrices(matrixState->Top(0), matrixState->Top(1)); }
void GL::RotateY(float a) { matrixState->RotateY(a);  glRotatef(a, 0.0f, 1.0f, 0.0f); CompareMatrices(matrixState->Top(0), matrixState->Top(1)); }
void GL::RotateZ(float a) { matrixState->RotateZ(a);  glRotatef(a, 0.0f, 0.0f, 1.0f); CompareMatrices(matrixState->Top(0), matrixState->Top(1)); }

#else

void GL::PushMatrix() { glPushMatrix(); }
void GL::PopMatrix() { glPopMatrix(); }

void GL::MultMatrix(const CMatrix44f& m) { glMultMatrixf(m); }
void GL::LoadMatrix(const CMatrix44f& m) { glLoadMatrixf(m); }
void GL::LoadIdentity() { glLoadIdentity(); }

void GL::Translate(const float3& v) { glTranslatef(v.x, v.y, v.z); }
void GL::Translate(float x, float y, float z) { glTranslatef(x, y, z); }
void GL::Scale(const float3& s) { glScalef(s.x, s.y, s.z); }
void GL::Scale(float x, float y, float z) { glScalef(x, y, z); }
void GL::Rotate(float a, float x, float y, float z) { glRotatef(a, x, y, z); }
void GL::RotateX(float a) { glRotatef(a, 1.0f, 0.0f, 0.0f); }
void GL::RotateY(float a) { glRotatef(a, 0.0f, 1.0f, 0.0f); }
void GL::RotateZ(float a) { glRotatef(a, 0.0f, 0.0f, 1.0f); }
#endif

