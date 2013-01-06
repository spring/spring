#ifndef JASON_SPIRIT_VALUE
#define JASON_SPIRIT_VALUE

/* Copyright (c) 2007 John W Wilkinson

   This source code can be used for any purpose as long as
   this comment is retained. */

// json spirit version 2.01

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <boost/config.hpp> 
#include <vector>
#include <string>
#include <cassert>

namespace json_spirit
{
    enum Value_type{ obj_type, array_type, str_type, bool_type, int_type, real_type, null_type };

    template< class String > struct Pair_impl;

    template< class String >        // eg std::string or std::wstring
    class Value_impl
    {
    public:

        typedef String                              String_type;
        typedef Pair_impl< String >                 Pair;
        typedef std::vector< Pair >                 Object;
        typedef std::vector< Value_impl< String > > Array;

        Value_impl();  // creates null value
        Value_impl( const typename String::pointer value ); // eg char*
        Value_impl( const String&                  value );
        Value_impl( const Object&                  value );
        Value_impl( const Array&                   value );
        Value_impl( bool                           value );
        Value_impl( int                            value );
        Value_impl( double                         value );

        bool operator==( const Value_impl& lhs ) const;

        Value_type type() const;

        const String& get_str()   const;
        const Object& get_obj()   const;
        const Array&  get_array() const;
        bool          get_bool()  const;
        int           get_int()   const;
        double        get_real()  const;

        Object& get_obj();
        Array&  get_array();

        static const Value_impl null;

    private:

        Value_type type_;

        String str_;
        Object obj_;
        Array array_;
        bool bool_;
        int i_;
        double d_;
    };

    template< class String >
    struct Pair_impl
    {
        Pair_impl( const String& name, const Value_impl< String >&  value );

        bool operator==( const Pair_impl< String >& lhs ) const;

        String name_;
        Value_impl< String > value_;
    };

    // typedefs for ASCII, compatible with JSON Spirit v1.02

    typedef Value_impl< std::string > Value;
    typedef Pair_impl < std::string > Pair;
    typedef Value::Object Object;
    typedef Value::Array Array;

    // typedefs for Unicode, new for JSON Spirit v2.00

#ifndef BOOST_NO_STD_WSTRING
    typedef Value_impl< std::wstring > wValue;
    typedef Pair_impl < std::wstring > wPair;
    typedef wValue::Object wObject;
    typedef wValue::Array wArray;
#endif

    ///////////////////////////////////////////////////////////////////////////////////////////////
    //
    // implementation

    template< class String >
    const Value_impl< String > Value_impl< String >::null;

    template< class String >
    Value_impl< String >::Value_impl()
    :   type_( null_type )
    {
    }

    template< class String >
    Value_impl< String >::Value_impl( const typename String::pointer value )
    :   type_( str_type )
    ,   str_( value )
    {
    }

    template< class String >
    Value_impl< String >::Value_impl( const String& value )
    :   type_( str_type )
    ,   str_( value )
    {
    }

    template< class String >
    Value_impl< String >::Value_impl( const Object& value )
    :   type_( obj_type )
    ,   obj_( value )
    {
    }

    template< class String >
    Value_impl< String >::Value_impl( const Array& value )
    :   type_( array_type )
    ,   array_( value )
    {
    }

    template< class String >
    Value_impl< String >::Value_impl( bool value )
    :   type_( bool_type )
    ,   bool_( value )
    {
    }

    template< class String >
    Value_impl< String >::Value_impl( int value )
    :   type_( int_type )
    ,   i_( value )
    {
    }

    template< class String >
    Value_impl< String >::Value_impl( double value )
    :   type_( real_type )
    ,   d_( value )
    {
    }

    template< class String >
    bool Value_impl< String >::operator==( const Value_impl& lhs ) const
    {
        if( this == &lhs ) return true;

        if( type() != lhs.type() ) return false;

        switch( type_ )
        {
            case str_type:   return get_str()   == lhs.get_str();
            case obj_type:   return get_obj()   == lhs.get_obj();
            case array_type: return get_array() == lhs.get_array();
            case bool_type:  return get_bool()  == lhs.get_bool();
            case int_type:   return get_int()   == lhs.get_int();
            case real_type:  return get_real()  == lhs.get_real();
            case null_type:  return true;
        };

        assert( false );

        return false; 
    }

    template< class String >
    Value_type Value_impl< String >::type() const
    {
        return type_;
    }

    template< class String >
    const String& Value_impl< String >::get_str() const
    {
        assert( type() == str_type );

        return str_;
    }

    template< class String >
    const typename Value_impl< String >::Object& Value_impl< String >::get_obj() const
    {
        assert( type() == obj_type );

        return obj_;
    }
     
    template< class String >
    const typename Value_impl< String >::Array& Value_impl< String >::get_array() const
    {
        assert( type() == array_type );

        return array_;
    }
     
    template< class String >
    bool Value_impl< String >::get_bool() const
    {
        assert( type() == bool_type );

        return bool_;
    }
     
    template< class String >
    int Value_impl< String >::get_int() const
    {
        assert( type() == int_type );

        return i_;
    }

    template< class String >
    double Value_impl< String >::get_real() const
    {
        assert( type() == real_type );

        return d_;
    }

    template< class String >
    typename Value_impl< String >::Object& Value_impl< String >::get_obj()
    {
        assert( type() == obj_type );

        return obj_;
    }

    template< class String >
    typename Value_impl< String >::Array& Value_impl< String >::get_array()
    {
        assert( type() == array_type );

        return array_;
    }

    template< class String >
    Pair_impl< String >::Pair_impl( const String& name, const Value_impl< String >& value )
    :   name_( name )
    ,   value_( value )
    {
    }

    template< class String >
    bool Pair_impl< String >::operator==( const Pair_impl& lhs ) const
    {
        if( this == &lhs ) return true;

        return ( name_ == lhs.name_ ) && ( value_ == lhs.value_ );
    }

    // converts a C string, ie. 8 bit char array, to a string object
    //
    template < class String_t >
    String_t to_str( const char* c_str )
    {
        String_t result;

        for( const char* p = c_str; *p != 0; ++p )
        {
            result += *p;
        }

        return result;
    }
}

#endif
