////////////////////////////////////////////////////////////////////////////////////
//
// InitUtil.h:
//		Utility templates to aid in the initialization of, and assignment to,
//		standard/extended containers of strings, pointers, numeric primitives (or
//		any scalar types for which extractors are defined).
//
// Version v0.9293 March 26, 2004 (for change log, see initutil92-log.txt)
// Web Page: www.bdsoft.com/tools/InitUtil.html
//
//		Checked out under the following platforms:
//			MSVC 7.1 (Stock, Dinkum Unabridged libraries)
//			Comeau C++ 4.3x
//			Metrowerks CodeWarrior 7/8
//			gcc 3.2 (except for hashed containers, for some reason)
//
// (C) Copyright Leor Zolman 2002. Permission to copy, use, modify, sell and
// distribute this software is granted provided this copyright notice appears
// in all copies. This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

#if !defined(__INITUTIL_H)
#define __INITUTIL_H


// If you define the symbol HASHED_CONTAINERS_SUPPORTED in your source file before
// including this header, your value (0 or 1) will be used. Else, it assumes your
// library DOES support hashed containers (hash_set/_multiset/_map/_multimap).

#if !defined(HASHED_CONTAINERS_SUPPORTED)
#define HASHED_CONTAINERS_SUPPORTED 0			// 0 if you'll never support them
#endif

//
////////////////////////////////////////////////////////////////////////////////////
//
//
// Summary of function templates provided at top-level namespace bds:
// ---------------------------------------------------------------------------
//
//								// (this form not supported for map variants)
//	Container make_cont(const std::string &intext,
//						bool stripLeadingSpaces = true, char delim = ',');
//
//								// (works for all containers, including map variants)
//	Container make_cont(const Container &, const std::string &intext,
//						bool stripLeadingSpaces = true, char delim = ',');
//
//	Container &set_cont(Container &cont , const std::string &intext,
//						bool stripLeadingSpaces = true, char delim = ',');
//
//	Container &app_cont(Container &m, const std::string &intext,
//						bool stripLeadingSpaces = true, char delim1 = ',');
//
//	Container make_cont(int size,
//						Container::value_type val = 1,
//						Container::value_type incr = 1);
//
//	Container make_cont(const Container &, int size,
//						Container::value_type val = 1,
//						Container::value_type incr = 1);
//
//	Container &set_cont(Container &c, int size,
//						Container::value_type val = 1,
//						Container::value_type incr = 1);
//
//	Container &app_cont(Container &c, int size,
//						Container::value_type val = 1,
//						Container::value_type incr = 1);
//	
//	Container make_cont_p(pointer1, pointer2, ... , NULL);
//
//	Container make_cont_p(const Container &c, pointer1, pointer2, ... , NULL);
//
//	Container &set_cont_p(Container &c, pointer1, pointer2, ..., NULL);
// 
//	Container &app_cont_p(Container &c, pointer1, pointer2, ..., NULL);
// 
//
// "Make" vs. "Set"/"App" Functions:
// ---------------------------
//
// The "make_" functions create a new container and return it by value.
//
// The "set_" functions operate upon an existing container, clearing it first, and
//		return a reference to that container.
//
// The "app_" functions append to existing contents of a container, returning
//		a reference to that container.
//
//
// Specifying the Container Type
// -----------------------------
//
// If the first argument to a "make_" function is NOT a container, then the in-
//		stantiation of that template must explicitly specify the container type
//		(this form is not supported for map/multimap/hash_map/hash_multimap).
//
// If the first argument to a "make_" function is a container, the only thing of
//		interest about that container is its type (a temporary container of that
//		type is returned). The parameter itself is not modified. These versions
//		are provided as an alternative to the version above requiring explicit
//		type specification. 
//
//
// String Scanning, Value Scanning, Pointer List and Sequence Generating Functions
// -------------------------------------------------------------------------------
//
// Functions taking a "intext" string parameter:
//
//      If the container to be returned or set has a value_type of string, the
//		"intext" string parameter is scanned for a delimited set of string
//		values (the delimiter defaults to ','), optionally stripping leading spaces.
//
//      If the container to be returned or set has any value_type *other* than
//		string, the string argument "intext" is scanned for a delimited set
//		of values, and those value are extracted into the container using the
//		value_type's native operator>> (and the boolean argument
//		"stripLeadingSpaces" is ignored).
//
//		For maps and multimaps (including hash_map and hash_multimap), each value is
//		expected to be of the form "(key, value)". The delimter between the key and
//		the value may be specified; the delimters around the pair may be either (),
//		[] or {}. The comma *between *pairs* is optional. 
//
//		NOTE: The "stripLeadingSpaces" argument for maps/multimaps/etc. is currently
//		ignored. For these container types, normal extraction (where leading whitespace
//		is ignored, and the first trailing whitespace terminates the scan) is the only
//		flavor available. Custom string extraction may be achieved by creating your own
//		string-like class that overloads extraction (>>) to behave in the desired
//		manner.
//		
//		I hope to have this fixed in an update soon. But first, someone has to ex-
//		plain to me how to *really* preserve all whitespace in a string extraction...
//
// Functions operating on containers of pointers (*_cont_p):
//
//		These functions take a NULL terminated, variable-length list of pointer
//		values (The container's value_type must match the pointer type).
//
// Functions taking size, initial value and increment:
//
//		The remaining functions create a sequence of values to put into the
//		container. The size is required; the last two arguments are of
//		the same template type and default to 1 (so they must be of a type
//		convertible from int); they specify the starting value and the
//		increment for the sequence to be generated.
// 
//

