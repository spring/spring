
#ifndef SIDE_PARSER_H
#define SIDE_PARSER_H

#include <string>
#include <vector>


class SideParser
{
	public:
		SideParser();
		~SideParser();

		bool Load();
		const std::string& GetErrorLog() const { return errorLog; }

		const unsigned int GetCount() const { return dataVec.size(); }

		const std::string& GetSideName(unsigned int index,
		                               const std::string& def = "") const;
		const std::string& GetCaseName(unsigned int index,
		                               const std::string& def = "") const;
		const std::string& GetCaseName(const std::string& name,
		                               const std::string& def = "") const;
		const std::string& GetStartUnit(unsigned int index,
		                                const std::string& def = "") const;
		const std::string& GetStartUnit(const std::string& name,
		                                const std::string& def = "") const;

		bool ValidSide(unsigned int index) const {
			return (index < dataVec.size());
		}

	private:
		struct Data {
			std::string caseName;  // full  case
			std::string sideName;  // lower case
			std::string startUnit; // lower case
		};
		typedef std::vector<Data> DataVec;

		const Data* FindSide(const std::string& sideName) const;

	private:
		DataVec dataVec;
		std::string errorLog;
};


extern SideParser sideParser;


#endif // SIDE_PARSER_H
