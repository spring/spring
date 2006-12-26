#ifndef WORD_COMPLETION_H
#define WORD_COMPLETION_H
// WordCompletion.h: interface for the CWordCompletion class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <string>
#include <vector>
#include <map>

using namespace std;

class CWordCompletion 
{
	public:
		CWordCompletion();
		~CWordCompletion();
		void Reset();
		void AddWord(const string& word,
		             bool startOfLine, bool unitName, bool minimap);
		vector<string> Complete(string& msg) const; // returns partial matches

	protected:
		class WordProperties {
			public:
				WordProperties()
				: startOfLine(false), unitName(false), minimap(false) {}
				WordProperties(bool sol, bool un, bool mm)
				: startOfLine(sol), unitName(un), minimap(mm) {}
			public:
				bool startOfLine;
				bool unitName;
				bool minimap;
		};
		map<string, WordProperties> words;
};

#endif /* WORD_COMPLETION_H */