#include <iostream>
#include <iterator>
#include <algorithm>
#include <set>
#include <map>
#include <sstream>
#include <cctype>
#include <cstdarg>

// Figure out the right namespace name for hashed containers:

#if !defined(HASH_NS) && HASHED_CONTAINERS_SUPPORTED
#	include <hash_set>
#	include <hash_map>
#	if defined(__GNUC__) && (__GNUC__ == 3)
#		define HASH_NS __gnu_cxx
#	elif defined(__MWERKS__)
#		define HASH_NS Metrowerks
#	elif defined(_MSC_VER) && (_MSC_VER >= 1310)
#		if defined(_C99)		// Dinkum Unabridged Library
#			define HASH_NS std
#		else
#			define HASH_NS stdext
#		endif
#	else
#		define HASH_NS std
#	endif
#endif		// HASHED_CONTAINERS_SUPPORTED

namespace bds {

  namespace helpers {

	///////////////////////////////////////////////////////////////////////////////
	//																			 //
	//      Helper Templates (used by the "public" templates in later sections)  //
	//																			 //
	///////////////////////////////////////////////////////////////////////////////


	//
	// Appender helper functor class for "appending" a value to a container in
	// the appropriate way. The default is to use push_back(); specializations
	// for set and multiset use insert():
	//

	// General Appender template:
	template<typename Container>
	class Appender_
	{
	public:
		Appender_(Container &c) : cont_(c)
		{}

		inline void operator()(const typename Container::value_type &value)
		{
			cont_.push_back(value);
		}

	private:
		Container &cont_;
	};


	// Specialization of Appender for sets:

	template<typename T>
	class Appender_<std::set<T> >
	{
	public:
		Appender_(std::set<T> &c) : cont_(c)
		{}

		inline void operator()(const T &value)
		{
			cont_.insert(value);
		}

	private:
		std::set<T> &cont_;
	};

	// Specialization of Appender for multisets:
	template<typename T>
	class Appender_<std::multiset<T> >
	{
	public:
		Appender_(std::multiset<T> &c) : cont_(c)
		{}

		inline void operator()(const T &value)
		{
			cont_.insert(value);
		}

	private:
		std::multiset<T> &cont_;
	};


#if HASHED_CONTAINERS_SUPPORTED
	
	// Specialization of Appender for hash_sets:

	template<typename T>
	class Appender_<HASH_NS::hash_set<T> >
	{
	public:
		Appender_(HASH_NS::hash_set<T> &c) : cont_(c)
		{}

		inline void operator()(const T &value)
		{
			cont_.insert(value);
		}

	private:
		HASH_NS::hash_set<T> &cont_;
	};

