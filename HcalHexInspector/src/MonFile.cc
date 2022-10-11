
#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <vector>

#include <cstring>

#include "HcalDIM/HcalHexInspector/interface/Tokenize.hh"
#include "HcalDIM/HcalHexInspector/interface/MonFile.hh"

using namespace std;

MonFile::MonFile() {
  m_file_open = false;
  m_debug = false;
}

MonFile::~MonFile() {
  if( m_file_open)
    m_ifs.close();
}

MonFile::MonFile( string fn, bool flag)
{
  m_debug = flag;
  OpenFile( fn);
}

MonFile::MonFile( string fn)
{
  m_debug = false;
  OpenFile( fn);
}

void MonFile::OpenFile( string fn)
{
  m_ifs.open( fn.c_str(), fstream::in);
  m_file_open = m_ifs.is_open();
  if( m_file_open) {

    // read header, check length
    //char buff[1024];
    string h;

    if( !ReadQuick()) {
      cerr << "Error reading first line" << endl;
      abort();
    }

    h = GetLine();

    if( h.length() < 3) {
      cerr << "Suspicious first line, giving up" << endl;
      cerr << h;
      abort();
    }
    // parse column names
    Tokenize( h, m_ColNames, "\t");
    if( m_debug) {
      cout << "Header: [" << h << "]" << endl;
      cout << "Parsed " << m_ColNames.size() << " columns" << endl;
    }
    
    // build index
    for( unsigned int i=0; i<m_ColNames.size(); i++) {
      if( m_debug)
	printf("set index[%s] = %d\n", m_ColNames[i].c_str(), i);	
      m_ColIndex[m_ColNames[i]] = i;
    }

    m_Header = h;		// Save header
    m_LineNo = 1;
  }
}

string MonFile::GetColumn( int i)
{
  if( m_debug) cout << "GetColumn( int " << i << endl;
  return m_Values[i];
}

string MonFile::GetColumn( string c)
{
  map<string,int>::iterator it;
  string rs;

  if( m_debug) cout << "Trying to get column " << c << endl;

  it = m_ColIndex.find(c);

  if( it == m_ColIndex.end())
    rs = "-NoSuchColumn[" + c + "]-";
  else
    rs = GetColumn( m_ColIndex[c]);
  if( m_debug) cout << "MonFile::GetColumn(" << c << ") = " << rs << endl;
  return rs;
}

void MonFile::ParseLine( void) {
  string s = GetLine();
  Tokenize( s, m_Values, "\t");
  if( m_debug) cout << "found " << m_Values.size() << " columns" << endl;
}

bool MonFile::ReadQuick( void) {
  if( m_debug)
    cout << "MonFile::ReadLine()...";

  string s;
  while( getline( m_ifs, s)) {
    if( s.length() > 2 && s.substr(0,1) != "#") { // skip comments with #
      if( m_debug) cout << "read [" << s << "]" << endl;
      m_Line = s;
      return true;
    } else {
      if( m_debug) cout << "skip [" << s << "]" << endl;
    }
  }
  return false;
}

bool MonFile::ReadLine( void) {
  if( m_debug)
    cout << "MonFile::ReadLine()...";

  if( !ReadQuick())
    return false;

  string s = GetLine();
  if( m_debug) cout << "read [" << s << "]" << endl;
  ParseLine();
  return true;
}

int MonFile::GetColumnIndex( string s)
{
  return m_ColIndex[s];
}

