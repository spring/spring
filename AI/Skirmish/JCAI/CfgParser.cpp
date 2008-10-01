//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#include "BaseAIDef.h"
#include "CfgParser.h"

// this keeps a list of implemented value types
vector<CfgValueClass*> CfgValue::classes;


//-------------------------------------------------------------------------
// CfgBuffer
//-------------------------------------------------------------------------

void CfgBuffer::ShowLocation() const {
	logPrintf ("In %s on line %d:", filename, line);
}

bool CfgBuffer::CompareIdent (const char *str) const
{
	int i = 0;
	while (str[i] && i+pos < len)
	{
		if (str[i] != data[pos+i])
			return false;

		i++;
	}

	return !str[i];
}

//-------------------------------------------------------------------------
// CfgValue - base config parsing class
//-------------------------------------------------------------------------

void CfgValue::dbgPrint (int depth)
{
	logPrintf ("??\n");
}

void CfgValue::AddValueClass (CfgValueClass *vc)
{
	if (find(classes.begin(), classes.end(), vc) == classes.end())
		classes.push_back (vc);
}

CfgValue* CfgValue::ParseValue (CfgBuffer& buf)
{
	CfgValue *v = 0;

	if (SkipWhitespace (buf)) {
		Expecting (buf, "Value");
		return 0;
	}

	for (int a=0;a<classes.size();a++)
	{
		if (classes[a]->Identify (buf))
		{
			v = classes[a]->Create ();

			if (!v->Parse (buf)) {
				delete v;
				return 0;
			}

			return v;
		}
	}

	// parse standard value types:
	char r = Lookahead (buf);

	if(buf.CompareIdent ("file"))
	{
		// load a nested config file
		return LoadNestedFile (buf);
	}
	else if(isalpha (*buf)) {
		v = new CfgLiteral;
		((CfgLiteral*)v)->ident = true;
	}
	else if(isdigit (r) || *buf == '.' || *buf == '-')
		v = new CfgNumeric;
	else if(*buf == '"')
		v = new CfgLiteral;
	else if(*buf == '{')
		v = new CfgList;

	if (v && !v->Parse (buf))
	{
		delete v;
		return 0;
	}

	return v;
}

CfgList* CfgValue::LoadNestedFile (CfgBuffer& buf)
{
	string s;
	if (!SkipKeyword (buf, "file"))
		return 0;

	SkipWhitespace (buf);

	CfgLiteral l;
	if (!l.Parse (buf))
		return 0;
	s = l.value;

	// insert the path of the current file in the string
	int i = strlen (buf.filename)-1;
	while (i > 0) {
		if (buf.filename [i]=='\\' || buf.filename [i] == '/')
			break;
		i--;
	}

	s.insert (s.begin(), buf.filename, buf.filename + i + 1);
	return LoadFile (s.c_str());
}

CfgList* CfgValue::LoadFile (const char *name)
{
	CfgBuffer buf;

	FILE *f = fopen (name, "rb");
	if (!f) {
		logPrintf ("Failed to open file %s\n", name);
		return 0;
	}

	fseek (f, 0, SEEK_END);
	buf.len = ftell(f);
	buf.data = new char [buf.len];
	fseek (f, 0, SEEK_SET);
	if (!fread (buf.data, buf.len, 1, f))
	{
		logPrintf ("Failed to read file %s\n", name);
		fclose (f);
		delete[] buf.data;
		return 0;
	}
	buf.filename = name;

	fclose (f);

	CfgList *nlist = new CfgList;
	if (!nlist->Parse (buf,true))
	{
		delete nlist;
		delete[] buf.data;
		return 0;
	}

	delete[] buf.data;
    return nlist;
}

bool CfgValue::SkipKeyword (CfgBuffer& buf, const char *kw)
{
	int a=0;
	while (!buf.end() && kw[a])
	{
		if (*buf != kw[a])
			break;
		++buf, ++a;
	}

	if (kw[a]) {
		buf.ShowLocation();
		logPrintf ("Expecting keyword %s\n", kw);
		return false;
	}

	return true;
}

void CfgValue::Expecting (CfgBuffer& buf, const char *s)
{
	buf.ShowLocation();
	logPrintf ("Expecting %s\n",  s);
}

bool CfgValue::ParseIdent (CfgBuffer& buf, string& name)
{
	if(isalnum (*buf))
	{
		name += *buf;
		for (++buf; isalnum (*buf) || *buf == '_' || *buf == '-'; ++buf)
			name += *buf;
		return true;
	}
	else {
		buf.ShowLocation(), logPrintf ("Expecting an identifier instead of '%c'\n", *buf);
		return false;
	}
}


