/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_SHADER_STATES_HDR
#define SPRING_SHADER_STATES_HDR

#include <cinttypes>
#include <string.h>
#include <string>

#include "Rendering/GL/myGL.h"
#include "System/UnorderedMap.hpp"

// NOTE:
//   the hash used here collides too much on certain inputs (eg. team-color
//   uniforms) and is not any faster to calculate than 4 direct comparisons
//   for floating-point inputs the v* are their bitwise representations (so
//   equality testing still works as expected)
//#define USE_HASH_COMPARISON

namespace Shader {
	struct UniformState {
	public:
		static constexpr size_t NAME_BUF_LEN = 128;

	private:
		union {
			std::int32_t i[17];
			float        f[17];
		};

		/// current glGetUniformLocation
		int location;

		#ifdef DEBUG
		/// uniform type
		int type;
		#endif

		/// uniform name in the shader
		char name[NAME_BUF_LEN];

	public:
		UniformState(const char* _name): location(-1) {
			i[0] = -0xFFFFFF;
			i[1] = -0xFFFFFF;
			i[2] = -0xFFFFFF;
			i[3] = -0xFFFFFF;

			#ifdef DEBUG
			type = -1;
			#endif

			memset(name, 0, sizeof(name));
			strncpy(name, _name, sizeof(name) - 1);
		}

		const int* GetIntValues() const { return &i[0]; }
		const float* GetFltValues() const { return &f[0]; }

		int GetLocation() const { return location; }
		const char* GetName() const { return name; }

		void SetLocation(int loc) { location = loc; }

		bool IsLocationValid() const;
		bool IsInitialized() const {
			return (i[0] != -0xFFFFFF) || (i[1] != -0xFFFFFF) || (i[2] != -0xFFFFFF) || (i[3] != -0xFFFFFF);
		}

	public:
		int GetType() const {
		#ifdef DEBUG
			return type;
		#else
			return -1;
		#endif
		}
		void SetType(int type) {
		#ifdef DEBUG
			this->type = type;
		#endif
		}
	#ifdef DEBUG
		void AssertType(int type) const;
	#else
		void AssertType(int type) const {}
	#endif

	public:
		int Hash(const int v0, const int v1, const int v2, const int v3) const;
		int Hash(const int* v, int count) const;

		bool CheckHash(const int v0, const int v1 = 0, const int v2 = 0, const int v3 = 0) const {
		#ifdef USE_HASH_COMPARISON
			return (Hash(i[0], i[1], i[2], i[3]) == Hash(v0, v1, v2, v3));
		#else
			return (i[0] == v0 && i[1] == v1 && i[2] == v2 && i[3] == v3);
		#endif
		}
		bool CheckHash(const int* v, int count) const {
		#ifdef USE_HASH_COMPARISON
			return (Hash(i, count) == Hash(v, count));
		#else
			bool equal = true;
			for (int n = 0; (n < count) && equal; n++) {
				equal &= (v[n] == i[n]);
			}
			return equal;
		#endif
		}

	private:
		bool Set_(const int v0, const int v1, const int v2, const int v3) {
			if (CheckHash(v0, v1, v2, v3))
				return false;
			i[0] = v0; i[1] = v1; i[2] = v2; i[3] = v3;
			return IsLocationValid();
		}


		bool Set_(const float v0, const float v1, const float v2, const float v3) {
			const int i0 = *reinterpret_cast<const int*>(&v0);
			const int i1 = *reinterpret_cast<const int*>(&v1);
			const int i2 = *reinterpret_cast<const int*>(&v2);
			const int i3 = *reinterpret_cast<const int*>(&v3);
			if (CheckHash(i0, i1, i2, i3))
				return false;
			f[0] = v0; f[1] = v1; f[2] = v2; f[3] = v3;
			return IsLocationValid();
		}

