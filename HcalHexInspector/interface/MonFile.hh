#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <ctime>

// minimum size for binary search chunk... set longer than largest possible line length
#define MONFILE_MIN_SRCH 10000

// length of timestamp to use (only use integer seconds)
#define MONFILE_TS_LEN 19

using namespace std;

  /**
     \brief Class for accessing HCAL MonLogger text files

     @author Eric Hazen (Boston University)

     This class provides access to MonLogger text files written by the
     HCAL online software.  It makes a few assumptions about the file
     format:  There must be a "timestamp" column.  The timestamps must
     be close to time order.
  */



class MonFile {

public:

  MonFile();			///< default constructor
  ~MonFile();			///< destructor

  MonFile( string fname);	///< constructor with file name.  Open
				/// specified file.
  MonFile( string fname, bool debug);	///< constructor with file name, debug flag
  bool is_open( void) { return m_file_open; }  ///< report if file successfully opened
  int NumCol( void) { return m_ColNames.size(); }   ///< number of columns in file

  bool FindTime( string t);	///< Find timestamp using binary search in file

  bool ReadLine( void);		///< read line to buffer, return true if OK
  string GetColumn( string s); ///< get data for column by name
  string GetColumn( int n); ///< get data for column by index1

  string GetLine(void) { return m_Line; } ///< get current line
  string GetHeader(void) { return m_Header; } ///< get header line

  streampos GetPos() { return m_ifs.tellg(); }  ///< get current file position

  bool ReadQuick(void);		///< read line to buffer, do not parse columns
  void ParseLine(void);		///< parse columns in already read line

  int GetColumnIndex( string s); ///< get index number for a column

  void OpenFile( string fn); ///< open file (usually called by constructor)
  void SetDebug( bool d) { m_debug = d; } ///< set debug flag

  static bool IsTimestamp( string s) { return( s.substr(0,3)=="200"&&s.substr(4,1)=="-"
					 &&s.substr(7,1)=="-"); }

  static string tm_to_ts( struct tm* t);
  static void ts_to_tm( string ts, struct tm* t);

  bool FindTimeLinear( string t); ///< Find timestamp using linear search
  bool FindNextTime( string& t);  ///< Find next timestamp in file, true of ok, false if eof/error

  static string ValidateTimestamp( string ts);

  string GetTimestamp(void);	///< extract timestamp from buffered line

private:
  bool m_debug;
  bool m_file_open;
  vector<string> m_ColNames; ///< vector of column names
  map<string,int> m_ColIndex;	///< map column names to column index
  vector<string> m_Values; ///< vector of current column values
  ifstream m_ifs;		///< input file
  int m_LineNo;			///< current line number in file
  string m_Line;		///< current line (raw)
  string m_Header;		///< header
};
