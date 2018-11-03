/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FileFilter.h"


#include "System/SpringRegex.h"

#include <climits>
#include <cctype>
#include <sstream>
#include <vector>

using std::string;
using std::vector;


class CFileFilter : public IFileFilter
{
public:
	void AddRule(const string& rule) override;
	bool Match(const string& filename) const override;

private:
	string glob_to_regex(const string& glob);

	struct Rule {
		Rule() = default;
		string glob;
		spring::regex regex;
		bool negate = false;
	};

	vector<Rule> rules;
};


IFileFilter* IFileFilter::Create()
{
	return new CFileFilter();
}


/** @brief Add a filtering rule.

A rule can be:
 - An empty line, this is ignored,
 - A line starting with a '#', this serves as a comment and is ignored,
 - A path starting with a path separator ('/' or '\'): this is an absolute
   path and matches only against the entire leading part of the filename
   passed to Match(): '/foo' matches 'foo' and 'foo/bar', but not 'bar/foo'.
 - Any other path is a relative path and is matched less strict: as long as
   there is a consecutive set of path elements matching the rule, there is a
   match: 'b/c/d' matches 'b/c/d', but also 'a/b/c/d/e'.

Note that:
 - Leading and trailing whitespace is ignored.
 - Globbing characters '*' and '?' can be used, both do NOT match path
   separators (like in shell, but unlike fnmatch(), or so I've been told.)
   e.g. 'foo\\*\\baz' matches 'foo/bar/baz' but not 'foo/ba/r/baz'.
 - Any path separator matches any other path separator, so there is no need to
   worry about converting them: 'foo/bar' matches 'foo\\bar' and 'foo:bar' too.
 - A path can be prefixed with an exclamation mark '!', this negates the
   pattern. Because the rules are matched in-order, one can use this to exclude
   a file from a more generic pattern.
 - By default, no file matches. This can be changed using AddRule("*") ofc.
*/
void CFileFilter::AddRule(const string& rule)
{
	if (rule.empty())
		return;

	// Split lines if line endings are present.
	if (rule.find('\n') != string::npos) {
		size_t beg = 0, end = 0;
		while ((end = rule.find('\n', beg)) != string::npos) {
			//printf("line: %s\n", rule.substr(beg, end - beg).c_str());
			AddRule(rule.substr(beg, end - beg));
			beg = end + 1;
		}
		AddRule(rule.substr(beg));
		return;
	}

	// Eat leading whitespace, return if we reach end of string.
	size_t p = 0;
	while (isspace(rule[p]))
		if (++p >= rule.length())
			return;

	// Nothing to do if the rule is a comment.
	if (rule[p] == '#')
		return;

	// Eat trailing whitespace, return if we meet p.
	size_t q = rule.length() - 1;
	while (isspace(rule[q])) {
		if (--q < p) {
			return;
		}
	}

	// Build the rule.
	Rule r;
	if (rule[p] == '!') {
		r.negate = true;
		if (++p > q) {
			return;
		}
	}
	r.glob = rule.substr(p, 1 + q - p);
	r.regex = spring::regex(glob_to_regex(r.glob)
		, spring::regex::icase);
	rules.push_back(r);
	//printf("added %s%s: %s\n", r.negate ? "!" : "", r.glob.c_str(), r.regex.expression());
}


/** @brief Checks whether filename matches this filter. */
bool CFileFilter::Match(const string& filename) const
{
	bool match = false;
	for (const auto& rule: rules) {
		if (spring::regex_search(filename, rule.regex))
			match = !rule.negate;
	}
	return match;
}


string CFileFilter::glob_to_regex(const string& glob) // FIXME remove; duplicate in FileSystem::ConvertGlobToRegex
{
#define PATH_SEPARATORS "/\\:"

	std::stringstream regex;
	string::const_iterator i = glob.begin();

	// If the path starts with a path separator, we take it as an absolute path
	// (relative to whatever is passed to Match() later on), so we insert the
	// begin anchor.

	// Otherwise we 'just' need to make sure the glob matches only full path
	// elements, so we require either start of line OR path separator.

	if ((i != glob.end() && *i == '/') || *i == '\\') {
		regex << '^';
		++i;
	}
	else
		regex << "(^|[" PATH_SEPARATORS "])";

	for (; i != glob.end(); ++i) {
		char c = *i;
		switch (c) {
			case '*':
				// In (shell) globbing the wildcards match anything except path separators.
				regex << "[^" PATH_SEPARATORS "]*";
				break;
			case '?':
				regex << "[^" PATH_SEPARATORS "]";
				break;
			case '/':
			case '\\':
			case ':':
				// Any path separator matches any other path separator.
				// (So we don't have to manually convert slashes before search.)
				regex << "[" PATH_SEPARATORS "]";
				break;
			default:
				if (!(isalnum(c) || c == '_'))
					regex << '\\';
				regex << c;
				break;
		}
	}

	// Make sure we only match full path elements. (see above)
	regex << "([" PATH_SEPARATORS "]|$)";

	return regex.str();
}