	public:
		bool Set(const int v0, const int v1, const int v2, const int v3) { AssertType(GL_INT_VEC4); return Set_(v0, v1, v2, v3); }
		bool Set(const int v0, const int v1, const int v2              ) { AssertType(GL_INT_VEC3); return Set_(v0, v1, v2,  0); }
		bool Set(const int v0, const int v1                            ) { AssertType(GL_INT_VEC2); return Set_(v0, v1,  0,  0); }
		bool Set(const int v0                                          ) { AssertType(GL_INT     ); return Set_(v0,  0,  0,  0); }

		bool Set(const float v0, const float v1, const float v2, const float v3) { AssertType(GL_FLOAT_VEC4); return Set_(v0, v1, v2, v3); }
		bool Set(const float v0, const float v1, const float v2                ) { AssertType(GL_FLOAT_VEC3); return Set_(v0, v1, v2, 0.f); }
		bool Set(const float v0, const float v1                                ) { AssertType(GL_FLOAT_VEC2); return Set_(v0, v1, 0.f, 0.f); }
		bool Set(const float v0                                                ) { AssertType(GL_FLOAT     ); return Set_(v0, 0.f, 0.f, 0.f); }

		bool Set2v(const int* v) {
			AssertType(GL_INT_VEC2);
			if (CheckHash(v[0], v[1]))
				return false;
			i[0] = v[0]; i[1] = v[1];
			return IsLocationValid();
		}
		bool Set3v(const int* v) {
			AssertType(GL_INT_VEC3);
			if (CheckHash(v[0], v[1], v[2]))
				return false;
			i[0] = v[0]; i[1] = v[1]; i[2] = v[2];
			return IsLocationValid();
		}
		bool Set4v(const int* v) {
			AssertType(GL_INT_VEC4);
			if (CheckHash(v[0], v[1], v[2], v[3]))
				return false;
			i[0] = v[0]; i[1] = v[1]; i[2] = v[2]; i[3] = v[3];
			return IsLocationValid();
		}


		bool Set2v(const float* v) {
			AssertType(GL_FLOAT_VEC2);
			const int* vi = reinterpret_cast<const int*>(v);
			if (CheckHash(vi[0], vi[1]))
				return false;
			f[0] = v[0]; f[1] = v[1];
			return IsLocationValid();
		}
		bool Set3v(const float* v) {
			AssertType(GL_FLOAT_VEC3);
			const int* vi = reinterpret_cast<const int*>(v);
			if (CheckHash(vi[0], vi[1], vi[2]))
				return false;
			f[0] = v[0]; f[1] = v[1]; f[2] = v[2];
			return IsLocationValid();
		}
		bool Set4v(const float* v) {
			AssertType(GL_FLOAT_VEC4);
			const int* vi = reinterpret_cast<const int*>(v);
			if (CheckHash(vi[0], vi[1], vi[2], vi[3]))
				return false;
			f[0] = v[0]; f[1] = v[1]; f[2] = v[2]; f[3] = v[3];
			return IsLocationValid();
		}


		bool Set2x2(const float* v, bool transp) {
			AssertType(GL_FLOAT_MAT2);
			const int* vi = reinterpret_cast<const int*>(v);
			if (CheckHash(vi, 4) && (bool)i[16] == transp)
				return false;
			memcpy(f, v, 4 * sizeof(float));
			i[16] = transp;
			return IsLocationValid();
		}
		bool Set3x3(const float* v, bool transp) {
			AssertType(GL_FLOAT_MAT3);
			const int* vi = reinterpret_cast<const int*>(v);
			if (CheckHash(vi, 9) && (bool)i[16] == transp)
				return false;
			memcpy(f, v, 9 * sizeof(float));
			i[16] = transp;
			return IsLocationValid();
		}
		bool Set4x4(const float* v, bool transp) {
			AssertType(GL_FLOAT_MAT4);
			const int* vi = reinterpret_cast<const int*>(v);
			if (CheckHash(vi, 16) && (bool)i[16] == transp)
				return false;
			memcpy(f, v, 16 * sizeof(float));
			i[16] = transp;
			return IsLocationValid();
		}
	};




