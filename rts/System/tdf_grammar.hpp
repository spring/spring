#ifndef TDF_GRAMMAR_HPP_INCLUDED
#define TDF_GRAMMAR_HPP_INCLUDED 
#include <map>
#include <list>
#include <iostream>
#include <string>

#include <boost/version.hpp>

#if BOOST_VERSION > 103500
#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/phoenix1_binders.hpp>
using namespace boost::spirit::classic;
#else
#include <boost/spirit.hpp>
#include <boost/spirit/phoenix/binders.hpp>
using namespace boost::spirit;
#endif

#include "TdfParser.h"


/**
 * \brief Simple std::ostream Actor, with fixed prefix or suffix. 
 * This actor prints the item parsed using the ostream, enclosed
 * by suffix and prefix string. 
 */
class ostream_actor
{
private:
	std::ostream & ref;
	std::string prefix, suffix;

public:
	ostream_actor( std::ostream & ref_, std::string const& addition = "" ) 
		: ref( ref_ ), suffix(addition) {};
	ostream_actor( std::string prefix, std::ostream & ref_, std::string const& addition = "" ) 
		: ref( ref_ ), prefix(prefix), suffix(addition) {};

	template<typename T2> void operator()(T2 const& val ) const { ref << prefix << val << suffix; }

	template<typename IteratorT>
	void operator()( IteratorT const& first, IteratorT const& last ) const { ref << prefix << std::string(first, last) << suffix; }
};

/**
 * \name Actor functions
 * uese these two functions to create a semantic action with ostream_actor. 
 * \{
 */
inline ostream_actor ostream_a( std::string const& prefix, std::ostream & ref, std::string const& suffix ="" ) 
{ return ostream_actor(prefix, ref, suffix); }
inline ostream_actor ostream_a( std::ostream & ref, std::string const& suffix= ""  ) 
{ return ostream_actor(ref, suffix ); }
/**
 * \}
 */


struct tdf_grammar : public grammar<tdf_grammar>
{
	enum Errors {
		semicolon_expected
		, equals_sign_expected
		, square_bracket_expected
		, brace_expected 
	};

	struct section_closure : closure<section_closure, TdfParser::TdfSection *>{ member1 context; };

	struct string_closure : closure<string_closure, std::string>{ member1 name; };

	TdfParser::TdfSection *section;
	mutable std::list<std::string>  *junk;
	tdf_grammar( TdfParser::TdfSection* sec, std::list<std::string> * junk_data )
	: section(sec), junk(junk_data)
	{ }
	template<typename ScannerT>
	struct definition
	{

		rule<ScannerT>  tdf, gather_junk_line;
		rule<ScannerT, string_closure::context_t> name;
		rule<ScannerT, section_closure::context_t> section;

		assertion<Errors> expect_semicolon, expect_equals_sign, expect_square_bracket, expect_brace;

		std::string temp1;

		definition(tdf_grammar const& self) 
			: expect_semicolon(semicolon_expected)
			, expect_equals_sign(equals_sign_expected)
			, expect_square_bracket(square_bracket_expected)
			, expect_brace(brace_expected)
		{
			using namespace boost::spirit;
			using namespace phoenix;
			tdf = *( 
				section(self.section) 
				| gather_junk_line  // if this rule gets hit then section did not consume everything,
				) 
				>> end_p
			;

			gather_junk_line = 
				lexeme_d[
				(+(~chset<>("}[\n")))
				[ push_back_a( * self.junk  ) ]
				// [ ostream_a( "Junk detected:", std::cout, "\n" ) ] // debug printouts
				]
			;

			name = (+(~chset<>(";{[]}=\n"))) // allows pretty much everything that isnt already used
			[ name.name = construct_<std::string>(arg1, arg2) ] 
			;

			section = '[' 
				>> name
				// [ ostream_a( "Just parsed a section: ", std::cout, "\n") ] // prints section name
				[ section.context = bind( &TdfParser::TdfSection::construct_subsection )(section.context, arg1)  ]
				>> expect_square_bracket( ch_p(']') )
				>> expect_brace (ch_p('{') )
				>> *
				(
					(
					name
					[var(temp1) = arg1] 
					>> ch_p('=') // turn this into expect_equals_sign( ch_p('=') ) if you want more strict parsing
					>> lexeme_d[ (*~ch_p(';')) // might be empty too!
					[ bind( &TdfParser::TdfSection::add_name_value)(section.context, var(temp1), construct_<std::string>(arg1,arg2) ) ]
					]
					>> expect_semicolon( ch_p(';') )
					)
					| section(section.context)
					| gather_junk_line  // if this rule gets touched we either hit a closing section } or there really is junk
				)
				>> expect_brace( ch_p('}') )
			;
		}
		rule<ScannerT> const& start() const { return tdf; }
	};
};

#endif