	// Specialization of Appender for hash_multisets:
	template<typename T>
	class Appender_<HASH_NS::hash_multiset<T> >
	{
	public:
		Appender_(HASH_NS::hash_multiset<T> &c) : cont_(c)
		{}

		inline void operator()(const T &value)
		{
			cont_.insert(value);
		}

	private:
		HASH_NS::hash_multiset<T> &cont_;
	};

#endif		// HASHED_CONTAINERS_SUPPORTED


	//
	// Container &make_cont_(const T &, Container &c,
	//								const string &intext,
	//								bool, char delim);
	//
	//		General helper template function used by templates below for filling
	//		containers (except map/multimap) of (non-string) values from a delimited
	//		list (one string).
	//		[Note: the bool parameter is ignored]
	//

	template<typename Container, typename T>
	Container &make_cont_(const T &, Container &c,
								const std::string &intext, bool,
								char delim)
	{
		using std::string;
		using std::isspace;
		string::const_iterator nextDelim;
		typename Container::value_type v;
							// create appropriate append functor
		Appender_<Container> app(c);

		string::const_iterator curPos = intext.begin(), end = intext.end();
		while (curPos < end)
		{
			nextDelim = std::find(curPos, end, delim);
			while (isspace(*curPos))		// ignore leading space
				++curPos;					// on each string

			string source(curPos, nextDelim);
			std::istringstream is(source);
			is >> v;
			app(v);

			curPos = ++nextDelim;
		}
		return c;
	}


	//
	// void make_cont_(const string &, Container &c, const string &intext,
	//						 bool stripLeadingSpaces, char delim);
	//
	//		Specialization of make_cont_ for string value_type-ed
	//		containers. Fills containers of *strings* from a delimited
	//		list (one string):
	//

	template<typename Container>
	Container &make_cont_(const std::string &, Container &c, const std::string &intext,
								bool stripLeadingSpaces, char delim)
	{
		using std::string;
		using std::isspace;
		string::const_iterator nextDelim;

							// create appropriate append functor
		Appender_<Container> app(c);

		string::const_iterator curPos = intext.begin(), end = intext.end();
		while (curPos < end)
		{
			nextDelim = std::find(curPos, end, delim);
			if (stripLeadingSpaces)
				while (isspace(*curPos))		// ignore leading space
					++curPos;					// on each string

			app(string(curPos, nextDelim));

			curPos = ++nextDelim;
		}

		return c;
	}


	//
	// Container &make_cont_(Container &c,
	//								const string &intext,
	//								bool stripLeadingSpaces, char delim);
	//
	//		map/multimap/hash_map/hash_multimap-specific helper function used by
	//		templates below for filling maps and multimaps from a delimited list (one
	//		string containing pairs).
	//		To group a pair, use () [] or {} (they may be intermixed, but each pair must use
	//			a matching set)
	//
	//		stripLeadingSpaces only applies to the value of a (key, value) pair (spaces following
	//			the opening pair delimiter are always retained for string values)
	//		[NOTE: stripLeadingSpaces is currently ignored.]
	//	
	//		delim is the delimiter between the key and the value in each pair.
	//

