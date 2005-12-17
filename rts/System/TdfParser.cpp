// SunParser.cpp: implementation of the TdfParser class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <boost/scoped_array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/spirit/utility/confix.hpp>
#include "TdfParser.h"
#include "tdf_grammar.hpp"
#include "FileSystem/FileHandler.h"
#include "Game/UI/InfoConsole.h"
#include "Platform/errorhandler.h"

using boost::spirit::parse;
using boost::spirit::space_p;
using boost::spirit::comment_p;
using boost::spirit::parse_info;

TdfParser::parse_error::parse_error( std::size_t l, std::size_t c, std::string const& f) throw() 
  : std::runtime_error( "Parse error in " + f + " at line " + boost::lexical_cast<std::string>(l) + " column " + boost::lexical_cast<std::string>(c) +".") 
  , line(l)
  , column(c)
  , filename(f)
{}

TdfParser::parse_error::parse_error( std::string const& line_of_error, std::size_t l, std::size_t c, std::string const& f) throw()
  : std::runtime_error( "Parse error in " + f + " at line " + boost::lexical_cast<std::string>(l) + " column " + boost::lexical_cast<std::string>(c) +" near\n"+ line_of_error) 
  , line(l)
  , column(c)
  , filename(f) 
{}

TdfParser::parse_error::parse_error( std::string const& message, std::string const& line_of_error, std::size_t l, std::size_t c, std::string const& f) 
  throw()
  : std::runtime_error( "Parse error '" + message + "' in " + f + " at line " + boost::lexical_cast<std::string>(l) + " column " + boost::lexical_cast<std::string>(c) +" near\n"+ line_of_error) 
  , line(l)
  , column(c)
  , filename(f) 
{}

TdfParser::parse_error::~parse_error() throw() {}
std::size_t TdfParser::parse_error::get_line() const {return line;}
std::size_t TdfParser::parse_error::get_column() const {return column;}
std::string const& TdfParser::parse_error::get_filename() const {return filename;}

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

TdfParser::TdfParser() {
}
void TdfParser::print( std::ostream & out ) const {
  root_section.print( out );
}
TdfParser::TdfParser(std::string const& filename) 
{
  LoadFile( filename );
}
void TdfParser::LoadFile(std::string const& filename)
{
  this->filename = filename;
	CFileHandler file(filename);
	if(!file.FileExists())
    throw std::runtime_error( ("file " + filename + " not found").c_str() );

	int size = file.FileSize();
  boost::scoped_array<char> filebuf( new char[size+2] );

	file.Read( filebuf.get(), file.FileSize());
  filebuf[size] = '\0'; //append newline at end to avoid parsing error at eof

  tdf_grammar grammar( &root_section );
  boost::spirit::parse_info<char const*> info;
  std::string message; 
  boost::spirit::position_iterator2<char const*> error_it( filebuf.get(), filebuf.get() + size, filename );

  try {
   info = boost::spirit::parse( 
      filebuf.get()
      , filebuf.get() + size
      , grammar
      , space_p 
      |  comment_p("/*", "*/")           // rule for C-comments
      |  comment_p("//")
      ); 
  }
  catch( boost::spirit::parser_error<tdf_grammar::Errors, char *> & e ) { // thrown by assertion parsers in tdf_grammar

    switch(e.descriptor) {
      case tdf_grammar::semicolon_expected: message = "semicolon expected"; break;
      case tdf_grammar::equals_sign_expected: message = "equals sign in name value pair expected"; break;
      case tdf_grammar::square_bracket_expected: message = "square bracket to close section name expected"; break;
      case tdf_grammar::brace_expected: message = "brace or further name value pairs expected"; break;
    };

    for( int i = 0, end = e.where -filebuf.get(); i != end; ++i,++error_it );
  }

  if( ! message.empty() )
    throw parse_error( message, error_it.get_currentline(), error_it.get_position().line, error_it.get_position().column, filename );


  // a different error might have happened:
  if(!info.full)  {
    for( int i = 0, end = info.stop -filebuf.get(); i != end; ++i,++error_it );

    throw parse_error( error_it.get_currentline(), error_it.get_position().line, error_it.get_position().column, filename );
  }

}

TdfParser::TdfParser( char const* buf, std::size_t size) {
  LoadBuffer( buf, size );
}
void TdfParser::LoadBuffer( char const* buf, std::size_t size)
{
  this->filename = "buffer";
  tdf_grammar grammar( &root_section );
  parse_info<char const*> info = parse( 
      buf
      , buf + size
      , grammar
      , space_p 
      |  comment_p("/*", "*/")           // rule for C-comments
      |  comment_p("//")
      ) ; 

  if(!info.full)
  {
    boost::spirit::position_iterator2<char const*> error_it( buf, buf + size, filename );
    error_it += info.stop - buf;
    throw parse_error( error_it.get_currentline(), error_it.get_position().line, error_it.get_position().column, filename );
  }
}

//find value, display messagebox if no such value found
std::string TdfParser::SGetValueMSG(std::string const& location)
{
  std::string lowerd = location;
	std::transform(lowerd.begin(), lowerd.end(), lowerd.begin(), static_cast<int (*)(int)>(std::tolower));
	std::string value;
	bool found = SGetValue(value, lowerd);
	if(!found)
		handleerror(hWnd, value.c_str(), "TDF parsing error", MBF_OK);
	return value;
}

