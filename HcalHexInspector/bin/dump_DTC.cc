/*
 * dump_DTC.cc - dump raw data in 'deadbeef' format from AMC13/DTC
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <cctype>
#include <cstdlib>
#include <string.h>

#include "HcalDIM/HcalHexInspector/interface/FedDTC.hh"

static int dump_level = 0;
static long fposn;

#define MAX_WORDS 0x10000
#define MY_SENTINEL 0xdeadbeef

void usage() {
  puts( "usage:  dump_DTC.exe [options] file_name");
  puts( "  -e (check for sequential EvN)");
  puts( "  -h (check for uHTR EvN)");
  puts( "  -v (verbose)");
  puts( "  -d <dump_level>");
  puts( "     1=DTC header 2=uHTR headers  3=uHTR payload");
}

// keep track of uHTR offsets
typedef struct {
  int EvN, BcN, OrN;
} a_num;

static a_num uHtrOffs[FEDGET_NUM_UHTR];

int check_num_offset( uint32_t, uint32_t, uint32_t, uint32_t*);

int main( int argc, char *argv[]) {

  FILE *fp = NULL;
  int evt_ct = 0;
  int verbose = 0;
  int check_evn = 0;
  int check_uhtr_evn = 0;
  int errz = 0;

  // command line controls
  if( argc > 1) {
    for( int i=1; i<argc; i++) {
      if( *argv[i] == '-') {
	switch( toupper( argv[i][1])) {

	case 'H':
	  check_uhtr_evn = 1;
	  break;

	case 'E':
	  check_evn = 1;
	  break;

	case 'V':
	  if( i < (argc-1)) {
	    if( isdigit( *argv[i+1])) {
	      ++i;
	      verbose = atoi( argv[i]);
	    } else
	      ++verbose;	      
	  } else
	    ++verbose;
	  break;
	  
	case 'D':
	  if( i < (argc-1)) {
	    if( isdigit( *argv[i+1])) {
	      ++i;
	      dump_level = atoi( argv[i]);
	    } else
	      ++dump_level;	      
	  } else
	    ++dump_level;
	  break;
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
    exit( 1);
  }

  uint32_t my_hdr[2];		// my header - sentinel plus word count
  uint32_t my_evt[MAX_WORDS];
  uint32_t last_evn = -999;

  fposn = ftell( fp);

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

    if( verbose) {
      printf("------------------------------------------------------------\n");
      printf("FILE: Event %d has %d words\n", evt_ct, my_hdr[1]);
    }
  
    memset( my_evt, 0, sizeof( my_evt));

    unsigned int rc;

    if( (rc = fread( my_evt, sizeof(uint32_t), my_hdr[1], fp)) != my_hdr[1]) {
      printf("Short read... tried for %d, got %d words\n", my_hdr[1], rc);
      exit( 1);
    }

    if( dump_level == 9) {
      for( unsigned int i=0; i<my_hdr[1]; i++)
	printf("%04x: %08x\n", i, my_evt[i]);
    }

    FedDTC fed( my_evt, my_hdr[1], 0);

    // check if uHTR EvN etc match AMC13 EvN
    if( check_uhtr_evn) {
      for( int i=0; i<FEDGET_NUM_UHTR; i++) {
	int evn_e, bcn_e, orn_e;
	if( fed.UHTR_nWords( i)) {
	  evn_e = check_num_offset( fed.EvN(), fed.UHTR_EvN(i), UHTR_EVN_MASK, (uint32_t *)&uHtrOffs[i].EvN);
	  bcn_e = check_num_offset( fed.BcN(), fed.UHTR_BcN(i), UHTR_BCN_MASK, (uint32_t *)&uHtrOffs[i].BcN);
	  orn_e = check_num_offset( fed.OrN(), fed.UHTR_OrN(i), UHTR_ORN_MASK, (uint32_t *)&uHtrOffs[i].OrN);
	  if( evn_e || bcn_e || orn_e) {
	    printf("ERR Number mismatches at EvN=%06x in uHTR %d:\n", fed.EvN(), i);
	    if( evn_e) printf("  DTC EvN =   %06x UHTR = %06x (offset %x)\n", fed.EvN(), fed.UHTR_EvN(i), evn_e);
	    if( bcn_e) printf("  DTC BcN =      %03x UHTR =    %03x (offset %x)\n", fed.BcN(), fed.UHTR_BcN(i), bcn_e);
	    if( orn_e) printf("  DTC OrN = %08x UHTR =     %02x (offset %x)\n", fed.OrN(), fed.UHTR_OrN(i), orn_e);
	  }
	}
      }
    }

    // check if AMC13 EvN are sequential
    if( check_evn) {
      if( evt_ct == 0) {
	last_evn = fed.EvN();
	printf("First EvN is %d (0x%06x)\n", last_evn, last_evn);
      } else {
	if( fed.EvN() != last_evn + 1) {
	  printf("After reading %d events, Last EvN was %d (0x%06x) current is %d (0x%06x)\n", evt_ct, last_evn, last_evn, fed.EvN(), fed.EvN() );
	  ++errz;
	}
      }
      last_evn = fed.EvN();
    }

    if( dump_level) 
      fed.Dump( dump_level);

    if( dump_level >= 10)
      fed.DumpRaw();

    ++evt_ct;
  }

  if( check_evn)
    printf("After reading %d events, Last EvN is %d (0x%06x)\n", evt_ct, last_evn, last_evn);

  if( !errz)
    printf("No errors\n");
  else
    printf("%d errors\n", errz);
  exit( 0);

  /*bail_out:*/
  printf("Error exit\n");
  exit( 1);
}


//
// compare dtc_n with htr_n using mask and offset
// return 0 if they match using *offset
// update offset and return new value on mis-match
//
int check_num_offset( uint32_t dtc_n, uint32_t htr_n, uint32_t mask, uint32_t *offset)
{
  uint32_t noff;
  /*static char msg[80];*/

  if( ((dtc_n + *offset) & mask) != htr_n) {
    noff = (htr_n - dtc_n) & mask;
    *offset = noff;
    return noff;
  } else
    return 0;
}
