//////////////////////////////////////////////////////////////////////

// TDFParser class, written for the Spring Engine as a replacement for
// SJ's SunParser class by Tobi. Slightly modified by AF for AI usage.
// Uses the GPL V2 licence

#include <boost/spirit.hpp>
#include <boost/spirit/error_handling.hpp>
#include <boost/scoped_array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/spirit/utility/confix.hpp>

#include "../Core/include.h"

#include "tdf_grammar.hpp"

using boost::spirit::parse;
using boost::spirit::space_p;
using boost::spirit::comment_p;
using boost::spirit::parse_info;

namespace ntai {
	void TdfParser::TdfSection::print( std::ostream & out )  const{
	  for( std::map<std::string,TdfSection*>::const_iterator it = sections.begin(), e=sections.end(); it != e; ++it ) {
		out << "[" << it->first << "] {\n";
		it->second->print(out);
		out << "};";
	  }
	  for( std::map<std::string,std::string>::const_iterator it = values.begin(), e=values.end(); it != e; ++it ) {
		out << it->first  << "=" << it->second << ";\n";
	  }
	}

	TdfParser::TdfSection* TdfParser::TdfSection::construct_subsection( std::string const& name )
	{
	  std::string lowerd_name = name;
		std::transform(lowerd_name.begin(), lowerd_name.end(), lowerd_name.begin(), static_cast<int (*)(int)>(std::tolower));
	  std::map<std::string,TdfSection*>::iterator it = sections.find(lowerd_name);
	  if( it != sections.end() )
		return it->second;
	  else {
		TdfSection* ret = new TdfSection;
		sections[lowerd_name] = ret;
		return ret;
	  }
	}

	void TdfParser::TdfSection::add_name_value(std::string const& name, std::string& value )
	{
	  std::string lowerd_name = name;
		std::transform(lowerd_name.begin(), lowerd_name.end(), lowerd_name.begin(), static_cast<int (*)(int)>(std::tolower));
	  values[lowerd_name] = value;
	}



	TdfParser::~TdfParser()
	{
	}

	TdfParser::TdfSection::~TdfSection()
	{
	  for( std::map<std::string,TdfSection*>::iterator it = sections.begin(), e=sections.end(); it != e; ++it )
		delete it->second;
	}

	TdfParser::TdfParser(Global* G) {
		this->G = G;
	}

	void TdfParser::print( std::ostream & out ) const {
		root_section.print( out );
	}

	TdfParser::TdfParser(Global* G,std::string const& filename){
		this->G = G;
		LoadFile( filename );
	}

	void TdfParser::LoadFile(std::string const& filename){
		this->filename = filename;
		int size = G->cb->GetFileSize(filename.c_str());
		if(size == -1){
			G->L.print("file " + filename + " not found");
			return;
		}
		char* c = new char[size];
		G->cb->ReadFile(filename.c_str(),c,size);
		LoadBuffer(c,(std::size_t)size);
		return;
	}

	void TdfParser::LoadVirtualFile(std::string const& filename){
		this->filename = filename;
		int size = G->cb->GetFileSize(filename.c_str());
		if(size == -1){
			G->L.print("file " + filename + " not found");
			return;
		}
		char* c = new char[size];
		G->cb->ReadFile(filename.c_str(),c,size);
		LoadBuffer(c,(std::size_t)size);
		return;
	}

	TdfParser::TdfParser(Global* G, char const* buf, std::size_t size) {
		this->G = G;
		LoadBuffer( buf, size );
	}

	void TdfParser::LoadBuffer( char const* buf, std::size_t size){
		this->filename = "buffer";
		std::list<std::string> junk_data;
		tdf_grammar grammar( &root_section, &junk_data );

		std::string message;

		boost::spirit::position_iterator2<char const*> error_it( buf, buf + size, filename );

		boost::spirit::parse_info<char const*> info;
		try {
			info = parse(
				buf
				, buf + size
				, grammar
				, space_p
				|  comment_p("/*", "*/")           // rule for C-comments
				|  comment_p("//")
				) ;
		}catch( boost::spirit::parser_error<tdf_grammar::Errors, char *> & e ) {
			// thrown by assertion parsers in tdf_grammar

			switch(e.descriptor) {
				case tdf_grammar::semicolon_expected: message = "semicolon expected"; break;
				case tdf_grammar::equals_sign_expected: message = "equals sign in name value pair expected"; break;
				case tdf_grammar::square_bracket_expected: message = "square bracket to close section name expected"; break;
				case tdf_grammar::brace_expected: message = "brace or further name value pairs expected"; break;
			};


			for( size_t  i = 0;i != size;  ++i,++error_it );

		}


		for(list<string>::const_iterator it = junk_data.begin(), e = junk_data.end(); it !=e ; ++it ){
			std::string temp = boost::trim_copy( *it );
			if( ! temp.empty() ) {
				G->L.eprint("Junk in "+ filename +  " :" + temp);
			}
		}

		if(!message.empty())
		G->L.eprint(message + " in " +filename);
		//throw parse_error( , error_it.get_currentline(), error_it.get_position().line, error_it.get_position().column, filename );
		
		if(!info.full){
			boost::spirit::position_iterator2<char const*> error_it( buf, buf + size, filename );
			for( size_t i = 0; i != size; ++i,++error_it );
			G->L.eprint("error in " +filename);
		//	throw parse_error( error_it.get_currentline(), error_it.get_position().line, error_it.get_position().column, filename );
		}
	}