	struct ShaderFlags {
	public:
		ShaderFlags() { Clear(); }
		ShaderFlags(const ShaderFlags& sf) = delete;
		ShaderFlags(ShaderFlags&& sf) { *this = std::move(sf); }

		ShaderFlags& operator = (const ShaderFlags& sf) = delete;
		ShaderFlags& operator = (ShaderFlags&& sf) {
			bitFlags = std::move(sf.bitFlags);
			intFlags = std::move(sf.intFlags);
			fltFlags = std::move(sf.fltFlags);

			numValUpdates = sf.numValUpdates;
			prvValUpdates = sf.prvValUpdates;
			flagHashValue = sf.flagHashValue;
			return *this;
		}

		std::string GetString() const {
			char buf[8192] = {0};
			char* ptr = &buf[0];

			#define PV(p) ((p).second).second
			#define RC(p) reinterpret_cast<const char*>((p).first)

			// boolean flags are checked via #ifdef's (not #if's) in a subset of shaders
			for (const auto& p: bitFlags) { if (PV(p)) ptr += snprintf(ptr, sizeof(buf) - (ptr - buf), "#define %s\n"   , RC(p)       ); }
			for (const auto& p: intFlags) {            ptr += snprintf(ptr, sizeof(buf) - (ptr - buf), "#define %s %d\n", RC(p), PV(p)); }
			for (const auto& p: fltFlags) {            ptr += snprintf(ptr, sizeof(buf) - (ptr - buf), "#define %s %f\n", RC(p), PV(p)); }

			#undef RC
			#undef PV
			return buf;
		}

		unsigned int CalcHash() const;
		unsigned int UpdateHash() { return (prvValUpdates = numValUpdates, flagHashValue = CalcHash()); }

		void Clear() {
			bitFlags.clear();
			intFlags.clear();
			fltFlags.clear();

			numValUpdates = 0;
			prvValUpdates = 0;
			flagHashValue = 0;
		}

		void Set(const char* key, bool val) {
			const auto it = bitFlags.find(key);

			// new keys or changed values count as an update
			// numBitUpdates += (it == bitFlags.end() || (it->second).second != val);
			numValUpdates += (it == bitFlags.end() || (it->second).second != val);

			bitFlags[key] = {(it == bitFlags.end())? strlen(key): (it->second).first, val};
		}

		void Set(const char* key, unsigned int val) { Set(key, int(val)); }
		void Set(const char* key, int val) {
			const auto it = intFlags.find(key);

			// numIntUpdates += (it == intFlags.end() || (it->second).second != val);
			numValUpdates += (it == intFlags.end() || (it->second).second != val);

			intFlags[key] = {(it == intFlags.end())? strlen(key): (it->second).first, val};
		}

		void Set(const char* key, float val) {
			const auto it = fltFlags.find(key);

			// numFltUpdates += (it == fltFlags.end() || (it->second).second != val);
			numValUpdates += (it == fltFlags.end() || (it->second).second != val);

			fltFlags[key] = {(it == fltFlags.end())? strlen(key): (it->second).first, val};
		}

		bool HashSet() const { return (flagHashValue != 0); }
		bool Updated() const { return (numValUpdates != prvValUpdates); }

	private:
		// NOTE: *only* pointers to constant addresses are allowed (literals, globals)
		spring::unsynced_map<const void*, std::pair<unsigned int,  bool> > bitFlags;
		spring::unsynced_map<const void*, std::pair<unsigned int,   int> > intFlags;
		spring::unsynced_map<const void*, std::pair<unsigned int, float> > fltFlags;

		unsigned int numValUpdates;
		unsigned int prvValUpdates;
		unsigned int flagHashValue;
	};
}


#endif
