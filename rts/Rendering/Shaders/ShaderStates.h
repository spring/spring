/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_SHADER_STATES_HDR
#define SPRING_SHADER_STATES_HDR

#include <boost/cstdint.hpp>
#include <string.h>
#include <string>
#include <map>
#include <sstream>
#include "lib/gml/gml_base.h"


namespace Shader {
	struct UniformState {
		union {
			boost::int32_t i[17];
			float   f[17];
		};
		int type; //TODO implement (should be either GL_FLOAT_VEC2, GL_INT_SAMPLER_CUBE, ... see GLSLCopyState.cpp)
		std::string name;

		UniformState(const std::string& name) : name(name) {
			i[0] = -6666;
			i[1] = -6666;
			i[2] = -6666;
			i[3] = -6666;
		}

		int Hash(const int v0, const int v1, const int v2, const int v3) const {
			int hash = ~0;//FIXME check if this is really faster than a for() if()
			hash += v0 ^ (hash * 33);
			hash += v1 ^ (hash * 33);
			hash += v2 ^ (hash * 33);
			hash += v3 ^ (hash * 33);
			return hash;
		}
		bool CheckHash(const int v0, const int v1 = 0, const int v2 = 0, const int v3 = 0) const {
			return (Hash(i[0], i[1], i[2], i[3]) == Hash(v0, v1, v2, v3));
		}

		int Hash(const int* v, int count) const {
			int hash = ~0;
			for (int i = 0; i < count; ++i) {
				hash += v[i] ^ (hash * 33);
			}
			return hash;
		}
		bool CheckHash(const int* v, int count) const {
			return (Hash(i, count) == Hash(v, count));
		}


		bool Set(const int v0, const int v1 = 0, const int v2 = 0, const int v3 = 0) {
			if (GML::ServerActive()) return true;
			if (CheckHash(v0, v1, v2, v3))
				return false;
			i[0] = v0; i[1] = v1; i[2] = v2; i[3] = v3;
			return true;
		}


		bool Set(const float v0, const float v1 = 0.0f, const float v2 = 0.0f, const float v3 = 0.0f) {
			if (GML::ServerActive()) return true;
			int i0 = *reinterpret_cast<const int*>(&v0);
			int i1 = *reinterpret_cast<const int*>(&v1);
			int i2 = *reinterpret_cast<const int*>(&v2);
			int i3 = *reinterpret_cast<const int*>(&v3);
			if (CheckHash(i0, i1, i2, i3))
				return false;
			f[0] = v0; f[1] = v1; f[2] = v2; f[3] = v3;
			return true;
		}


		bool Set2v(const int* v) {
			if (GML::ServerActive()) return true;
			if (CheckHash(v[0], v[1]))
				return false;
			i[0] = v[0]; i[1] = v[1];
			return true;
		}
		bool Set3v(const int* v) {
			if (GML::ServerActive()) return true;
			if (CheckHash(v[0], v[1], v[2]))
				return false;
			i[0] = v[0]; i[1] = v[1]; i[2] = v[2];
			return true;
		}
		bool Set4v(const int* v) {
			if (GML::ServerActive()) return true;
			if (CheckHash(v[0], v[1], v[2], v[3]))
				return false;
			i[0] = v[0]; i[1] = v[1]; i[2] = v[2]; i[3] = v[3];
			return true;
		}


		bool Set2v(const float* v) {
			if (GML::ServerActive()) return true;
			const int* vi = reinterpret_cast<const int*>(v);
			if (CheckHash(vi[0], vi[1]))
				return false;
			f[0] = v[0]; f[1] = v[1];
			return true;
		}
		bool Set3v(const float* v) {
			if (GML::ServerActive()) return true;
			const int* vi = reinterpret_cast<const int*>(v);
			if (CheckHash(vi[0], vi[1], vi[2]))
				return false;
			f[0] = v[0]; f[1] = v[1]; f[2] = v[2];
			return true;
		}
		bool Set4v(const float* v) {
			if (GML::ServerActive()) return true;
			const int* vi = reinterpret_cast<const int*>(v);
			if (CheckHash(vi[0], vi[1], vi[2], vi[3]))
				return false;
			f[0] = v[0]; f[1] = v[1]; f[2] = v[2]; f[3] = v[3];
			return true;
		}


		bool Set2x2(const float* v, bool transp) {
			if (GML::ServerActive()) return true;
			const int* vi = reinterpret_cast<const int*>(v);
			if (CheckHash(vi, 4) && (bool)i[16] == transp)
				return false;
			memcpy(f, v, 4 * sizeof(float));
			i[16] = transp;
			return true;
		}
		bool Set3x3(const float* v, bool transp) {
			if (GML::ServerActive()) return true;
			const int* vi = reinterpret_cast<const int*>(v);
			if (CheckHash(vi, 9) && (bool)i[16] == transp)
				return false;
			memcpy(f, v, 9 * sizeof(float));
			i[16] = transp;
			return true;
		}
		bool Set4x4(const float* v, bool transp) {
			if (GML::ServerActive()) return true;
			const int* vi = reinterpret_cast<const int*>(v);
			if (CheckHash(vi, 16) && (bool)i[16] == transp)
				return false;
			memcpy(f, v, 16 * sizeof(float));
			i[16] = transp;
			return true;
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
				std::istringstream buf(*it);
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
