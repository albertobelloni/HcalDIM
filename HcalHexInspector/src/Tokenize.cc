/**
   \brief Simple tokenizer class

   This class parses a string into tokens separated by one or more of a list of delimiters.
   The tokens are returned in a vector of strings
*/


#include "HcalDIM/HcalHexInspector/interface/Tokenize.hh"

#include <string>
#include <vector>
#include <algorithm>

using namespace std;

/** \brief parse a string into tokens
    \param str String to parse
    \param tokens Vector of tokens
    \param delimeters Delimiters (default is space)
*/
void Tokenize(const string& str,
	      vector<string>& tokens,
	      const string& delimiters)
{
  // empty vector
  tokens.clear();
  // Skip delimiters at beginning.
  string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  string::size_type pos     = str.find_first_of(delimiters, lastPos);

  while (string::npos != pos || string::npos != lastPos)
    {
      // Found a token, add it to the vector.
      tokens.push_back(str.substr(lastPos, pos - lastPos));
      // Skip delimiters.  Note the "not_of"
      lastPos = str.find_first_not_of(delimiters, pos);
      // Find next "non-delimiter"
      pos = str.find_first_of(delimiters, lastPos);
    }
}
