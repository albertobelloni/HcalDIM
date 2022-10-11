//
// summarize DCC TTS states
//

#include <iostream>
#include <ctype.h>
#include <cstdlib>
#include <stdint.h>

#include "HcalDIM/HcalHexInspector/interface/MonFile.hh"

#define max(x,y) ((x)>(y)?(x):(y))
#define sec(n) ((n)/64e6)

using namespace std;

struct dcc_timers {
  uint64_t bsy, ofw, rdy, run, syn;
  int nt;
};

int main( int argc, char *argv[]) {

  string fn = argv[1];

  vector<dcc_timers> vt(32);

  MonFile mf( fn);

  if( !mf.is_open()) {
    cout << "Error opening file " << argv[1] << endl;
    abort();
  }

  while( mf.ReadLine()) {
    int fed = atoi( mf.GetColumn("FED").c_str());
    if( fed >= 700 && fed <= 731) {
      int i = fed-700;
      vt[i].run = max( strtoull( mf.GetColumn("RUN_TIMER").c_str(), NULL, 0), vt[i].run);
      vt[i].rdy = max( strtoull( mf.GetColumn("READY_TIMER").c_str(), NULL, 0), vt[i].rdy);
      vt[i].ofw = max( strtoull( mf.GetColumn("OVF_WARN_TIMER").c_str(), NULL, 0), vt[i].ofw);
      vt[i].bsy = max( strtoull( mf.GetColumn("BUSY_TIMER").c_str(), NULL, 0), vt[i].bsy);
      vt[i].syn = max( strtoull( mf.GetColumn("SYNC_LOST_TIMER").c_str(), NULL, 0), vt[i].syn);
      vt[i].nt++;
    }
  }

  printf("\"FED\",\"RUN\",\"RDY\",\"OFW\",\"BSY\",\"SYN\"\n");
  for( int i=0; i<32; i++) {
    if( vt[i].nt) {
      printf( "%3d,%8.2g,%8.2g,%8.2g,%8.2g,%8.2g\n",
	      i+700,
	      sec(vt[i].run),
	      sec(vt[i].rdy),
	      sec(vt[i].ofw),
	      sec(vt[i].bsy),
	      sec(vt[i].syn));
    }
  }

  return 0;

}
