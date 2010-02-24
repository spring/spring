/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WORD_COMPLETION_H
#define WORD_COMPLETION_H

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
		void RemoveWord(const std::string& word);
		/// Returns partial matches
		std::vector<std::string> Complete(std::string& msg) const;

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
