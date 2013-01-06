/* Copyright (c) 2007 John W Wilkinson

   This source code can be used for any purpose as long as
   this comment is retained. */

// json spirit version 2.01

#include "json_spirit_writer.h"
#include "json_spirit_value.h"

#include <cassert>
#include <sstream>
#include <iomanip>

using namespace json_spirit;
using namespace std;

namespace
{
    // this class solely allows its inner classes and methods to
    // be conveniently templatised by Value type and dependent types
    //
    template< class Value_t >
    struct Writer
    {
        typedef typename Value_t::String_type     String_t;
        typedef typename Value_t::Object          Object_t;
        typedef typename Value_t::Array           Array_t;
        typedef typename String_t::value_type     Char_t;
        typedef typename String_t::const_iterator Iter_t;
        typedef Pair_impl< String_t >             Pair_t;
        typedef std::basic_ostream< Char_t >      Ostream_t;

        // this class generates the JSON text,
        // it keeps track of the indentation level etc.
        //
        class Generator
        {
        public:

            Generator( const Value_t& value, Ostream_t& os, bool pretty )
            :   os_( os )
            ,   indentation_level_( 0 )
            ,   pretty_( pretty )
            {
                output( value );
            }

        private:

            void output( const Value_t& value )
            {
                switch( value.type() )
                {
                    case obj_type:   output( value.get_obj() );   break;
                    case array_type: output( value.get_array() ); break;
                    case str_type:   output( value.get_str() );   break;
                    case bool_type:  output( value.get_bool() );  break;
                    case int_type:   os_ << value.get_int();      break;
                    case real_type:  os_ << showpoint << setprecision( 16 ) 
                                         << value.get_real();     break;
                    case null_type:  os_ << "null";               break;
                    default: assert( false );
                }
            }

            void output( const Object_t& obj )
            {
                output_array_or_obj( obj, '{', '}' );
            }

            void output( const Array_t& arr )
            {
                output_array_or_obj( arr, '[', ']' );
            }

            void output( const Pair_t& pair )
            {
                output( pair.name_ ); space(); os_ << ':'; space(); output( pair.value_ );
            }

            void output( const String_t& s )
            {
                os_ << '"' << add_esc_chars( s ) << '"';
            }

            void output( bool b )
            {
                os_ << to_str( b ? "true" : "false" );
            }

            template< class T >
            void output_array_or_obj( const T& t, Char_t start_char, Char_t end_char )
            {
                os_ << start_char; new_line();

                ++indentation_level_;
                
                for( typename T::const_iterator i = t.begin(); i != t.end(); ++i )
                {
                    indent(); output( *i );

                    if( i != t.end() - 1 )
                    {
                        os_ << ',';
                    }

                    new_line();
                }

                --indentation_level_;

                indent(); os_ << end_char;
            }
            
            void indent()
            {
                if( !pretty_ ) return;

                for( int i = 0; i < indentation_level_; ++i )
                { 
                    os_ << "    ";
                }
            }

            void space()
            {
                if( pretty_ ) os_ << ' ';
            }

            void new_line()
            {
                if( pretty_ ) os_ << '\n';
            }

            String_t to_str( const char* c_str )
            {
                return ::to_str< String_t >( c_str );
            }

            Char_t to_hex( Char_t c )
            {
                assert( c <= 0xF );

                if( c < 10 ) return '0' + c;

                return 'A' + c - 10;
            }

            String_t non_printable_to_string( Char_t c )
            {
                String_t result( to_str( "\\u" ) );

                result.resize( 6, '0' );

                for( int i = 0; i < 4; ++i  )
                {
                    result[ 5 - i ] = to_hex( c % 16 );

                    c >>= 4;
                }

                return result;
            }

            bool add_esc_char( Char_t c, String_t& s )
            {
                switch( c )
                {
                    case '"':  s += to_str( "\\\"" ); return true;
                    case '\\': s += to_str( "\\\\" ); return true;
                    case '\b': s += to_str( "\\b"  ); return true;
                    case '\f': s += to_str( "\\f"  ); return true;
                    case '\n': s += to_str( "\\n"  ); return true;
                    case '\r': s += to_str( "\\r"  ); return true;
                    case '\t': s += to_str( "\\t"  ); return true;
                }

                return false;
            }

            String_t add_esc_chars( const String_t& s )
            {
                String_t result;

                const Iter_t end( s.end() );

                for( Iter_t i = s.begin(); i != end; ++i )
                {
                    const Char_t c( *i );

                    if( add_esc_char( c, result ) ) continue;

                    if( iswprint( c ) )
                    {
                        result += c;
                    }
                    else
                    {
                        result += non_printable_to_string( c );
                    }
                }

                return result;
            }

            Ostream_t& os_;
            int indentation_level_;
            bool pretty_;
        };

        static void write( const Value_t& value, Ostream_t& os, bool pretty  )
        {
            Generator( value, os, pretty );
        }

        static String_t write( const Value_t& value, bool pretty )
        {
            basic_ostringstream< Char_t > os;

            write( value, os, pretty );

            return os.str();
        }
    };
}

void json_spirit::write( const Value& value, std::ostream& os )
{
    Writer< Value >::write( value, os, false );
}

void json_spirit::write_formatted( const Value& value, std::ostream& os )
{
    Writer< Value >::write( value, os, true );
}

std::string json_spirit::write( const Value& value )
{
    return Writer< Value >::write( value, false );
}

std::string json_spirit::write_formatted( const Value& value )
{
    return Writer< Value >::write( value, true );
}

#ifndef BOOST_NO_STD_WSTRING

void json_spirit::write( const wValue& value, std::wostream& os )
{
    Writer< wValue >::write( value, os, false );
}

void json_spirit::write_formatted( const wValue& value, std::wostream& os )
{
    Writer< wValue >::write( value, os, true );
}
std::wstring json_spirit::write( const wValue&  value )
{
    return Writer< wValue >::write( value, false );
}

std::wstring json_spirit::write_formatted( const wValue&  value )
{
    return Writer< wValue >::write( value, true );
}

#endif