	template<typename Container, typename K, typename V>
	Container &make_cont_(Container &c,
							const std::string &intext,
							bool, char delim)
	{
		using std::string;
		using std::isspace;
		string::const_iterator nextDelim;
		K k;
		V v;
		char closeDelim;

		string::const_iterator curPos = intext.begin(), end = intext.end();
		while (true)
		{
			while (curPos != end && isspace(*curPos))
				++curPos;
			
			if (curPos == end)								// no more data
				break;

			if (*curPos == '(')						// make sure we see opening delim
				closeDelim = ')';
			else if (*curPos == '[')
				closeDelim = ']';
			else if (*curPos == '{')
				closeDelim = '}';
			else
			{
				std::cerr << "InitUtil error: Missing '(', '[' or '{' at:\n\t"
									<< string(curPos,end) << std::endl;
				return c;
			}
			
			++curPos;										// advance past '('

			if (curPos == end)
			{
				std::cerr << "InitUtil error: Last pair incomplete in line:\n\t" <<
						string(intext.begin(), end) << std::endl;
				return c;
			}

			if ((nextDelim = std::find(curPos, end, delim)) == end)
			{
				std::cerr << "InitUtil error: '" << delim << "' missing at:\n\t" <<
							 string(curPos, end) << std::endl;
				return c;
			}
			
			if (std::find(curPos, end, closeDelim) < nextDelim)
			{
				std::cerr << "InitUtil error: '" << closeDelim << "' precedes '" << delim
							<< "' at:\n\t" << string(curPos, end) << std::endl;
				return c;
			}
				

			string source(curPos, nextDelim);
			std::istringstream is(source);

			is >> k;

			curPos = ++nextDelim;				// advance past intra-pair delimiter

			if ((nextDelim = std::find(curPos, end, closeDelim)) == end)
			{
				std::cerr << "InitUtil error: Trailing '" << closeDelim <<
						"' on pair missing at:\n\t" << string(curPos, end) << std::endl;
				return c;
			}

			source.assign(curPos, nextDelim);
			std::istringstream is2(source);

			is2 >> v;

			c.insert(typename Container::value_type(k,v));
			curPos = ++nextDelim;				// advance past intra-pair delimiter

			while (curPos != end && (isspace(*curPos) || *curPos == ','))
				++curPos;						// ignore spaces and (optional) inter-pair commas
			if (curPos != end && *curPos == ',')
				++curPos;						// and go past it to next pair...
		}
		return c;
	}


	//
	//	Container &make_cont_(Container &c, int size,
	//								Container::value_type val,
	//								Container::value_type incr);
	//
	//		Helper function used by sequence generation templates
	//		below for filling  a container of values with a numeric sequence.
	//		This version takes a size, initial value, and optional increment:
	//

	template<typename Container>
	Container &make_cont_(Container &c, int size,
								typename Container::value_type val,
								typename Container::value_type incr)
	{
							// create appropriate append functor
		Appender_<Container> app(c);

		for (int i = 1; i <= size; i++)
		{
			app(val);
			val += incr;
		}
		return c;
	}

  }		// end namespace helpers


	///////////////////////////////////////////////////////////////////////////////
	//																			 //
	//      					The "Public" Functions:							 //
	//																			 //
	///////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////
	//																			 //
	// Container initialization/generation templates given string-encoded data:	 //
	//																			 //
	///////////////////////////////////////////////////////////////////////////////



	//
	//	Container make_cont(const string &intext, 
	//						bool stripLeadingSpaces = true, char delim = ',');
	//
	//		Template to generate container with given values
	//		(optionally skipping leading spaces for string values).
	//		[Explicit container specialization required]
	//		[For containers of non-string value_types, the bool arg is
	//		ignored.]
	//
	//	Example: vector<string> vs = make_cont<vector<string> >("this, is, a, test");
	//

	template<typename Container>
	inline Container make_cont(const std::string &intext, 
							   bool stripLeadingSpaces = true, char delim = ',')
	{
		Container c;
		return helpers::make_cont_<Container>
					(typename Container::value_type(), c, intext, stripLeadingSpaces, delim);
	}


	//
	//	Container make_cont(const Container &, const string &intext,
	//						bool stripLeadingSpaces = true, char delim = ',');
	//
	//		Template to generate container with given string values
	//		(optionally skipping leading spaces for string values).
	//		First arg is an instance of the container type to build, but it is
	//			NOT modified (used to convey type information only).
	//		[For containers of non-string value_types, the bool arg is
	//		ignored.]
	//
	//	Example: vector<string> vs = make_cont(vs, "this, is, a, test");
	//

	template<typename Container>
	inline Container make_cont(const Container &, const std::string &intext,
							bool stripLeadingSpaces = true, char delim = ',')
	{
		Container c;
		return helpers::make_cont_<Container>
					(typename Container::value_type(), c, intext, stripLeadingSpaces, delim);
	}


	//
	//	Container make_cont(const Cont &, const string &intext,
	//						bool stripLeadingSpaces = true,
	//						char delim);
	//
	//		Templates to generate map/multimap with given string pair values
	//		(optionally skipping leading spaces for keys and values)
	//		First arg is an instance of the container type to build, but it is
	//			NOT modified (used to convey type information only).
	//
	//	Note: stripLeadingSpaces arguments are currently ignored.
	//
	//	Example: map<string, int> m = make_cont(m, "(name1, 10), (name2, 20), (name3, 30)");
	//

