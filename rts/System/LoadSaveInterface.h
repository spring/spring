#ifndef LOADSAVEINTERFACE_H
#define LOADSAVEINTERFACE_H

#include <string>

#include "Sync/SyncedFloat3.h"
#include "float3.h"

class CLoadSaveInterface
{
public:
	CLoadSaveInterface(void);
	virtual ~CLoadSaveInterface(void);

	virtual void lsBool(bool& v)=0;
	virtual void lsChar(char& v)=0;
	virtual void lsUChar(unsigned char& v)=0;
	virtual void lsInt(int& v)=0;
	virtual void lsShort(short int& v)=0;
	virtual void lsFloat(float& v)=0;
	virtual void lsFloat3(float3& v)=0;
	virtual void lsDouble(double& v)=0;
	virtual void lsString(std::string& v)=0;

#if defined(SYNCDEBUG) || defined(SYNCCHECK)
	inline void lsBool(SyncedBool& v) {
		bool w = v;
		this->lsBool(w);
		v = w;
	}
	inline void lsChar(SyncedSchar& v) {
		//assumes char == signed char
		char w = v;
		this->lsChar(w);
		v = w;
	}
	inline void lsUChar(SyncedUchar& v) {
		unsigned char w = v;
		this->lsUChar(w);
		v = w;
	}
	inline void lsInt(SyncedSint& v) {
		int w = v;
		this->lsInt(w);
		v = w;
	}
	inline void lsShort(SyncedSshort& v) {
		short w = v;
		this->lsShort(w);
		v = w;
	}
	inline void lsFloat(SyncedFloat& v) {
		float w = v;
		this->lsFloat(w);
		v = w;
	}
	inline void lsFloat3(SyncedFloat3& v) {
		float3 w = v;
		this->lsFloat3(w);
		v = w;
	}
	inline void lsDouble(SyncedDouble& v) {
		double w = v;
		this->lsDouble(w);
		v = w;
	}
#endif
};

#endif /* LOADSAVEINTERFACE_H */
