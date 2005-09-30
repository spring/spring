#ifndef TDF_GRAMMAR_HPP_INCLUDED
#define TDF_GRAMMAR_HPP_INCLUDED 
#include <map>
#include <string>
#include <boost/spirit/core.hpp>
#include <boost/spirit/symbols.hpp>
#include <boost/spirit/attribute.hpp>
#include <boost/spirit/dynamic.hpp>
#include <boost/spirit/phoenix.hpp>
#include <boost/spirit/utility/chset.hpp>
#include <boost/spirit/utility/lists.hpp>
#include <boost/spirit/iterator/file_iterator.hpp>
#include <boost/spirit/utility/grammar_def.hpp>
#include <boost/spirit/iterator/position_iterator.hpp>
#include <boost/spirit/phoenix/binders.hpp>
#include <boost/spirit/error_handling/exceptions.hpp>
#include "StdAfx.h"
#include "TdfParser.h"


struct tdf_grammar : public boost::spirit::grammar<tdf_grammar> {
  enum Errors {
    semicolon_expected
      , equals_sign_expected
      , square_bracket_expected
      , brace_expected 
  };
  typedef std::map<std::string,std::string> map_type;
  typedef map_type::value_type value_t;
  typedef std::pair<map_type::iterator,bool> insert_ret;

  struct section_closure : boost::spirit::closure<section_closure, TdfParser::TdfSection *>{ member1 context; };

  struct string_closure : boost::spirit::closure<string_closure, std::string>{ member1 name; };

  TdfParser::TdfSection *section;
  tdf_grammar( TdfParser::TdfSection* sec ) :section(sec) { }
  template<typename ScannerT>
  struct definition {

    boost::spirit::rule<ScannerT>  tdf;
    boost::spirit::rule<ScannerT, string_closure::context_t> name;
    boost::spirit::rule<ScannerT, section_closure::context_t> section;
    boost::spirit::assertion<Errors> expect_semicolon, expect_equals_sign, expect_square_bracket, expect_brace;
    std::string temp1;
    definition(tdf_grammar const& self) : expect_semicolon(semicolon_expected)
      , expect_equals_sign(equals_sign_expected)
      , expect_square_bracket(square_bracket_expected)
      , expect_brace(brace_expected)  { 
      using namespace boost::spirit;
      using namespace phoenix;
      tdf = 
       *(section(self.section)) // Attribute von oben nach unten reichen 
        ;
      name =  
        (+chset<>("a-zA-Z0-9_+-"))
        [ name.name = construct_<std::string>(arg1, arg2) ] // Attribut nach oben durchreichen
        ;
      section = '[' 
        >> name
        [ section.context = bind( &TdfParser::TdfSection::construct_subsection )(section.context, arg1)  ]
        >> expect_square_bracket( ch_p(']') )
        >> expect_semicolon (ch_p('{') )
        >> *
        (
         (
          name
          [var(temp1) = arg1] 
          >> expect_equals_sign( ch_p('=') )
          >> lexeme_d[ (*~ch_p(';')) // might be empty too?!
          [ bind( &TdfParser::TdfSection::add_name_value)(section.context, var(temp1), construct_<std::string>(arg1,arg2) ) ]
          ]
          >> expect_semicolon( ch_p(';') )
         )
         | section(section.context)
        )
        >> expect_brace( ch_p('}') )
        ;
    }
    boost::spirit::rule<ScannerT> const& start() const { return tdf; }
  };
};

#endif