	template<typename K, typename V>
	inline std::map<K,V> make_cont(const std::map<K,V> &, const std::string &intext,
							bool stripLeadingSpaces = true,
							char delim = ',')
	{
		typedef std::map<K,V> maptype;

		maptype c;
		return helpers::make_cont_<maptype,K,V>(c, intext, stripLeadingSpaces, delim);
	}

	template<typename K, typename V>
	inline std::multimap<K,V> make_cont(const std::multimap<K,V> &, const std::string &intext,
							bool stripLeadingSpaces = true,
							char delim = ',')
	{
		typedef std::multimap<K,V> maptype;

		maptype c;
		return helpers::make_cont_<maptype,K,V>(c, intext, stripLeadingSpaces, delim);
	}

#if HASHED_CONTAINERS_SUPPORTED
	
	template<typename K, typename V>
	inline HASH_NS::hash_map<K,V> make_cont(const HASH_NS::hash_map<K,V> &, const std::string &intext,
							bool stripLeadingSpaces = true,
							char delim = ',')
	{
		typedef HASH_NS::hash_map<K,V> maptype;
		maptype c;
		return helpers::make_cont_<maptype,K,V>(c, intext, stripLeadingSpaces, delim);
	}

	template<typename K, typename V>
	inline HASH_NS::hash_multimap<K,V> make_cont(const HASH_NS::hash_multimap<K,V> &,
							const std::string &intext,
							bool stripLeadingSpaces = true,
							char delim = ',')
	{
		typedef HASH_NS::hash_multimap<K,V> maptype;

		maptype c;
		return helpers::make_cont_<maptype,K,V>(c, intext, stripLeadingSpaces, delim);
	}

#endif		// HASHED_CONTAINERS_SUPPORTED

	template<typename K, typename V>
	inline std::map<K,V> &set_cont(std::map<K,V> &c, const std::string &intext,
							bool stripLeadingSpaces = true,
							char delim = ',')
	{
		c.clear();
		return helpers::make_cont_<std::map<K,V>,K,V>(c, intext, stripLeadingSpaces, delim);
	}

	template<typename K, typename V>
	inline std::multimap<K,V> &set_cont(std::multimap<K,V> &c, const std::string &intext,
							bool stripLeadingSpaces = true,
							char delim = ',')
	{
		c.clear();
		return helpers::make_cont_<std::multimap<K,V>,K,V>(c, intext, stripLeadingSpaces, delim);
	}

#if HASHED_CONTAINERS_SUPPORTED

	template<typename K, typename V>
	inline HASH_NS::hash_map<K,V> &set_cont(HASH_NS::hash_map<K,V> &c, const std::string &intext,
							bool stripLeadingSpaces = true,
							char delim = ',')
	{
		c.clear();
		return helpers::make_cont_<HASH_NS::hash_map<K,V>,K,V>(c, intext, stripLeadingSpaces, delim);
	}

	template<typename K, typename V>
	inline HASH_NS::hash_multimap<K,V> &set_cont(HASH_NS::hash_multimap<K,V> &c, const std::string &intext,
							bool stripLeadingSpaces = true,
							char delim = ',')
	{
		c.clear();
		return helpers::make_cont_<HASH_NS::hash_multimap<K,V>,K,V>(c, intext, stripLeadingSpaces, delim);
	}

#endif		// HASHED_CONTAINERS_SUPPORTED

	template<typename K, typename V>
	inline std::map<K,V> &app_cont(std::map<K,V> &c, const std::string &intext,
							bool stripLeadingSpaces = true,
							char delim = ',')
	{
		return helpers::make_cont_<std::map<K,V>,K,V>(c, intext, stripLeadingSpaces, delim);
	}

	template<typename K, typename V>
	inline std::multimap<K,V> &app_cont(std::multimap<K,V> &c, const std::string &intext,
							bool stripLeadingSpaces = true,
							char delim = ',')
	{
		return helpers::make_cont_<std::multimap<K,V>,K,V>(c, intext, stripLeadingSpaces, delim);
	}


#if HASHED_CONTAINERS_SUPPORTED

