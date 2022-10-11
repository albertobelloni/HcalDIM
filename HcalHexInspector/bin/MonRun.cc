//
// find monitor entries for a run from monitoring file(s)
// usage:  MonRun <run_no> <file> ...
//

#include <iostream>
#include <ctype.h>
#include <ctime>
#include <stdint.h>

#include <cstdlib>
#include <stdlib.h>

#include "HcalDIM/HcalHexInspector/interface/MonFile.hh"

#define RUN_INFO_FILE "/nfshome0/hcalsw/monlog_loc/ALL/HCAL_SUPERVISOR_STATE.txt"

// extend run times by this many seconds (start and end)
#define EXTRA_SEC 30

static bool debug = false;

using namespace std;

void usage() {
  cout << "usage:  MonRun <run> [-c column [-c column]] <file> [<file>...]" << endl;
}

void output_columns( MonFile& mf, vector<string>& cols, vector<string>& ops, vector<uint64_t>& vals, vector<bool>is_expr);
void add_col_expr( const char* s, vector<string>& cols, vector<string>& ops, vector<uint64_t>& vals, vector<bool>& is_expr);
void dump_col_exprs( vector<string>& cols, vector<string>& ops, vector<uint64_t>& vals, vector<bool>& is_expr);


static bool do_and = false;

int main( int argc, char *argv[]) {

  vector<string> files;
  vector<string> columns;
  vector<string> col_oper;
  vector<uint64_t> col_val;
  vector<bool> col_expr;

  bool select_columns = false;

  string time_hi = "1999-01-01";
  string time_lo = "2999-12-31";

  int run = -999;

  add_col_expr( "timestamp", columns, col_oper, col_val, col_expr);

  cout << "# Processing " << argc << " arguments" << endl;

  for( int i=1; i<argc; i++) {
    if( *argv[i] == '-') {
      switch( toupper( argv[i][1])) {
      case 'D':
	debug = true;
	break;
      case 'A':
	do_and = true;
	break;
      case 'C':
	while( i < argc-1 && *argv[i+1] != '-') {
	  ++i;
	  cout << "Add column expr " << argv[i] << endl;
	  add_col_expr( argv[i], columns, col_oper, col_val, col_expr);
	}
	select_columns = true;
	break;
      default:
	cout << "Don't understand option " << argv[i] << endl;
	usage();
      }
    } else {
      if( isdigit( *argv[i]))
	run = atoi( argv[i]);
      else 
	files.push_back( argv[i]);
    }
  }

  if( debug) dump_col_exprs(columns, col_oper, col_val, col_expr);

  // open the file with run times
  MonFile RunInfo( RUN_INFO_FILE, false);

  // read the whole thing, keep track of run times
  if( !RunInfo.is_open()) {
    cout << "Open failed on " << RUN_INFO_FILE << endl;
    abort();
  }

  int ri = RunInfo.GetColumnIndex( "RUN_NUMBER"); // save index for efficiency
  int ti = RunInfo.GetColumnIndex( "timestamp"); // save index for efficiency

  bool before_run = true;
  bool in_run = false;

  /// get run times
  while( RunInfo.ReadLine()) {

    if( before_run) {
      if( atoi(RunInfo.GetColumn( ri).c_str()) == run) {
	time_lo = RunInfo.GetColumn( ti).substr(0,MONFILE_TS_LEN);
	before_run = false;
	in_run = true;
      }
    } else if( in_run) {
      if( atoi(RunInfo.GetColumn( ri).c_str()) != run) {
	time_hi = RunInfo.GetColumn( ti).substr(0,MONFILE_TS_LEN);
	in_run = false;
      }
    } else {
      break;
    }
  }

  if( before_run) {
    cout << "ERROR: Did not find run " << run << endl;
    abort();
  }

  cout << "# Time range for run " << run << ":  " << time_lo << " - " << time_hi << endl;

  // subtract EXTRA_SEC seconds from time_lo
  struct tm tyme_tm;
  tyme_tm.tm_isdst = 0;

  MonFile::ts_to_tm( time_lo, &tyme_tm);
  tyme_tm.tm_sec -= EXTRA_SEC;
  mktime( &tyme_tm);		// normalize
  time_lo = MonFile::tm_to_ts( &tyme_tm);

  // add EXTRA_SEC to time_hi
  MonFile::ts_to_tm( time_hi, &tyme_tm);
  tyme_tm.tm_sec += EXTRA_SEC;
  mktime( &tyme_tm);		// normalize
  time_hi = MonFile::tm_to_ts( &tyme_tm);

  cout << "# Adj  range for run " << run << ":  " << time_lo << " - " << time_hi << endl;

  // loop over files, extract output in range
  int nf = files.size();
  if( nf) {
    for( int i=0; i<nf; i++) {
      cout << "#----- " << files[i] << " -----" << endl;

      MonFile mf( files[i], false);
      if( select_columns) {
	for( unsigned int ii=0; ii<columns.size(); ii++) {
	  cout << columns[ii] << "\t";
	}
	cout << endl;
      } else
	cout << mf.GetHeader() << endl;

      // find start of run
      if( mf.FindTime( time_lo)) {
	cout << "# Found start of run at " << mf.GetLine().substr(0,MONFILE_TS_LEN) << endl;
	if( select_columns)
	  output_columns( mf, columns, col_oper, col_val, col_expr);
	else
	  cout << mf.GetLine() << endl; // output starting line
	while( mf.ReadQuick()) { // read without unpacking
	  string t = mf.GetTimestamp();
	  if( t > time_hi)
	    break;
	  if( select_columns)
	    output_columns( mf, columns, col_oper, col_val, col_expr);
	  else
	    cout << mf.GetLine() << endl;
	}
      } else {
	cout << "# Did not find start time \"" << time_lo << "\" in file " << files[i] << endl;
      }
    }
  }

  return 0;

}