	//find value, display messagebox if no such value found
	std::string TdfParser::SGetValueMSG(std::string const& location){
	  std::string lowerd = location;
		std::transform(lowerd.begin(), lowerd.end(), lowerd.begin(), static_cast<int (*)(int)>(std::tolower));
		std::string value="";
		bool found = SGetValue(value, lowerd);
		if(!found){
			G->L.print(value);
			return string("");
		}
		return value;
	}

	//find value, return default value if no such value found
	std::string TdfParser::SGetValueDef(std::string const& defaultvalue, std::string const& location){
	  std::string lowerd = location;
		std::transform(lowerd.begin(), lowerd.end(), lowerd.begin(), static_cast<int (*)(int)>(std::tolower));
		std::string value;
		bool found = SGetValue(value, lowerd);
		if(!found){
			value = defaultvalue;
		}
		return value;
	}

	//finds a value in the file, if not found returns false, and errormessages is returned in value
	/*bool TdfParser::GetValue(std::string &value, ...)
	{
		std::string searchpath; //for errormessages
		va_list loc;
		va_start(loc, value);
		int numargs=0;
		while(va_arg(loc, char*)) //determine number of arguments
			numargs++;
		va_start(loc, value);
		TdfSection *sectionptr;
		for(int i=0; i<numargs-1; i++)
		{
			char *arg = va_arg(loc, char*);
			searchpath += '\\';
			searchpath += arg;
			sectionptr = sections[arg];
			if(sectionptr==NULL)
			{
				value = "Section " + searchpath + " missing in file " + filename;
				return false;
			}
		}
		char *arg = va_arg(loc, char*);
		std::string svalue = sectionptr->values[arg];
		searchpath += '\\';
		searchpath += arg;
		if(svalue == "")
		{
			value = "Value " + searchpath + " missing in file " + filename;
			return false;
		}
		value = svalue;
		return true;
	}*/

	/*void TdfParser::Test()
	{
		TdfSection *unitinfo = sections["UNITINFO"];
		TdfSection *weapons = unitinfo->sections["WEAPONS"];
		std::string mo = weapons->values["weapon1"];
	}*/

	//finds a value in the file , if not found returns false, and errormessages is returned in value
	//location of value is sent as a string "section\section\value"
	bool TdfParser::SGetValue(std::string &value, std::string const& location){
		std::string lowerd = location;
		std::transform(lowerd.begin(), lowerd.end(), lowerd.begin(), static_cast<int (*)(int)>(std::tolower));
		std::string searchpath; //for errormessages

		//split the location string
		std::vector<std::string> loclist = GetLocationVector(lowerd);
		if(root_section.sections.find(loclist[0]) == root_section.sections.end()){
			value = "";
			//G->L.print("Section " + loclist[0] + " missing in file " + filename);
			return false;
		}
		TdfSection *sectionptr = root_section.sections[loclist[0]];
		searchpath = loclist[0];
		for(unsigned int i=1; i<loclist.size()-1; i++){
			//const char *arg = loclist[i].c_str();
			searchpath += '\\';
			searchpath += loclist[i];
			if(sectionptr->sections.find(loclist[i]) == sectionptr->sections.end()){
				value = "";
				//G->L.print("Section " + searchpath + " missing in file " + filename);
				return false;
			}
			sectionptr = sectionptr->sections[loclist[i]];
		}
		searchpath += '\\';
		searchpath += loclist[loclist.size()-1];
		if(sectionptr->values.find(loclist[loclist.size()-1]) == sectionptr->values.end()){
			value = "Value " + searchpath + " missing in file " + filename;
			return false;
		}
			std::string svalue = sectionptr->values[loclist[loclist.size()-1]];
		value = svalue;
		return true;
	}

