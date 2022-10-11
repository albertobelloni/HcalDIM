//
// 

#include <iostream>
#include <ctype.h>

#include "HcalDIM/HcalHexInspector/interface/MonFile.hh"

using namespace std;

int main( int argc, char *argv[]) {

  string fn;
  string s1 = "1999-01-01";

  for( int i=1; i<argc; i++) {
    if( isdigit( *argv[i])) {
      s1 = argv[i];
    } else {
      fn = argv[i];
    }

  }

  string ts = MonFile::ValidateTimestamp(s1);

  cout << "Searching for \"" << s1 << "\"" << endl;

  MonFile m( fn, true);
  if( !m.is_open())
    cout << "Open failed on " << argv[1] << endl;
  else
    cout << "File opened successfully" << endl;

  m.FindTime( ts);

  return 0;

}