//find value, return default value if no such value found
std::string TdfParser::SGetValueDef(std::string const& defaultvalue, std::string const& location)
{
  std::string lowerd = location;
	std::transform(lowerd.begin(), lowerd.end(), lowerd.begin(), static_cast<int (*)(int)>(std::tolower));
	std::string value;
	bool found = SGetValue(value, lowerd);
	if(!found)
		value = defaultvalue;
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
bool TdfParser::SGetValue(std::string &value, std::string const& location)
{
  std::string lowerd = location;
	std::transform(lowerd.begin(), lowerd.end(), lowerd.begin(), static_cast<int (*)(int)>(std::tolower));
	std::string searchpath; //for errormessages
	//split the location string
	std::vector<std::string> loclist = GetLocationVector(lowerd);
	if(root_section.sections.find(loclist[0]) == root_section.sections.end())
	{
		value = "Section " + loclist[0] + " missing in file " + filename;
		return false;
	}
	TdfSection *sectionptr = root_section.sections[loclist[0]];
	searchpath = loclist[0];
	for(unsigned int i=1; i<loclist.size()-1; i++)
	{
		//const char *arg = loclist[i].c_str();
		searchpath += '\\';
		searchpath += loclist[i];
		if(sectionptr->sections.find(loclist[i]) == sectionptr->sections.end())
		{
			value = "Section " + searchpath + " missing in file " + filename;
			return false;
		}
		sectionptr = sectionptr->sections[loclist[i]];
	}
	searchpath += '\\';
	searchpath += loclist[loclist.size()-1];
	if(sectionptr->values.find(loclist[loclist.size()-1]) == sectionptr->values.end())
	{
		value = "Value " + searchpath + " missing in file " + filename;
		return false;
	}
        std::string svalue = sectionptr->values[loclist[loclist.size()-1]];
	value = svalue;
	return true;
}

//return a map with all values in section
const std::map<std::string, std::string> TdfParser::GetAllValues(std::string const& location)
{
  std::string lowerd = location;
	std::transform(lowerd.begin(), lowerd.end(), lowerd.begin(), static_cast<int (*)(int)>(std::tolower));
	std::map<std::string, std::string> emptymap;
	std::string searchpath; //for errormessages
	std::vector<std::string> loclist = GetLocationVector(lowerd);	
	if(root_section.sections.find(loclist[0]) == root_section.sections.end())
	{
		handleerror(hWnd, ("Section " + loclist[0] + " missing in file " + filename).c_str(), "Sun parsing error", MBF_OK);
		return emptymap;
	}
	TdfSection *sectionptr = root_section.sections[loclist[0]];
	searchpath = loclist[0];
	for(unsigned int i=1; i<loclist.size(); i++)
	{
		searchpath += '\\';
		searchpath += loclist[i];
		if(sectionptr->sections.find(loclist[i]) == sectionptr->sections.end())
		{
			handleerror(hWnd, ("Section " + searchpath + " missing in file " + filename).c_str(), "Sun parsing error", MBF_OK);
			return emptymap;
		}
		sectionptr = sectionptr->sections[loclist[i]];
	}
	return sectionptr->values;
}

//return vector with all section names in it
std::vector<std::string> TdfParser::GetSectionList(std::string const& location)
{
  std::string lowerd = location;
	std::transform(lowerd.begin(), lowerd.end(), lowerd.begin(), static_cast<int (*)(int)>(std::tolower));
	std::vector<std::string> loclist = GetLocationVector(lowerd);
	std::vector<std::string> returnvec;
	std::map<std::string,TdfSection*> *sectionsptr = &root_section.sections;
	if(loclist[0].compare("")!=0)
	{
 		std::string searchpath;// = loclist[0];
		for(unsigned int i=0; i<loclist.size(); i++)
		{
			searchpath += loclist[i];
			if(sectionsptr->find(loclist[i]) == sectionsptr->end())
			{
				handleerror(hWnd, ("Section " + searchpath + " missing in file " + filename).c_str(), "Sun parsing error", MBF_OK);
        			return returnvec;
			}
			sectionsptr = &sectionsptr->find(loclist[i])->second->sections;
        		searchpath += '\\';
		}       
	}
	std::map<std::string,TdfSection*>::iterator it;
	for(it=sectionsptr->begin(); it!=sectionsptr->end(); it++)
	{
		returnvec.push_back(it->first);
		std::transform(returnvec.back().begin(), returnvec.back().end(), returnvec.back().begin(), (int (*)(int))std::tolower);
	}
	return returnvec;
}

bool TdfParser::SectionExist(std::string const& location)
{
  std::string lowerd = location;
	std::transform(lowerd.begin(), lowerd.end(), lowerd.begin(), static_cast<int (*)(int)>(std::tolower));
	std::vector<std::string> loclist = GetLocationVector(lowerd);		
	if(root_section.sections.find(loclist[0]) == root_section.sections.end())
	{
		return false;
	}
	TdfSection *sectionptr = root_section.sections[loclist[0]];
	for(unsigned int i=1; i<loclist.size(); i++)
	{
		if(sectionptr->sections.find(loclist[i]) == sectionptr->sections.end())
		{
			return false;
		}
		sectionptr = sectionptr->sections[loclist[i]];
	}
	return true;
}

std::vector<std::string> TdfParser::GetLocationVector(std::string const& location)
{
  std::string lowerd = location;
	std::transform(lowerd.begin(), lowerd.end(), lowerd.begin(), static_cast<int (*)(int)>(std::tolower));
	std::vector<std::string> loclist;
	int start = 0;
	int next = 0;

	while((next = lowerd.find_first_of("\\", start)) != std::string::npos)
	{
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

float3 TdfParser::GetFloat3(float3 def, std::string const& location)
{
	std::string s=SGetValueDef("",location);
	if(s.empty())
		return def;
	float3 ret;
	ParseArray(s,ret.xyz,3);
	return ret;
}