void output_columns( MonFile& mf, vector<string>& cols, vector<string>& ops, vector<uint64_t>& vals, vector<bool>is_expr)
{
  mf.ParseLine();

  /*int nc = cols.size();*/

  bool b_this, b_and, b_or;

  b_and = true;
  b_or = false;

  // set conditions for each column
  for( unsigned int i=0; i<cols.size(); i++) {
    if( is_expr[i]) {

      string s = mf.GetColumn( cols[i]);
      uint64_t sv = strtoull( s.c_str(), NULL, 0);

      if( debug) cout << "Get column " << i << " which is " << s << endl;

      b_this = false;

      if( ops[i] == "=")
	b_this = (sv == vals[i]);
      else if( ops[i] == "!=")
	b_this = (sv != vals[i]);
      else if( ops[i] == "<")
	b_this = (sv < vals[i]);
      else if( ops[i] == ">") {
	b_this = (sv > vals[i]);
	if( debug) cout << " compare " << sv << ">" << vals[i] << " = " << b_this << endl;
      } else {
	cerr << "unknown operator: " << ops[i] << endl;
	abort();
      }

      if( debug)
	cout << "column " << s << " b_this=" << b_this << endl;

      b_and = b_and && b_this;
      b_or = b_or || b_this;
      if( debug)
	cout << "  b_and=" << b_and << " b_or=" << b_or << endl;
    }
  }

  if( (do_and && b_and) || (!do_and && b_or)) {
    if( debug) cout << "*** dump this row ***" << endl;
    for( unsigned int i=0; i<cols.size(); i++)
      cout << mf.GetColumn( cols[i]) << "\t";
    cout << endl;
  }

}


//
// parse and add column expression of the form:
//    <name>[<op><value>]
// where <op> is < > = !=
// value is stored as string, but if all digits is compared as numeric
//
void add_col_expr( const char* s, vector<string>& cols, vector<string>& ops, vector<uint64_t>& vals, vector<bool>& is_expr)
{
  cout << "# add_col_expr(): Processing string " << s << endl;

  string e = s;
  size_t op = e.find_first_of("<>!=");
  if( op == string::npos) {	// no operator

    cout << "# no operator, save " << s << endl;

    cols.push_back( s);
    ops.push_back("");
    vals.push_back(0);
    is_expr.push_back(false);
  } else {
    size_t op2 = e.find_last_of("<>!=");
    string my_col = e.substr(0,op);
    string my_op = e.substr(op,(op2-op)+1);
    string s_val = e.substr(op2+1);
    uint64_t my_val = strtoull( s_val.c_str(), NULL, 0);
    cout << "# Saving expr " << my_col << " " << my_op << " " << my_val << endl;
    cols.push_back( my_col);
    ops.push_back( my_op);
    vals.push_back( my_val);
    is_expr.push_back(true);
  }
}


//
// Dump expressions for debug
//
void dump_col_exprs( vector<string>& cols, vector<string>& ops, vector<uint64_t>& vals, vector<bool>& is_expr)
{
  int nc = cols.size();
  int no = ops.size();
  int nv = vals.size();
  int ex = is_expr.size();

  cout << "nc=" << nc << " no=" << no << " nv=" << nv << " ex=" << ex << endl;
  for( int i=0; i<nc; i++) {
    printf("%2d: cols='%s' ops='%s' vals='%lu' is_expr=%s\n",
	   i, cols[i].c_str(), ops[i].c_str(), vals[i],
	   is_expr[i] ? "true" : "false");
  }
}
