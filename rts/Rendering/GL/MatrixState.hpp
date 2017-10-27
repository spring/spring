/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GL_MATRIX_STACK_H
#define GL_MATRIX_STACK_H

#define CHECK_MATRIX_STACK_UNDERFLOW

#ifdef CHECK_MATRIX_STACK_UNDERFLOW
#include <cassert>
#endif

#include "System/Matrix44f.h"

namespace GL {
	struct MatrixState {
	private:
		std::vector<CMatrix44f> stacks[3] = {{CMatrix44f::Identity()}, {CMatrix44f::Identity()}, {CMatrix44f::Identity()}};
		std::vector<CMatrix44f>* stack = &stacks[0];

		CMatrix44f dummy;

	public:
		const CMatrix44f& Top(unsigned int mode) const {
			#ifdef CHECK_MATRIX_STACK_UNDERFLOW
			if (stacks[mode].empty()) {
				assert(false);
				return dummy;
			}
			#endif
			return (stacks[mode].back());
		}

		CMatrix44f& Top(unsigned int mode) {
			#ifdef CHECK_MATRIX_STACK_UNDERFLOW
			if (stacks[mode].empty()) {
				assert(false);
				return dummy;
			}
			#endif
			return (stacks[mode].back());
		}

		const CMatrix44f& Top() const { return (Top(stack - &stacks[0])); }
		      CMatrix44f& Top()       { return (Top(stack - &stacks[0])); }

		void SetMode(unsigned int mode) { stack = &stacks[mode]; }

		void Push(const CMatrix44f& m) { stack->push_back(m); }
		void Push() { Push(Top()); }
		void Pop() {
			#ifdef CHECK_MATRIX_STACK_UNDERFLOW
			if (stack->empty()) {
				assert(false);
				return;
			}
			#endif
			stack->pop_back();
		}

		void Mult(const CMatrix44f& m) { Load(Top() = Top() * m); }
		void Load(const CMatrix44f& m);

		void Translate(const float3& v) { Load((Top()).Translate(v)); }
		void Translate(float x, float y, float z) { Translate({x, y, z}); }
		void Scale(const float3& s) { Load((Top()).Scale(s)); }
		void Scale(float x, float y, float z) { Scale({x, y, z}); }
		void RotateX(float a) { Load((Top()).RotateX(a)); }
		void RotateY(float a) { Load((Top()).RotateY(a)); }
		void RotateZ(float a) { Load((Top()).RotateZ(a)); }
	};


	void SetMatrixStatePointer(bool mainThread);


	void MatrixMode(unsigned int glMode);

	const CMatrix44f& GetMatrix();
	const CMatrix44f& GetMatrix(unsigned int glMode);

	void PushMatrix();
	void PopMatrix();

	void MultMatrix(const CMatrix44f& m);
	void LoadMatrix(const CMatrix44f& m);
	void LoadIdentity();

	void Translate(const float3& v);
	void Translate(float x, float y, float z);
	void Scale(const float3& s);
	void Scale(float x, float y, float z);
	void RotateX(float a);
	void RotateY(float a);
	void RotateZ(float a);
}

#endif

