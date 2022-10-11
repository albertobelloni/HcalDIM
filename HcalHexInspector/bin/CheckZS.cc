//
// Read deadbeef files, calculate "ZS criteria"
//

#define TEST_COND(fed,h) (fed->fed_id() == 726 && h == 5)

#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <stdint.h>

#include <iostream>

#include "HcalDIM/HcalHexInspector/interface/FedGet.hh"
#include "HcalDIM/HcalHexInspector/interface/dccv4_format.h"

#define DCC_VER 0x2c18

#define LHC_BUNCHES 3564

using namespace std;

typedef uint32_t DWORD;

#define MAX_HTR_PAYLOAD 0x800

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#define fed_valid(f) ((f)>=HCAL_MIN_FED&&(f)<(HCAL_MIN_FED+HCAL_NUM_FED))

static int evt_ct;
static long fposn;
static bool debug = false;
static int thr;

void usage() {
  printf("Revision 99 Sept 2008\n");
  printf("usage: CheckZS  [-f lo_fed hi_fed] [-t threshold] file\n");
}

// define size of arrays for DAQ and TP samples (actually 20, but be safe!)
#define MAX_SAMP 32
#define MAX_HTR_PAYLOAD 0x800

// arbitrary upper limit on maxsum for ZS study
#define MAX_SUM 100

static int max_hist[MAX_SUM+1];

static long max_counts;
static long max_us;
static long not_ok;

static unsigned int lo_fed, hi_fed;

int main( int argc, char *argv[])
{
  FILE *fp = NULL;
  /*FILE *rf = NULL;*/

  /*int nev = 0;*/

  // command line controls
  if( argc > 1) {
    for( int i=1; i<argc; i++) {
      if( *argv[i] == '-') {

	switch( toupper( argv[i][1])) {

	case 'T':
	  if( i < (argc-1) && isdigit( *argv[i+1])) {
	    ++i;
	    thr = atoi( argv[i]);
	  } else {
	    printf("Need threshold after -t\n");
	    exit(1);
	  }
	  break;

	case 'F':
	  if( i < (argc-2) && isdigit( *argv[i+1]) && isdigit( *argv[i+2]) ) {
	    ++i;
	    lo_fed = atoi( argv[i]);
	    ++i;
	    hi_fed = atoi( argv[i]);
	  } else {
	    printf("Need FEDs after -f\n");
	    exit(1);
	  }
	  break;

	default:
	  printf("Don't understand option %s\n", argv[i]);
	  usage();
	  exit(1);
	}
      } else {
	if( fp == NULL) {
	  if( (fp = fopen( argv[i], "r")) == NULL)
	    printf("Error opening file %s\n", argv[i]);
	}
      }
    }
  } else {
    usage();
  }

  bool fed_range = false;
  if( fed_valid(lo_fed) && fed_valid(hi_fed))  // FED range specified?
    fed_range = true;
  
  if( fp == NULL) {
    printf("Need an input file!\n");
    exit( 1);
  }

  DWORD my_hdr[2];		// my header - sentinel plus word count
  DWORD my_evt[MAX_WORDS];

  printf("Starting to read file...\n");

  // loop reading and dumping events
  while( fread( my_hdr, sizeof(my_hdr), 1, fp) == 1) {

    if( my_hdr[0] != MY_SENTINEL) {
      printf("Expecting %08x, saw %08x at event start\n",
	     MY_SENTINEL, my_hdr[0]);
      exit( 1);
    }

    if( my_hdr[1] < 20 || my_hdr[1] > MAX_WORDS) {
      printf("Event size of %d words is unreasonable\n", my_hdr[1]);
      exit( 1);
    }

    unsigned int rc;
    if( (rc = fread( my_evt, sizeof(DWORD), my_hdr[1], fp)) != my_hdr[1]) {
      printf("Short read... tried for %d, got %d words\n", my_hdr[1], rc);
      goto bail_out;
    }

    FedGet* fed;
    fed = new FedGet( my_evt, my_hdr[1], DCC_VER);

    if( (!fed_range && fed_valid(fed->fed_id())) ||
	(fed->fed_id() >= lo_fed && fed->fed_id() <= hi_fed)) {

      if( debug) printf("Evn %d FED %d\n", fed->EvN(), fed->fed_id());

      // find good digis
      for( int h=0; h<FEDGET_NUM_HTR; h++) {
	if( fed->HTR_nWords(h)) {

	  if( fed->HTR_us(h)) {	// unsuppressed "sprinkle"?
	    ++max_us;
	  } else {

	    if( debug) printf("  HTR %d\n", h);

	    for( int ch=0; ch<FEDGET_NUM_CHAN; ch++) {

	      if( fed->HTR_QIE_status(h, ch) == QIE_STATUS_OK) {

		int maxsum = fed->HTR_QIE_zs_sum( h, ch);

		if( maxsum > 0) {

		  if( maxsum > MAX_SUM) {
		    ++max_hist[MAX_SUM];
		  } else {
		    ++max_hist[maxsum];
		    ++max_counts;
		  }		

		  if( thr && (maxsum < thr)) {
		    printf("maxsum %d is below threshold\n", maxsum);
		    fed->Dump(9);
		  }

		  //		  if( debug)
		  if( debug && TEST_COND(fed,h))
		    printf("EvN %6d FED %3d HTR %2d ch %2d maxsum %d\n",
			   fed->EvN(), fed->fed_id(), h, ch, maxsum);
		}
	      } else {
		++not_ok;
	      }
	    }
	  }
	}
      }
      
    }

    ++evt_ct;
    fposn = ftell( fp);
    if( !(evt_ct % 10000))
      printf("%d fragments processed\n", evt_ct);
  }

 bail_out:
  printf("Processed 0x%x (%d) fragments\n", evt_ct, evt_ct);

  printf("ZS criteria histogram:\n");
  for( int i=0; i<MAX_SUM; i++) {
    if( max_hist[i])
      printf("%3d %d\n", i, max_hist[i]);
  }
  printf("%ld Counts, Overflow (> %d) = %d\n", max_counts, MAX_SUM, max_hist[MAX_SUM]);
  printf("%ld unsuppressed\n", max_us);
  printf("%ld bad digi\n", not_ok);

}

