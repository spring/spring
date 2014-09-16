/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_SHADER_STATES_HDR
#define SPRING_SHADER_STATES_HDR

#include <boost/cstdint.hpp>
#include <string.h>
#include <string>
#include <map>
#include <sstream>

namespace Shader {
	struct UniformState {
	private:
		union {
			boost::int32_t i[17];
			float          f[17];
		};

		// TODO implement (should be either GL_FLOAT_VEC2, GL_INT_SAMPLER_CUBE, ... see GLSLCopyState.cpp)
		// int type;
		// current glGetUniformLocation
		int location;

		std::string name;

	public:
		UniformState(const std::string& _name): location(-1), name(_name) {
			i[0] = -0xFFFFFF;
			i[1] = -0xFFFFFF;
			i[2] = -0xFFFFFF;
			i[3] = -0xFFFFFF;
		}

		const int* GetIntValues() const { return &i[0]; }
		const float* GetFltValues() const { return &f[0]; }

		// int GetType() const { return type; }
		int GetLocation() const { return location; }
		const std::string& GetName() const { return name; }

		// void SetType(int type) { type = type; }
		void SetLocation(int loc) { location = loc; }

		bool IsLocationValid() const;

		bool IsUninit() const {
			return (i[0] == -0xFFFFFF) && (i[1] == -0xFFFFFF) && (i[2] == -0xFFFFFF) && (i[3] == -0xFFFFFF);
		}

	public:
		int Hash(const int v0, const int v1, const int v2, const int v3) const {
			int hash = ~0;//FIXME check if this is really faster than a for() if()
			hash += v0 ^ (hash * 33);
			hash += v1 ^ (hash * 33);
			hash += v2 ^ (hash * 33);
			hash += v3 ^ (hash * 33);
			return hash;
		}
		int Hash(const int* v, int count) const {
			int hash = ~0;
			for (int n = 0; n < count; ++n) {
				hash += v[n] ^ (hash * 33);
			}
			return hash;
		}

		bool CheckHash(const int v0, const int v1 = 0, const int v2 = 0, const int v3 = 0) const {
			// NOTE:
			//   the hash used here collides too much on certain inputs (eg. team-color
			//   uniforms) and is not any faster to calculate than 4 direct comparisons
			//   for floating-point inputs the v* are their bitwise representations (so
			//   equality testing still works as expected)
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

	public:
		bool Set(const int v0, const int v1 = 0, const int v2 = 0, const int v3 = 0) {
			if (CheckHash(v0, v1, v2, v3))
				return false;
			i[0] = v0; i[1] = v1; i[2] = v2; i[3] = v3;
			return IsLocationValid();
		}


		bool Set(const float v0, const float v1 = 0.0f, const float v2 = 0.0f, const float v3 = 0.0f) {
			const int i0 = *reinterpret_cast<const int*>(&v0);
			const int i1 = *reinterpret_cast<const int*>(&v1);
			const int i2 = *reinterpret_cast<const int*>(&v2);
			const int i3 = *reinterpret_cast<const int*>(&v3);
			if (CheckHash(i0, i1, i2, i3))
				return false;
			f[0] = v0; f[1] = v1; f[2] = v2; f[3] = v3;
			return IsLocationValid();
		}


		bool Set2v(const int* v) {
			if (CheckHash(v[0], v[1]))
				return false;
			i[0] = v[0]; i[1] = v[1];
			return IsLocationValid();
		}
		bool Set3v(const int* v) {
			if (CheckHash(v[0], v[1], v[2]))
				return false;
			i[0] = v[0]; i[1] = v[1]; i[2] = v[2];
			return IsLocationValid();
		}
		bool Set4v(const int* v) {
			if (CheckHash(v[0], v[1], v[2], v[3]))
				return false;
			i[0] = v[0]; i[1] = v[1]; i[2] = v[2]; i[3] = v[3];
			return IsLocationValid();
		}


		bool Set2v(const float* v) {
			const int* vi = reinterpret_cast<const int*>(v);
			if (CheckHash(vi[0], vi[1]))
				return false;
			f[0] = v[0]; f[1] = v[1];
			return IsLocationValid();
		}
		bool Set3v(const float* v) {
			const int* vi = reinterpret_cast<const int*>(v);
			if (CheckHash(vi[0], vi[1], vi[2]))
				return false;
			f[0] = v[0]; f[1] = v[1]; f[2] = v[2];
			return IsLocationValid();
		}
		bool Set4v(const float* v) {
			const int* vi = reinterpret_cast<const int*>(v);
			if (CheckHash(vi[0], vi[1], vi[2], vi[3]))
				return false;
			f[0] = v[0]; f[1] = v[1]; f[2] = v[2]; f[3] = v[3];
			return IsLocationValid();
		}


		bool Set2x2(const float* v, bool transp) {
			const int* vi = reinterpret_cast<const int*>(v);
			if (CheckHash(vi, 4) && (bool)i[16] == transp)
				return false;
			memcpy(f, v, 4 * sizeof(float));
			i[16] = transp;
			return IsLocationValid();
		}
		bool Set3x3(const float* v, bool transp) {
			const int* vi = reinterpret_cast<const int*>(v);
			if (CheckHash(vi, 9) && (bool)i[16] == transp)
				return false;
			memcpy(f, v, 9 * sizeof(float));
			i[16] = transp;
			return IsLocationValid();
		}
		bool Set4x4(const float* v, bool transp) {
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
		SShaderFlagState() : updates(0), lastUpdates(0), lastHash(0) {}
		virtual ~SShaderFlagState() {}

		unsigned int GetHash();

		std::string GetString() const
		{
			std::ostringstream strbuf;
			for (std::map<std::string, std::string>::const_iterator it = flags.begin(); it != flags.end(); ++it) {
				strbuf << "#define " << it->first << " " << it->second << std::endl;
			}
			return strbuf.str();
		}


		void ClearFlag(const std::string& flag)
		{
			++updates;
			flags.erase(flag);
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

		//FIXME gives compiletime error
		/*bool GetFlag(const std::string& flag) const
		{
			std::map<std::string, std::string>::const_iterator it = flags.find(flag);
			if (it != flags.end()) {
				return true;
			} else {
				return false;
			}
		}*/

		void SetFlag(const std::string& flag, const std::string& newvalue)
		{
			++updates;
			flags[flag] = newvalue;
		}

		const std::string& GetFlag(const std::string& flag) const
		{
			std::map<std::string, std::string>::const_iterator it = flags.find(flag);
			if (it != flags.end()) {
				return it->second;
			} else {
				static std::string nulstr;
				return nulstr;
			}
		}

		template <typename T>
		void SetFlag(const std::string& flag, const T newvalue)
		{
			++updates;
			std::ostringstream buffer;
			buffer << newvalue;
			flags[flag] = buffer.str();
		}

		template <typename T>
		T GetFlag(const std::string& flag) const
		{
			std::map<std::string, std::string>::const_iterator it = flags.find(flag);
			if (it != flags.end()) {
				std::istringstream buf(it->second);
				T temp;
				buf >> temp;
				return temp;
			} else {
				return T();
			}

		}

	private:
		int updates;
		int lastUpdates;
		int lastHash;
		std::map<std::string, std::string> flags;
		//std::set<std::string> skipAutoUpdate;
	};
}

#endif
