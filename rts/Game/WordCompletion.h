/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WORD_COMPLETION_H
#define WORD_COMPLETION_H

#include <string>
#include <vector>
#include <map>

class CWordCompletion
{
	CWordCompletion();

public:
	/**
	 * This function initialized a singleton instance,
	 * if not yet done by a call to GetInstance()
	 */
	static void CreateInstance();
	static CWordCompletion* GetInstance() { return singleton; }
	static void DestroyInstance();

	void AddWord(const std::string& word, bool startOfLine, bool unitName,
			bool miniMap);
	void RemoveWord(const std::string& word);
	/// Returns partial matches
	std::vector<std::string> Complete(std::string& msg) const;

private:
	void Reset();

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

	static CWordCompletion* singleton;

	std::map<std::string, WordProperties> words;
};

#define wordCompletion CWordCompletion::GetInstance()

#endif /* WORD_COMPLETION_H */