	template<typename K, typename V>
	inline HASH_NS::hash_map<K,V> &app_cont(HASH_NS::hash_map<K,V> &c, const std::string &intext,
							bool stripLeadingSpaces = true,
							char delim = ',')
	{
		return helpers::make_cont_<HASH_NS::hash_map<K,V>,K,V>(c, intext, stripLeadingSpaces, delim);
	}

	template<typename K, typename V>
	inline HASH_NS::hash_multimap<K,V> &app_cont(HASH_NS::hash_multimap<K,V> &c,
							const std::string &intext,
							bool stripLeadingSpaces = true,
							char delim = ',')
	{
		return helpers::make_cont_<HASH_NS::hash_multimap<K,V>,K,V>(c, intext, stripLeadingSpaces, delim);
	}

#endif		// HASHED_CONTAINERS_SUPPORTED


	//
	//	Container &set_cont(Container &cont, const std::string &intext,
	//						bool stripLeadingSpaces = true, char delim = ',');
	//
	//		Function for assigning to a container a list of
	//		values specified in a delimited string (optionally skipping
	//		leading spaces for string values).
	//		First arg is the container to assign to.
	//		[For containers of non-string value_types, the bool arg is
	//		ignored.]
	//
	//	Example: vector<string> vs;  set_cont(vs, "1,2,3,5,500,600,-5");
	//

	template<typename Container>
	inline Container &set_cont(Container &cont, const std::string &intext,
							   bool stripLeadingSpaces = true, char delim = ',')
	{
		cont.clear();
		return helpers::make_cont_<Container>
					(typename Container::value_type(), cont, intext, stripLeadingSpaces, delim);
	}


	//
	//	Container &app_cont(Container &cont, const std::string &intext,
	//						bool stripLeadingSpaces = true, char delim = ',');
	//
	//		Function for appending to a container a list of
	//		values specified in a delimited string (optionally skipping
	//		leading spaces for string values).
	//		First arg is the container to assign to.
	//		[For containers of non-string value_types, the bool arg is
	//		ignored.]
	//
	//	Example: vector<string> vs; ...; app_cont(vs, "1,2,3,5,500,600,-5");
	//

	template<typename Container>
	inline Container &app_cont(Container &cont, const std::string &intext,
							   bool stripLeadingSpaces = true, char delim = ',')
	{
		return helpers::make_cont_<Container>
					(typename Container::value_type(), cont, intext, stripLeadingSpaces, delim);
	}



	///////////////////////////////////////////////////////////////////////////////
	//																			 //
	// Container-of-pointers initialization/generation templates given variable- //
	// length list of pointer values:											 //
	//																			 //
	///////////////////////////////////////////////////////////////////////////////


	//
	//	Container make_cont_p(T *p, ..., NULL);
	//
	//		Template to generate container with given pointer values.
	//		[Explicit container specialization required]
	//
	//	Example: vector<int *> vip = make_cont_p<vector<int *> >(&i, &j, &k, 0);
	//


	template<typename Container, typename T>
	Container make_cont_p(T *p, ...)
	{
		using namespace std;
		Container c;
							// create appropriate append functor
		helpers::Appender_<Container> app(c);
		va_list ap;

		va_start(ap, p);
		while (p != NULL)		 // NULL ends list
		{
			app(p);
			p = va_arg(ap, T *);
		}
		va_end(ap);

		return c;
	}


	//
	//	Container make_cont_p(const Container &, T *p, ..., NULL);
	//
	//		Template to generate container with given pointer values.
	//		First arg is an instance of the container type to build, but it is
	//			NOT modified (used to convey type information only).
	//
	//	Example: vector<int *> vip = make_cont_p(vip, &i, &j, &k, 0);
	//


	template<typename Container, typename T>
	Container make_cont_p(const Container &, T *p, ...)
	{
		using namespace std;
		Container c;
							// create appropriate append functor
		helpers::Appender_<Container> app(c);
		va_list ap;

		va_start(ap, p);
		while (p != NULL)		// NULL ends list
		{
			app(p);
			p = va_arg(ap, T *);
		}

		va_end(ap);

		return c;
	}