bool CfgValue::SkipWhitespace (CfgBuffer& buf)
{
	// Skip whitespaces and comments
	for(;!buf.end();++buf)
	{
		if (*buf == ' ' || *buf == '\t' || *buf == '\r' || *buf == '\f' || *buf == 'v')
			continue;
		if (*buf == '\n')
		{
			buf.line ++;
			continue;
		}
		if (buf[0] == '/' && buf[1] == '/')
		{
			buf.pos += 2;
			while (*buf != '\n' && !buf.end())
				++buf;
			++buf.line;
			continue;
		}
		if (*buf == '/' && buf[1] == '*')
		{
			buf.pos += 2;
			while (!buf.end())
			{
				if(buf[0] == '*' && buf[1] == '/')
				{
					buf.pos+=2;
					break;
				}
				if(*buf == '\n')
					buf.line++;
				++buf;
			}
			continue;
		}
		break;
	}

	return buf.end();
}

char CfgValue::Lookahead (CfgBuffer& buf)
{
	CfgBuffer cp = buf;

	if (!SkipWhitespace (cp))
		return *cp;

	return 0;
}

//-------------------------------------------------------------------------
// CfgNumeric - parses int/float/double
//-------------------------------------------------------------------------

bool CfgNumeric::Parse (CfgBuffer& buf)
{
	bool dot=*buf=='.';
	string str;
	str+=*buf;
	++buf;
	while (1) {
		if(*buf=='.') {
			if(dot) break;
			else dot=true;
		}
		if(!(isdigit(*buf) || *buf=='.'))
			break;
		str += *buf;
		++buf;
	}
	if(dot) value = atof (str.c_str());
	else value = atoi (str.c_str());
	return true;
}

void CfgNumeric::dbgPrint (int depth)
{
	logPrintf ("%f\n", value);
}

//-------------------------------------------------------------------------
// CfgLiteral - parses string constants
//-------------------------------------------------------------------------
bool CfgLiteral::Parse (CfgBuffer& buf)
{
	if (ident)
		return ParseIdent (buf, value);

	++buf;
	while (*buf != '\n')
	{
		if(*buf == '\\')
			if(buf[1] == '"') {
				value += buf[1];
				buf.pos += 2;
				continue;
			}

		if(*buf == '"')
			break;

		value += *buf;
		++buf;
	}
	++buf;
	return true;
}

void CfgLiteral::dbgPrint (int depth)
{
	logPrintf ("%s\n", value.c_str());
}

//-------------------------------------------------------------------------
// CfgList & CfgListElem - parses a list of values enclosed with { }
//-------------------------------------------------------------------------

bool CfgListElem::Parse (CfgBuffer& buf)
{
	if (SkipWhitespace (buf))
	{
		logPrintf ("%d: Unexpected end of file in list element\n", buf.line);
		return false;
	}

	// parse name
	if (!ParseIdent (buf, name)) 
		return false;

	SkipWhitespace (buf);

	if (*buf == '=') {
		++buf;

		value = ParseValue (buf);

		return value!=0;
	} else
		return true;
}

bool CfgList::Parse (CfgBuffer& buf, bool root)
{
	if (!root)
	{
		SkipWhitespace (buf);

		if (*buf != '{') {
			Expecting (buf, "{");
			return false;
		}

		++buf;
	}

	while (!SkipWhitespace (buf))
	{
		if (*buf == '}') {
			++buf;
			return true;
		}

		childs.push_back (CfgListElem());
		if (!childs.back ().Parse (buf))
			return false;
	}

	if (!root && buf.end())
	{
		buf.ShowLocation (), logPrintf ("Unexpected end of node at line\n");
		return false;
	}

	return true;
}

CfgValue* CfgList::GetValue (const char *name)
{
	for (list<CfgListElem>::iterator i = childs.begin();i != childs.end(); ++i)
		if (!STRCASECMP (i->name.c_str(), name))
			return i->value;
	return 0;
}

double CfgList::GetNumeric (const char *name, double def)
{
	CfgValue *v = GetValue (name);
	if (!v) return def;

	CfgNumeric *n = dynamic_cast<CfgNumeric*>(v);
	if (!n) return def;

	return n->value;
}

const char* CfgList::GetLiteral (const char *name, const char *def)
{
	CfgValue *v = GetValue (name);
	if (!v) return def;

	CfgLiteral *n = dynamic_cast<CfgLiteral*>(v);
	if (!n) return def;

	return n->value.c_str();
}

void CfgList::dbgPrint(int depth)
{
	int n = 0;
	if (depth) 
		logPrintf ("list of %d elements\n", childs.size());

	for (list<CfgListElem>::iterator i = childs.begin(); i != childs.end(); ++i)
	{
		for (int a=0;a<depth;a++) logPrintf ("  ");
		logPrintf ("List element(%d): %s", n++, i->name.c_str());
		if (i->value) {
			logPrintf (" = ");
			i->value->dbgPrint (depth+1);
		} else
			logPrintf ("\n");
	}
}

