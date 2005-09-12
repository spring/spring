#ifndef LOADSAVEINTERFACE_H
#define LOADSAVEINTERFACE_H

#include <string>

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
};


#endif /* LOADSAVEINTERFACE_H */
