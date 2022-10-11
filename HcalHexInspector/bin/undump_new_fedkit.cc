//  undump_new_fedkit.cc
//
//  read output of fedkit-test-merge with --dump option
//  write a "deadbeef" format binary file
//  complain and abort if format not recognized
//

#include <iostream>
#include <fstream>
#include <string>
#include <stdint.h>
#include <stdlib.h>

// maximum event size (32-bit words)
#define MAX_EVT 0x8000
// DCC header size (64-bit words)
#define HDR_SIZ 12

using namespace std;

static string output_file = "test.dat";
static string input_file = "test.txt";

static string usage = "usage: undump_new_fedkit [-v] [input_file [output_file.dat]]";

static int verbose = 0;

int main( int argc, char *argv[])
{

  int file_names = 0;

  if( argc > 1) {
    for( int i=1; i<argc; i++) {
      if( *argv[i] == '-') {
	switch( toupper( argv[i][1])) {
	case 'H':
	  cout << usage << endl;
	  exit( 1);
	case 'V':
	  ++verbose;
	}
      } else {
	if( file_names == 0)
	  input_file = argv[i];
	else if( file_names == 1)
	  output_file = argv[i];
	++file_names;
      }	
    }
  }
  
  ifstream f_in;
  f_in.open( input_file.c_str() );
  if( !f_in.is_open()) {
    cout << "Error opening " << input_file << " for reading" << endl;
    exit(1);
  }

  ofstream f_out;
  f_out.open( output_file.c_str(), ios::out | ios::binary );
  if( !f_out.is_open()) {
    cout << "Error opening " << output_file << " for writing" << endl;
    exit(1);
  }

  // read lines looking for start of hex stuff
  char buff[256];
  string line;
  if( verbose) cout << "Starting to read " << input_file << endl;

  while( f_in.getline( buff, 256)) {
    line = buff;
    if( verbose) cout << line << endl;
    if( line.find( "Going on infinite", 0) != string::npos ) {
      cout << "Found start of data" << endl;
      break;
    }
  }


  int nwords = 0;
  int evt_size = 0;
  uint32_t data[MAX_EVT];
  int evt_num = 0;

  while( f_in.getline( buff, 256)) {
    line = buff;
    if( line.find(" (PCI ", 0) == string::npos) {
      cout << "Found end of data" << endl;
      break;
    }
    // parse hex data
    uint32_t hi, lo;
    /*int n = */ sscanf( buff, "%*s %*s %*s %*s 0x%08x%08x", &hi, &lo);
    if( verbose > 1) printf("%08x %08x\n", hi, lo);

    if( nwords*2+1 >= MAX_EVT) {
      cout << "ERROR:  event length overflow " << endl;
      cerr << "ERROR:  event length overflow " << endl;
      exit( 1);
    }

    data[nwords*2] = lo;
    data[nwords*2+1] = hi;

    if( nwords == HDR_SIZ-1) {
      if( verbose) printf("Event %d\n", evt_num);
      ++evt_num;
      if( verbose) {
	cout << "HEADER:" << endl;
	for( int i=0; i<HDR_SIZ*2; i++)
	  printf("%2d: 0x%08x\n", i, data[i]);
      }
      // calculate event size
      evt_size = 8+18;
      for( int i=6; i<21; i++)
	evt_size += data[i] & 0xfff;
      if( verbose) printf("Calculated event size %d (0x%x) words\n", evt_size, evt_size);
      if( evt_size & 1) {
	++evt_size;
	if( verbose) printf( "Adjusted event size %d (0x%x) words\n", evt_size, evt_size);
      }
    }

    if( nwords*2 == evt_size-2) {
      if( verbose) {
	cout << "END of event" << endl;
      }
      uint32_t tmp;
      tmp = 0xdeadbeef;
      f_out.write( (char*)&tmp, sizeof(tmp));
      tmp = evt_size;
      f_out.write( (char*)&tmp, sizeof(tmp));
      for( int i=0; i<evt_size; i++)
	f_out.write( (char*)&data[i], sizeof(data[0]));
      nwords = 0;
    } else 
      ++nwords;
  }

  cout << nwords << " words left over" << endl;

  if( nwords == evt_size-1) {
    if( verbose) {
      cout << "END of event" << endl;
    }
    uint32_t tmp;
    tmp = 0xdeadbeef;
    f_out.write( (char*)&tmp, sizeof(tmp));
    tmp = evt_size;
    f_out.write( (char*)&tmp, sizeof(tmp));
    for( int i=0; i<evt_size; i++)
      f_out.write( (char*)&data[i], sizeof(data[0]));
    nwords = 0;
  }

  cout << evt_num << " events written to " << output_file << endl;

  return 0;
}
