//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#ifndef JC_CONFIG_PARSER_H
#define JC_CONFIG_PARSER_H

#include <string>
#include <list>

class CfgValue;
class CfgListElem;
class CfgList;
class CfgNumeric;
class CfgLiteral;

struct CfgBuffer
{
	CfgBuffer () : pos(0), line(1), data(0), len(0), filename(0) {}
	char operator*() const { return data[pos]; }
	bool end() const {  return pos == len; }
	CfgBuffer& operator++() { if (pos<len) pos++; return *this; }
	CfgBuffer operator++(int) { CfgBuffer t=*this; pos++; return t; }
	char operator[](int i)const { return data[pos+i]; }
	void ShowLocation() const;
	bool CompareIdent (const char *str) const;

	const char *filename;
	int pos;
	int len;
	char *data;
	int line;
};

class CfgValueClass
{
public:
	virtual CfgValue* Create () = 0;
	virtual bool Identify (const CfgBuffer& buf) = 0; // only lookahead
};

class CfgValue 
{
public:
	CfgValue() {}
	virtual ~CfgValue() {}

	virtual bool Parse (CfgBuffer& buf) = 0;
	virtual void dbgPrint (int depth);

	static void Expecting (CfgBuffer& buf, const char *s);
	static bool ParseIdent (CfgBuffer& buf, string& name);
	static bool SkipWhitespace (CfgBuffer& buf);
	static char Lookahead (CfgBuffer& buf);
	static CfgValue* ParseValue(CfgBuffer& buf);
	static CfgList* LoadNestedFile(CfgBuffer& buf);
	static CfgList* LoadFile(const char *name);
	static bool SkipKeyword (CfgBuffer& buf, const char *kw);

	static void AddValueClass (CfgValueClass *vc);
protected:
	static vector<CfgValueClass *> classes;
};

class CfgListElem : public CfgValue
{
public:
	CfgListElem() { value=0; }
	~CfgListElem() { if (value) delete value; }
	bool Parse (CfgBuffer& buf);

	string name;
	CfgValue *value;
};

// list of cfg nodes
class CfgList : public CfgValue
{
public:
	bool Parse (CfgBuffer& buf, bool root);
	bool Parse (CfgBuffer& buf) { return Parse (buf, false); }

	void dbgPrint (int depth);

	CfgValue* GetValue (const char *name);
	double GetNumeric(const char *name, double def=0.0f);
	const char* GetLiteral(const char *name, const char *def=0);
	int GetInt (const char *name, int def) { return (int) GetNumeric (name, def); }

	list<CfgListElem> childs;
};

class CfgLiteral : public CfgValue  {
public:
	CfgLiteral () { ident=false; }
	bool Parse (CfgBuffer& buf);
	void dbgPrint (int depth);

	bool ident;
	string value;
};

class CfgNumeric : public CfgValue {
public:
	CfgNumeric () { value=0.0; }
	bool Parse (CfgBuffer& buf);
	void dbgPrint (int depth);

	double value;
};


#endif
