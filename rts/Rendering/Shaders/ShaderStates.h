/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_SHADER_STATES_HDR
#define SPRING_SHADER_STATES_HDR

#include "Rendering/GL/myGL.h"

#include <boost/cstdint.hpp>
#include <string.h>
#include <string>
#include <unordered_map>
#include <sstream>

// NOTE:
//   the hash used here collides too much on certain inputs (eg. team-color
//   uniforms) and is not any faster to calculate than 4 direct comparisons
//   for floating-point inputs the v* are their bitwise representations (so
//   equality testing still works as expected)
//#define USE_HASH_COMPARISON

namespace Shader {
	struct UniformState {
	private:
		union {
			boost::int32_t i[17];
			float          f[17];
		};

		/// current glGetUniformLocation
		int location;

		/// uniform name in the shader
		std::string name;

	#ifdef DEBUG
		/// uniform type
		int type;
	#endif

	public:
		UniformState(const std::string& _name): location(-1), name(_name) {
			i[0] = -0xFFFFFF;
			i[1] = -0xFFFFFF;
			i[2] = -0xFFFFFF;
			i[3] = -0xFFFFFF;
		#ifdef DEBUG
			type = -1;
		#endif
		}

		const int* GetIntValues() const { return &i[0]; }
		const float* GetFltValues() const { return &f[0]; }

		int GetLocation() const { return location; }
		const std::string& GetName() const { return name; }

		void SetLocation(int loc) { location = loc; }

		bool IsLocationValid() const;
		bool IsUninit() const {
			return (i[0] == -0xFFFFFF) && (i[1] == -0xFFFFFF) && (i[2] == -0xFFFFFF) && (i[3] == -0xFFFFFF);
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
		bool Set(const int v0, const int v1, const int v2, const int v3)
			{ AssertType(GL_INT_VEC4); return Set_(v0, v1, v2, v3); }
		bool Set(const int v0, const int v1, const int v2)
			{ AssertType(GL_INT_VEC3); return Set_(v0, v1, v2,  0); }
		bool Set(const int v0, const int v1)
			{ AssertType(GL_INT_VEC2); return Set_(v0, v1,  0,  0); }
		bool Set(const int v0)
			{ AssertType(GL_INT     ); return Set_(v0,  0,  0,  0); }

		bool Set(const float v0, const float v1, const float v2, const float v3)
			{ AssertType(GL_FLOAT_VEC4); return Set_(v0, v1, v2, v3); }
		bool Set(const float v0, const float v1, const float v2)
			{ AssertType(GL_FLOAT_VEC3); return Set_(v0, v1, v2, 0.f); }
		bool Set(const float v0, const float v1)
			{ AssertType(GL_FLOAT_VEC2); return Set_(v0, v1, 0.f, 0.f); }
		bool Set(const float v0)
			{ AssertType(GL_FLOAT     ); return Set_(v0, 0.f, 0.f, 0.f); }

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


	struct SShaderFlagState {
	public:
		SShaderFlagState() : updates(1), lastUpdates(0), lastHash(0) {}
		virtual ~SShaderFlagState() {}

		unsigned int GetHash();

		void ClearHash()
		{
			lastHash = 0;
			updates = 1;
			lastUpdates = 0;
		}

		std::string GetString() const
		{
			std::ostringstream strbuf;
			for (auto it = flags.begin(); it != flags.end(); ++it) {
				strbuf << "#define " << it->first << " " << it->second << std::endl;
			}
			return strbuf.str();
		}


		void ClearFlag(const std::string& flag)
		{
			++updates;
			flags.erase(flag);
		}


		template <typename T>
		void SetFlag(const std::string& flag, const T newvalue)
		{
			++updates;
			std::ostringstream buffer;
			buffer << newvalue;
			flags[flag] = buffer.str();
		}

		// specializations
		void SetFlag(const std::string& flag, const std::string& newvalue)
		{
			++updates;
			flags[flag] = newvalue;
		}

		void SetFlag(const std::string& flag, const bool enable)
		{
			if (enable) {
				++updates;
				flags[flag] = "";
			} else {
				ClearFlag(flag);
			}
		}


		template<typename T>
		T GetFlag(const std::string& flag) const
		{
			auto it = flags.find(flag);
			if (it != flags.end()) {
				std::istringstream buf(it->second);
				T temp;
				buf >> temp;
				return temp;
			}
			return T();
		}

		bool GetFlagBool(const std::string& flag) const
		{
			return (flags.find(flag) != flags.end());
		}

		const std::string& GetFlagString(const std::string& flag) const
		{
			auto it = flags.find(flag);
			if (it != flags.end()) {
				return it->second;
			} else {
				static std::string nulstr;
				return nulstr;
			}
		}

		bool HasFlag(const std::string& flag) const
		{
			return (flags.find(flag) != flags.end());
		}

	private:
		int updates;
		int lastUpdates;
		int lastHash;
		std::unordered_map<std::string, std::string> flags;
		//std::set<std::string> skipAutoUpdate;
	};
}


#endif
