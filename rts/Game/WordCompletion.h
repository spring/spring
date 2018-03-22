/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WORD_COMPLETION_H
#define WORD_COMPLETION_H

#include <string>
#include <vector>

class CWordCompletion
{
public:
	void Init();
	void Sort();
	void Filter();

	bool AddWord(const std::string& word, bool startOfLine, bool unitName, bool miniMap);
	bool AddWordRaw(const std::string& word, bool startOfLine, bool unitName, bool miniMap);
	bool RemoveWord(const std::string& word);

	/// Returns partial matches
	std::vector<std::string> Complete(std::string& msg) const;

private:
	class WordProperties {
		public:
			WordProperties()
				: startOfLine(false)
				, unitName(false)
				, miniMap(false)
			{}
			WordProperties(bool startOfLine, bool unitName, bool miniMap)
				: startOfLine(startOfLine)
				, unitName(unitName)
				, miniMap(miniMap)
			{}

			bool startOfLine;
			bool unitName;
			bool miniMap;
	};

	typedef std::pair<std::string, WordProperties> WordEntry;

	std::vector<WordEntry> words;
};

extern CWordCompletion wordCompletion;

#endif /* WORD_COMPLETION_H */
