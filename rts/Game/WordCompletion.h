#ifndef WORD_COMPLETION_H
#define WORD_COMPLETION_H
// WordCompletion.h: interface for the CWordCompletion class.
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>
#include <map>

class CWordCompletion
{
	public:
		CWordCompletion();
		~CWordCompletion();
		void Reset();
		void AddWord(const std::string& word,
		             bool startOfLine, bool unitName, bool minimap);
		std::vector<std::string> Complete(std::string& msg) const; // returns partial matches

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
		std::map<std::string, WordProperties> words;
};

#endif /* WORD_COMPLETION_H */
