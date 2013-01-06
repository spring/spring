#ifndef JASON_SPIRIT_READER
#define JASON_SPIRIT_READER

/* Copyright (c) 2007 John W Wilkinson

   json spirit version 2.01

   This source code can be used for any purpose as long as
   this comment is retained. */

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "json_spirit_value.h"
#include <iostream>

namespace json_spirit
{
    // converts text to a JSON value that will be either a JSON object or array
    //
    bool read( const std::string&  s, Value&  value );
    bool read( std::istream&  is,     Value&  value );

#ifndef BOOST_NO_STD_WSTRING
    bool read( const std::wstring& s, wValue& value );
    bool read( std::wistream& is,     wValue& value );
#endif
}

#endif
