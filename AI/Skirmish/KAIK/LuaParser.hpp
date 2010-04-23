#ifndef KAIK_LUA_PARSER_HDR
#define KAIK_LUA_PARSER_HDR

#include <list>
#include <map>
#include <string>

struct lua_State;

struct LuaTable {
public:
	~LuaTable() {
		for (std::map<LuaTable*, LuaTable*>::iterator it = TblTblPairs.begin(); it != TblTblPairs.end(); it++) {
			delete it->first;
			delete it->second;
		}
		for (std::map<LuaTable*, std::string>::iterator it = TblStrPairs.begin(); it != TblStrPairs.end(); it++) {
			delete it->first;
		}
		for (std::map<LuaTable*, int>::iterator it = TblIntPairs.begin(); it != TblIntPairs.end(); it++) {
			delete it->first;
		}
		for (std::map<std::string, LuaTable*>::iterator it = StrTblPairs.begin(); it != StrTblPairs.end(); it++) {
			delete it->second;
		}
		for (std::map<int, LuaTable*>::iterator it = IntTblPairs.begin(); it != IntTblPairs.end(); it++) {
			delete it->second;
		}
	}

	bool operator < (const LuaTable& t) const {
		return (this < &t);
	}

	bool operator == (const LuaTable& t) const {
		return (
			TblTblPairs == t.TblTblPairs &&
			TblStrPairs == t.TblStrPairs &&
			TblIntPairs == t.TblIntPairs &&
			StrTblPairs == t.StrTblPairs &&
			StrStrPairs == t.StrStrPairs &&
			StrIntPairs == t.StrIntPairs &&
			IntTblPairs == t.IntTblPairs &&
			IntStrPairs == t.IntStrPairs &&
			IntIntPairs == t.IntIntPairs
		);
	}

	void Print(int) const;
	void Parse(lua_State*, int);



	typedef std::pair<LuaTable*, LuaTable*>     TblTblPair;
	typedef std::pair<LuaTable*, std::string>   TblStrPair;
	typedef std::pair<LuaTable*, int>           TblIntPair;
	typedef std::pair<std::string, LuaTable*>   StrTblPair;
	typedef std::pair<std::string, std::string> StrStrPair;
	typedef std::pair<std::string, int>         StrIntPair;
	typedef std::pair<int, LuaTable*>           IntTblPair;
	typedef std::pair<int, std::string>         IntStrPair;
	typedef std::pair<int, int>                 IntIntPair;

	void GetTblTblKeys(std::list<LuaTable*>*  ) const;
	void GetTblStrKeys(std::list<LuaTable*>*  ) const;
	void GetTblIntKeys(std::list<LuaTable*>*  ) const;
	void GetStrTblKeys(std::list<std::string>*) const;
	void GetStrStrKeys(std::list<std::string>*) const;
	void GetStrIntKeys(std::list<std::string>*) const;
	void GetIntTblKeys(std::list<int>*        ) const;
	void GetIntStrKeys(std::list<int>*        ) const;
	void GetIntIntKeys(std::list<int>*        ) const;

	const LuaTable* GetTblVal(LuaTable* key, LuaTable* defVal = 0) const;
	const LuaTable* GetTblVal(const std::string& key, LuaTable* defVal = 0) const;
	const LuaTable* GetTblVal(int key, LuaTable* defVal = 0) const;

	const std::string& GetStrVal(LuaTable* key, const std::string& defVal) const;
	const std::string& GetStrVal(const std::string& key, const std::string& defVal) const;
	const std::string& GetStrVal(int key, const std::string& defVal) const;

	int GetIntVal(LuaTable* key, int defVal) const;
	int GetIntVal(const std::string& key, int defVal) const;
	int GetIntVal(int key, int defVal) const;

	bool HasStrTblKey(const std::string& key) const { return (StrTblPairs.find(key) != StrTblPairs.end()); }
	bool HasStrStrKey(const std::string& key) const { return (StrStrPairs.find(key) != StrStrPairs.end()); }
	bool HasStrIntKey(const std::string& key) const { return (StrIntPairs.find(key) != StrIntPairs.end()); }

	template<typename T> void GetArray(const LuaTable* tbl, T* array, int len) const {
		for (int i = 0; i < len; i++) {
			array[i] = T(tbl->GetIntVal(i + 1, T(0)));
		}
	}

	template<typename V> V GetVec(const std::string& key, int len) const {
		const std::map<std::string, LuaTable*>::const_iterator it = StrTblPairs.find(key);

		V v;

		if (it != StrTblPairs.end()) {
			GetArray(it->second, &v.x, len);
		}

		return v;
	}

private:
	std::map<LuaTable*, LuaTable*>     TblTblPairs;
	std::map<LuaTable*, std::string>   TblStrPairs;
	std::map<LuaTable*, int>           TblIntPairs;
	std::map<std::string, LuaTable*>   StrTblPairs;
	std::map<std::string, std::string> StrStrPairs;
	std::map<std::string, int>         StrIntPairs;
	std::map<int, LuaTable*>           IntTblPairs;
	std::map<int, std::string>         IntStrPairs;
	std::map<int, int>                 IntIntPairs;
};


struct LuaParser {
public:
	LuaParser();
	~LuaParser();

	bool Execute(const std::string&, const std::string&);

	const LuaTable* GetRootTbl() const { return root; }
	const std::string& GetError() const { return error; }

private:
	lua_State* luaState;
	LuaTable* root;

	std::map<std::string, LuaTable*> tables;
	std::string error;
};

#endif