	//return a map with all values in section
	const std::map<std::string, std::string> TdfParser::GetAllValues(std::string const& location){
	  std::string lowerd = location;
		std::transform(lowerd.begin(), lowerd.end(), lowerd.begin(), static_cast<int (*)(int)>(std::tolower));
		std::map<std::string, std::string> emptymap;
		std::string searchpath; //for errormessages
		std::vector<std::string> loclist = GetLocationVector(lowerd);
		if(root_section.sections.find(loclist[0]) == root_section.sections.end()){
	//		handleerror(hWnd, ("Section " + loclist[0] + " missing in file " + filename).c_str(), "Sun parsing error", MBF_OK);
		//	info->AddLine ("Section " + loclist[0] + " missing in file " + filename);
			return emptymap;
		}
		TdfSection *sectionptr = root_section.sections[loclist[0]];
		searchpath = loclist[0];
		for(unsigned int i=1; i<loclist.size(); i++){
			searchpath += '\\';
			searchpath += loclist[i];
			if(sectionptr->sections.find(loclist[i]) == sectionptr->sections.end()){
	//			handleerror(hWnd, ("Section " + searchpath + " missing in file " + filename).c_str(), "Sun parsing error", MBF_OK);
				//info->AddLine ("Section " + searchpath + " missing in file " + filename);
				return emptymap;
			}
			sectionptr = sectionptr->sections[loclist[i]];
		}
		return sectionptr->values;
	}

	//return vector with all section names in it
	std::vector<std::string> TdfParser::GetSectionList(std::string const& location){
	  std::string lowerd = location;
		std::transform(lowerd.begin(), lowerd.end(), lowerd.begin(), static_cast<int (*)(int)>(std::tolower));
		std::vector<std::string> loclist = GetLocationVector(lowerd);
		std::vector<std::string> returnvec;
		std::map<std::string,TdfSection*> *sectionsptr = &root_section.sections;
		if(loclist[0].compare("")!=0)	{
 			std::string searchpath;// = loclist[0];
			for(unsigned int i=0; i<loclist.size(); i++){
				searchpath += loclist[i];
				if(sectionsptr->find(loclist[i]) == sectionsptr->end()){
	//				handleerror(hWnd, ("Section " + searchpath + " missing in file " + filename).c_str(), "Sun parsing error", MBF_OK);
				//	info->AddLine ("Section " + searchpath + " missing in file " + filename);
        			return returnvec;
				}
				sectionsptr = &sectionsptr->find(loclist[i])->second->sections;
        			searchpath += '\\';
			}
		}
		std::map<std::string,TdfSection*>::iterator it;
		for(it=sectionsptr->begin(); it!=sectionsptr->end(); it++){
			returnvec.push_back(it->first);
			std::transform(returnvec.back().begin(), returnvec.back().end(), returnvec.back().begin(), (int (*)(int))std::tolower);
		}
		return returnvec;
	}

	bool TdfParser::SectionExist(std::string const& location){
	  std::string lowerd = location;
		std::transform(lowerd.begin(), lowerd.end(), lowerd.begin(), static_cast<int (*)(int)>(std::tolower));
		std::vector<std::string> loclist = GetLocationVector(lowerd);
		if(root_section.sections.find(loclist[0]) == root_section.sections.end()){
			return false;
		}
		TdfSection *sectionptr = root_section.sections[loclist[0]];
		for(unsigned int i=1; i<loclist.size(); i++){
			if(sectionptr->sections.find(loclist[i]) == sectionptr->sections.end()){
				return false;
			}
			sectionptr = sectionptr->sections[loclist[i]];
		}
		return true;
	}

	std::vector<std::string> TdfParser::GetLocationVector(std::string const& location){
	  std::string lowerd = location;
		std::transform(lowerd.begin(), lowerd.end(), lowerd.begin(), static_cast<int (*)(int)>(std::tolower));
		std::vector<std::string> loclist;
		int start = 0;
		int next = 0;

		while((next = lowerd.find_first_of("\\", start)) != (int)std::string::npos){
			loclist.push_back(lowerd.substr(start, next-start));
			start = next+1;
		}
		loclist.push_back(lowerd.substr(start));

	  return loclist;
	}

	/*
	template<typename T>
	void TdfParser::GetMsg(T& value, const std::string& key)
	{
		std::string str;
		str = SGetValueMSG(key);
		std::stringstream stream;
		stream << str;
		stream >> value;
	}

	template<typename T>
	void TdfParser::GetDef(T& value, const std::string& key, const std::string& defvalue)
	{
		std::string str;
		str = SGetValueDef(key, defvalue);
		std::stringstream stream;
		stream << str;
		stream >> value;
	}*/

	float3 TdfParser::GetFloat3(float3 def, std::string const& location){
		std::string s=SGetValueDef("",location);
		if(s.empty()){
			return def;
		}
		float3 ret;
		ParseArray(s,&ret.x,3);
		return ret;
	}
}