// binary search to find first timestamp after t
// leave first later time in buffer, file positioned to next line
// return false if not found
bool MonFile::FindTime( string t)
{
  long first, last, /*mid,*/ siz;

  string ts = ValidateTimestamp( t);
  if( m_debug) cout << "MonFile::FindTime( " << ts << ")" << endl;


  // get size
  m_ifs.seekg( 0, fstream::end);
  siz = m_ifs.tellg();
  if( m_debug) cout << "  size = " << siz << endl;

  first = 0L;
  last = siz;

  if( siz < MONFILE_MIN_SRCH) {	             // file size < minimum... linear search
    return FindTimeLinear( ts);
  } else {
  
    // binary search... stop when diff < minimum
    while ( (last-first) > MONFILE_MIN_SRCH) {

      int mid = (first + last) / 2;  // compute mid point.
      if( m_debug) cout << "Searching in (" << first << "..." << last << ") mid="
			<< mid << endl;

      m_ifs.seekg( mid, fstream::beg);

      string tf;
      if( !FindNextTime( tf)) {
	cout << "Failed to find timestamp at " << mid << " range (" << first << "-" << last << ")" << endl;
	abort();	     // couldn't find any timestamp here... shouldn't happen
      }

      if( m_debug) cout << "Compare \"" << ts << "\" > \"" << tf << "\"" << endl;

      if (ts > tf) {
	first = mid + 1;  // repeat search in top half.
	if( m_debug) cout << "overshot - new first =" << first << endl;
      } else if (ts < tf) {
	last = mid - 1; // repeat search in bottom half.
	if( m_debug) cout << "undershot - new last =" << last << endl;
      } else {
	if( m_debug) cout << "Exact Match! - cleanup and return" << endl;
	return true;
      }
    }
    
    // OK, we need to do a linear search now, starting from first
    m_ifs.seekg( first, fstream::beg);
    return FindTimeLinear( ts);
  }
  
}



// read file and look for next timestamp and return in t
// return false on eof/error
// should just be start of next line
bool MonFile::FindNextTime( string& t)
{
  string s;

  while( ReadQuick()) {
    if( IsTimestamp( GetLine())) {
      t = GetTimestamp();
      return true;
    }
  }
  return false;
}


// search through file looking for timestamp >= t
bool MonFile::FindTimeLinear( string SearchTS)
{
  string FileTS;
  if( m_debug) cout << "MonFile::FindTimeLinear( " << SearchTS << ")" << endl;
  while( FindNextTime( FileTS)) {
    if( FileTS >= SearchTS) {
      if( m_debug) cout << "  Found time " << FileTS << " at offset " << m_ifs.tellg() << endl;
      return true;
    }
  }
  return false;
}


// should do more checking
string MonFile::ValidateTimestamp( string ts)
{
  string rts;

  string template_ts = "2008-01-01T00:00:00.000000";
  template_ts = template_ts.substr(0,MONFILE_TS_LEN);

  unsigned int n = ts.length();

  if( n < 10) {			// need at least a date
    cerr << "Need at least a date in timestamp " << ts << endl;
    abort();
  }

  if( n > template_ts.length()) {
    cerr << "Timestamp way too long " << ts << endl;
    abort();
  }

  if( !IsTimestamp(ts)) {
    cerr << "This doesn't look like a timestamp: " << ts << endl;
    cerr << "Correct format is                 : " << template_ts << endl;
    abort();
  }

  if( n < MONFILE_TS_LEN) {
    rts = ts + template_ts.substr( n);
  } else if( n > MONFILE_TS_LEN) {
    rts = ts.substr(0,MONFILE_TS_LEN);
  } else
    rts = ts;

  return rts;
}

// Keep it simple for efficiency
string MonFile::GetTimestamp(void)
{
  return ValidateTimestamp( GetLine().substr(0,MONFILE_TS_LEN));
}



// convert timestamp in format 2008-12-25T12:34:56
// to struct tm
void MonFile::ts_to_tm( string ts, struct tm* t)
{
  memset( t, 0, sizeof(struct tm));

  t->tm_year = atoi( ts.substr(0,4).c_str())-1900;
  t->tm_mon = atoi( ts.substr(5,2).c_str())-1;
  t->tm_mday = atoi( ts.substr(8,2).c_str());
  t->tm_hour = atoi( ts.substr(11,2).c_str());
  t->tm_min = atoi( ts.substr(14,2).c_str());
  t->tm_sec = atoi( ts.substr(17,2).c_str());
}

// convert struct tm back to timestamp
string MonFile::tm_to_ts( struct tm* t)
{
  char tmp[40];

  strftime( tmp, 40, "%Y-%m-%dT%H:%M:%S", t);
  string s = tmp;

  return s;
}
