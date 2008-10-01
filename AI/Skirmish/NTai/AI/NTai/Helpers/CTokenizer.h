// http://www.codeproject.com/cpp/stringtok.asp

// For the case when the separators are white spaces 0x09-0x0D and 0x20;
class CIsSpace : public unary_function<char, bool>
{
public:
  bool operator()(char c) const;
};

inline bool CIsSpace::operator()( char c ) const
{
    //isspace<char> returns true if c is a white-space character (0x09-0x0D or 0x20)
    // Need a locale for this call [TPK 04-10-06]
    locale loc;
    return isspace< char >( c, loc );
}



// For the case where the separator is the comma character ',':
class CIsComma : public unary_function<char, bool>
{
public:
  bool operator()(char c) const;
};

inline bool CIsComma::operator()(char c) const
{
  return (',' == c);
}

// For the case where the separator is a character from a set of characters given in a STL string:
class CIsFromString : public unary_function<char, bool>
{
public:
  //Constructor specifying the separators
  CIsFromString(string const& rostr) : m_ostr(rostr) {}
  bool operator()(char c) const;

private:
  string m_ostr;
};

inline bool CIsFromString::operator()(char c) const
{
  int iFind = m_ostr.find(c);
  if(iFind != int(string::npos))
    return true;
  else
    return false;
}

//Finally the string tokenizer class implementing the Tokenize() function is a static member function.
// Notice that CIsSpace is the default predicate for the Tokenize() function. (changed by AF to CIsComma)

template <class Pred=CIsComma>
class CTokenizer{
public:
  //The predicate should evaluate to true when applied to a separator.
  static void Tokenize(vector<string>& roResult, string const& rostr,
                       Pred const& roPred=Pred());
  static void Tokenize(set<string>& roResult, string const& rostr,
                       Pred const& roPred=Pred());
};

//The predicate should evaluate to true when applied to a separator.
template <class Pred>
inline void CTokenizer<Pred>::Tokenize(vector<string>& roResult,
                                            string const& rostr, Pred const& roPred)
{
	//First clear the results vector
	roResult.clear();
	string::const_iterator it = rostr.begin();
	string::const_iterator itTokenEnd = rostr.begin();
	while( it != rostr.end() ){
        //Eat seperators
        // Additional check for end of string here, needed if the last character(s)
        // in the input string are actually separator characters themselves.
        // Enables handling of a string composed entirely of separators [TPK 04-10-06]
        while( it != rostr.end() && roPred( *it ) ){
			it++;
        }

        //Find next token
		itTokenEnd = find_if(it, rostr.end(), roPred);
		//Append token to result
		if(it < itTokenEnd){
			roResult.push_back(string(it, itTokenEnd));
		}
		it = itTokenEnd;
	}
}

//The predicate should evaluate to true when applied to a separator.
template <class Pred>
inline void CTokenizer<Pred>::Tokenize(set<string>& roResult,
                                            string const& rostr, Pred const& roPred)
{
	//First clear the results vector
	roResult.clear();
	string::const_iterator it = rostr.begin();
	string::const_iterator itTokenEnd = rostr.begin();
	while( it != rostr.end() ){
		// Eat seperators
		// Additional check for end of string here, needed if the last character(s)
		// in the input string are actually separator characters themselves.
		// Enables handling of a string composed entirely of separators [TPK 04-10-06]
		while( it != rostr.end() && roPred( *it ) ){
			it++;
		}

		//Find next token
		itTokenEnd = find_if(it, rostr.end(), roPred);
		//Append token to result
		if(it < itTokenEnd){
			roResult.insert(string(it, itTokenEnd));
		}
		it = itTokenEnd;
	}
}


/// Howto use:
/*
//Test CIsSpace() predicate
cout << "Test CIsSpace() predicate:" << endl;
//The Results Vector
vector<string> oResult;
//Call Tokeniker
CTokenizer<>::Tokenize(oResult, " wqd \t hgwh \t sdhw \r\n kwqo \r\n  dk ");
//Display Results
for(int i=0; i<oResult.size(); i++)
  cout << oResult[i] << endl;
  */

/*//Test CIsComma() predicate
cout << "Test CIsComma() predicate:" << endl;
//The Results Vector
vector<string> oResult;
//Call Tokeniker
CTokenizer<CIsComma>::Tokenize(oResult, "wqd,hgwh,sdhw,kwqo,dk", CIsComma());
//Display Results
for(int i=0; i<oResult.size(); i++)
  cout << oResult[i] << endl;*/

/*//Test CIsFromString predicate
cout << "Test CIsFromString() predicate:" << endl;
//The Results Vector
vector<string> oResult;
//Call Tokeniker
CTokenizer<CIsFromString>::Tokenize(oResult, ":wqd,;hgwh,:,sdhw,:;kwqo;dk,",
                                          CIsFromString(",;:"));
//Display Results
cout << "Display strings:" << endl;
for(int i=0; i<oResult.size(); i++)
  cout << oResult[i] << endl;*/
