/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Future.h"

// This is part of the hack which allows the use of std::future
// the code below is copied directly from GNU ISO C++ Library
// it is available under the following license:


#if defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS)
// Copyright (C) 2003-2014 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.
namespace std {
  __future_base::_Result_base::_Result_base() = default;

  __future_base::_Result_base::~_Result_base() = default;

#if defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ < 9)
  __future_base::_State_base::~_State_base() = default;
#endif
}

#endif
