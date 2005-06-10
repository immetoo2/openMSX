//$Id$

#include <cstdlib>
#include <algorithm>
#include "StringOp.hh"

using std::string;
using std::transform;

namespace openmsx {

namespace StringOp {

int stringToInt(const string& str)
{
	return strtol(str.c_str(), NULL, 0);
}

bool stringToBool(const string& str)
{
	string low = StringOp::toLower(str);
	return (low == "true") || (low == "yes") || (low == "1");
}

double stringToDouble(const string& str)
{
	return strtod(str.c_str(), NULL);
}

string toLower(const string& str)
{
	string result = str;
	transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

} // namespace StringOp

} // namespace openmsx