	//
	//	Container &set_cont_p(Container &c, T *p, ..., NULL);
	//
	//		Template to replace container contents with given pointer values.
	//		First arg is the container to assign to.
	//
	//	Example:	vector<int *> vip;
	//				// ...
	//				set_cont_p(vip, &i, &j, &k, 0);
	//

	template<typename Container, typename T>
	Container &set_cont_p(Container &c, T *p, ...)
	{
		using namespace std;
		va_list ap;
							// create appropriate append functor
		helpers::Appender_<Container> app(c);

		c.clear();

		va_start(ap, p);
		while (p != NULL)	 // NULL ends list
		{
			app(p);
			p = va_arg(ap, T *);
		}
		va_end(ap);

		return c;
	}

	//
	//	Container &app_cont_p(Container &c, T *p, ..., NULL);
	//
	//		Template to append to container contents with given pointer values.
	//		First arg is the container to assign to.
	//
	//	Example:	vector<int *> vip;
	//				// ...
	//				app_cont_p(vip, &i, &j, &k, 0);
	//

	template<typename Container, typename T>
	Container &app_cont_p(Container &c, T *p, ...)
	{
		using namespace std;
		va_list ap;
							// create appropriate append functor
		helpers::Appender_<Container> app(c);

		va_start(ap, p);
		while (p != NULL)	 // NULL ends list
		{
			app(p);
			p = va_arg(ap, T *);
		}
		va_end(ap);

		return c;
	}



	///////////////////////////////////////////////////////////////////////////////
	//																			 //
	//       Container initialization/generation templates given size,			 //
	//				        initial value and increment:						 //
	//																			 //
	///////////////////////////////////////////////////////////////////////////////


	//
	//	Container make_cont(int size,
	//						Container::value_type val = 1,
	//						Container::value_type incr = 1);
	//
	//		Template to generate a container of values with size elements
	//		in the sequence [val, val+incr, val+2*incr, ...].
	//		[Explicit container specialization required]
	//
	//	Example: vector<int> vi = make_cont<vector<int> >(100);
	//						

	template<typename Container>
	inline Container make_cont(int size,
							   typename Container::value_type val = 1,
							   typename Container::value_type incr = 1)
	{
		Container c;
		return helpers::make_cont_(c, size, val, incr);
	}


	//
	//	Container make_cont(const Container &, int size,
	//						Container::value_type val = 1,
	//						Container::value_type incr = 1);
	//
	//		Template to generate a container of values with size elements
	//		First arg is an instance of the container type to build, but it is
	//			NOT modified.
	//
	//		Example: vector<int> vi = make_cont(vi, 100, 0, .1);
	//

	template<typename Container>
	inline Container make_cont(const Container &, int size,
							   typename Container::value_type val = 1,
							   typename Container::value_type incr = 1)
	{
		Container c;
		return helpers::make_cont_(c, size, val, incr);
	}


	//
	//	Container &set_cont(Container &c, int size,
	//						Container::value_type val = 1,
	//						Container::value_type incr = 1);
	//
	//		Template to assign size values to the given container,
	//		in the sequence [val, val+incr, val+2*incr, ...].
	//		First arg is the container to be assigned.
	//
	//	Example: vector<int> vi; set_cont(vi, 100, 0, .1);
	//

	template<typename Container>
	inline Container &set_cont(Container &c, int size,
							   typename Container::value_type val = 1,
							   typename Container::value_type incr = 1)
	{
		c.clear();
		return helpers::make_cont_(c, size, val, incr);
	}

	//
	//	Container &app_cont(Container &c, int size,
	//						Container::value_type val = 1,
	//						Container::value_type incr = 1);
	//
	//		Template to append size values to the given container,
	//		in the sequence [val, val+incr, val+2*incr, ...].
	//		First arg is the container to be assigned.
	//
	//	Example: vector<int> vi; ...; app_cont(vi, 100, 0, .1);
	//

	template<typename Container>
	inline Container &app_cont(Container &c, int size,
							   typename Container::value_type val = 1,
							   typename Container::value_type incr = 1)
	{
		return helpers::make_cont_(c, size, val, incr);
	}

}	// namespace bds
	
#endif		// #if defined (INITUTIL_H)
