//
// simple dump utility for CDF-format raw data
//   (using DCCv4, HTRv3)
//
// expects one 32-bit word with the word count to precede
// the CDF header in each event
//

#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <stdint.h>
#include <string.h>

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


void usage() {
  printf("Revision 09 Oct 2008\n");
  printf("usage: dump_FED [-v verbose] [-e (check EvN)] [-o (check OrN)]\n");
  printf("                [-d dump_level]\n");
  printf("                [-f fed_no]\n");
  printf("                [-c (check FED CRC)]\n");
  printf("                [-x (check HTR CRC)]\n");
  printf("                [-b (check BcN)] [-s (check HTRs)]\n");
  printf("                [-q (types)] Check QIE.  Type = A (all) E (err) C (capid) L (len)\n");
  printf("                [-t (show TTS)]\n");
  printf("                [-r first_evn [last_evn]]\n");
  printf("                [-y mask] check DCC E,P,B,V,T,C bits (mask hex from 0x3f)\n");
  printf("                [-h (special hamming check)]\n");
  printf("                [-u (unsupressed events only)]\n");
  printf("                [-z (special output for ROOT to root.dat)\n");
  printf("Dump levels:\n");
  printf("  1 - DCC header only  3 - TP              2 - HTR headers\n");
  printf("  4 - HTR payloads (formatted)\n");
  printf("  5 - QIE data         6 - payloads+QIE    7 - QIE raw\n");
  printf("  8 - QIE summary      9 - QIE All!\n");
}

// define size of arrays for DAQ and TP samples (actually 20, but be safe!)
#define MAX_SAMP 32

#define MAX_HTR_PAYLOAD 0x800

void dump_evt( DWORD* my_evt, DWORD words, int verbosity);
const char *show_tts( int t);
int check_hamming( uint32_t* dat, uint32_t nw, int fed, int htr);

static long fposn;		// file position for error reports
static int ham_err[HCAL_NUM_FED][FEDGET_NUM_HTR][MAX_HTR_PAYLOAD];
static uint32_t ham_bits[HCAL_NUM_FED][FEDGET_NUM_HTR][MAX_HTR_PAYLOAD];
static int nHtrCrcErr[HCAL_NUM_FED][FEDGET_NUM_HTR];
static int verbose = 0;
static int hamming = 0;
static int root_output = 0;
static unsigned int only_fed = 0;
static int qie_check = 0;

int main( int argc, char *argv[])
{
  FILE *fp = NULL;
  FILE *rf = NULL;

  //int nev = 0;

  static int dump_level = 0;
  bool check_evn = false;
  bool check_bcn = false;
  bool check_orn = false;
  bool check_num = false;
  bool check_crc = false;
  bool check_htr_crc = false;
  bool unsup = false;
  bool unsup_evt = false; // was not initialized

  bool check_tts = false;

  uint32_t htr_status = 0;
  uint32_t lrb_status = 0;
  uint32_t dcc_htr_status = 0;

  uint32_t htr_evn[FEDGET_NUM_HTR];

  uint32_t first_evn = 1;
  uint32_t last_evn = 0xffffff;

    // command line controls
  if( argc > 1) {
    for( int i=1; i<argc; i++) {
      if( *argv[i] == '-') {
	switch( toupper( argv[i][1])) {

	case 'Q':
	  if( i < (argc-1) && isalpha(*argv[i+1])) { // list of options?
	    char *s = argv[i+1];
	    for( unsigned int k=0; k<strlen(s); k++) {
	      if( isalpha( s[k])) {
		switch( toupper( s[k])) {
		case 'E':
		  qie_check |= QIE_STATUS_ERR;
		  break;
		case 'C':
		  qie_check |= QIE_STATUS_CAPID;
		  break;
		case 'L':
		  qie_check |= QIE_STATUS_LEN;
		  break;
		case 'A':
		  qie_check |= (0xff - QIE_STATUS_OK - QIE_STATUS_EMPTY);
		  break;
		default:
		  printf("Unknown character in -q string.  Only E, C, L are valid\n");
		}
	      }
	    }
	  } else {		// no, take defaults
	    qie_check = QIE_STATUS_ERR | QIE_STATUS_CAPID | QIE_STATUS_LEN;
	  }
	  break;

	case 'U':
	  unsup = true;
	  break;

	case 'F':
	  if( i < (argc-1) && isdigit( *argv[i+1])) {
	    ++i;
	    only_fed = atoi( argv[i]);
	  } else {
	    printf("Need FED ID after -f\n");
	    exit(1);
	  }
	  break;

	case 'R':
	  if( i < (argc-1)) {
	    ++i;
	    first_evn = strtoul( argv[i], NULL, 0);
	    if( i < (argc-1)) {
	      if( isdigit( *argv[i+1])) {
		++i;
		last_evn = strtoul( argv[i], NULL, 0);
	      }
	    }
	  } else
	    printf("Usage: -r first_evn [last_evn]\n");
	  break;

	case 'X':
	  check_htr_crc = true;
	  break;

	case 'C':
	  check_crc = true;
	  break;

	case 'H':		// set hamming check
	  if( isdigit( argv[i][2]))
	    hamming = atoi( &argv[i][2]);
	  else
	    ++hamming;
	  break;

	case 'T':
	  check_tts = true;
	  break;

	case 'O':
	  check_orn = true;
	  break;

	case 'E':
	  check_evn = true;
	  break;
	  
	case 'B':
	  check_bcn = true;
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

	case 'Z':
	  if( i < (argc-1)) {
	    if( isdigit( *argv[i+1])) {
	      ++i;
	      root_output = atoi( argv[i]);
	    } else
	      ++root_output;	      
	  } else
	    ++root_output;
	  break;

	case 'V':		// set verbosity
	  if( isdigit( argv[i][2]))
	    verbose = atoi( &argv[i][2]);
	  else
	    ++verbose;
	  break;

	case 'Y':		// DCC epvtc mask
	  if( i < (argc-1) && isdigit(argv[i+1][0])) {
	    dcc_htr_status = strtoul( argv[i+1], NULL, 0);
	    ++i;
	  } else
	    dcc_htr_status = 0x3f;	// default E*P*B*V*/T*/C
	  break;

	case 'S':
	  if( i < (argc-1) && isdigit(argv[i+1][0])) {
	    htr_status = strtoul( argv[i+1], NULL, 0);
	    ++i;
	    if( i < (argc-1) && isdigit(argv[i+1][0])) {
	      lrb_status = strtoul( argv[i+1], NULL, 0);
	      ++i;
	    }
	  } else {
	    htr_status = 0x8f;
	    lrb_status = 0xff;
	  }
	  break;

	default:
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
  } else
    usage();

  if( fp == NULL) {
    printf("Need an input file!\n");
    exit( 1);
  }

  if( only_fed) printf("Looking only at FED %d\n", only_fed);
  if( check_evn) printf("Checking for matching EvN\n");
  if( check_bcn) printf("Checking for matching BcN\n");
  if( check_orn) printf("Checking for matching OrN\n");
  if( check_htr_crc) printf("Checking HTR CRCs\n");

  if( htr_status) printf("Looking for HTR status bits mask %04x, LRB error mask %02x\n", htr_status, lrb_status);
  if( root_output) printf("Special summary output to root.dat\n");
  printf("hamming=%d\n", hamming);

  if( qie_check) {
    int i, b;
    printf("Check for QIE errors: ");
    for( i=0, b=1; i<QIE_STATUS_MAX; i++, b<<=1)
      if( qie_check & b)
	printf(" %s", QIE_status_name(i));
    printf("\n");
  }

  if( root_output) {
    if( (rf = fopen( "root.dat", "w")) == NULL) {
      printf("Error opening root.dat\n");
      exit(1);
    }
  }

  // set dump_level if not already set by user for
  // dumps of mis-matches
  if( check_evn || check_bcn || check_orn) {
    check_num = true;
    if( dump_level == 0)
      dump_level = 4;
  }

  DWORD my_hdr[2];		// my header - sentinel plus word count
  DWORD my_evt[MAX_WORDS];
  int evt_ct = 0;
  int evt_span = 0;
  int evt_first = 0;
  int evt_last = 0;
  uint64_t ev_t0 = 0;
  uint64_t ev_tsum = 0;
  int lastTTS = 0;

  uint64_t ev_t_first = 0, ev_t_last = 0; // were not initialized

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
    if( (rc = fread( my_evt, sizeof(DWORD), my_hdr[1], fp)) != my_hdr[1]) {
      printf("Short read... tried for %d, got %d words\n", my_hdr[1], rc);
      goto bail_out;
    }

    FedGet* fed;

  try_again:

    try {
      FedGet try_fed( my_evt, my_hdr[1], DCC_VER);
      fed = &try_fed;
    } catch( int err) {
      if( err == 1) {
	printf( "Exception in FedGet::FedGet(): bad event length\n");
	printf( "File event size: 0x%x\n", my_hdr[1]);
	printf( "Dumping what we have:\n");
	for( unsigned int i=0; i<my_hdr[1]; i++) {
	  if( (i % 8) == 0)
	    printf("%04x: ", i);
	  printf( " %08x", my_evt[i]);
	  if( (i % 8) == 7)
	    printf( "\n");
	}
	printf("\n");
	printf("-------------------\n");

	printf( "Skipping event... rewind to offset %ld\n", fposn);
	fseek( fp, fposn, SEEK_SET); // back to start of this event
	if( fread( my_hdr, sizeof(my_hdr), 1, fp) == 1) {
	  // now just look for next deadbeef
	  while( fread( &my_hdr[0], sizeof( my_hdr[0]), 1, fp) == 1) {
	    if( my_hdr[0] == MY_SENTINEL) {
	      printf("found next header at offset 0x%ld\n", ftell( fp));
	      break;
	    }
	  }
	  if( my_hdr[0] == MY_SENTINEL) {
	    fposn = ftell( fp) - sizeof( my_hdr[0]);;
	    fread( &my_hdr[1], sizeof( my_hdr[1]), 1, fp);
	    if( (rc = fread( my_evt, sizeof(DWORD), my_hdr[1], fp)) != my_hdr[1]) {
	      printf("Short read... tried for %d, got %d words\n", my_hdr[1], rc);
	      goto bail_out;
	    }
	    goto try_again;
	  } else {
	    printf( "Read to end of file, didn't find next header\n");
	    exit(1);
	  }
	} else {
	  printf( "Error re-reading header\n");
	  exit(1);
	}
      } else {
	printf( "Exception in FedGet::FedGet(): unknown exception\n");
	exit( 1);
      }
    }

    if( hamming)
      fed->SetHamming(  true);

    if( (only_fed == 0 || only_fed == fed->fed_id()) &&
	(fed->EvN() >= first_evn && fed->EvN() <= last_evn) )  {

      bool dump_this = false;

      if( evt_ct == 0) {		// first event
	evt_first = fed->EvN();
	ev_t_first = (uint64_t)fed->OrN() * (uint64_t)LHC_BUNCHES + fed->BcN();
      }
      evt_last = fed->EvN();
      ev_t_last = (uint64_t)fed->OrN() * (uint64_t)LHC_BUNCHES + fed->BcN();      

      uint64_t ev_t = (uint64_t)fed->OrN() * (uint64_t)LHC_BUNCHES + fed->BcN();
      if( ev_t0) {
	uint64_t tdiff = ev_t - ev_t0;
	ev_tsum += tdiff;
	if( ((!unsup || unsup_evt) && (dump_level >= 2 && !check_num)) || dump_this) {
	  cout << "Time since last event: " << (ev_t-ev_t0) << " (total " << ev_tsum << ")" << endl;
	}
      }
      ev_t0 = ev_t;

      if( check_tts) {
	if( fed->fTTS() != lastTTS) {
	  printf("EvN %d TTS %s -> %s\n", fed->EvN(), show_tts(lastTTS), show_tts(fed->fTTS()) );
	  lastTTS = fed->fTTS();
	}
      }

      if( check_crc) {
	if( fed->CRC() != fed->CalcCRC()) {
	  printf("CDF CRC Error at EvN %d FED %d\n", fed->EvN(), fed->fed_id());
	}
      }

      if( root_output)
	fprintf( rf, "EVN %d FED %d OrN %d BcN %d\n", fed->EvN(), fed->fed_id(), fed->OrN(), fed->BcN() );

      if( unsup || qie_check || root_output || check_orn || check_evn || check_bcn || htr_status || lrb_status || hamming || check_htr_crc || dcc_htr_status) {
	if( unsup)
	  unsup_evt = false;

	for( int h=0; h<FEDGET_NUM_HTR; h++) {
	  if( fed->HTR_nWords(h)) {

	    if( fed->HTR_us(h))
	      unsup_evt = true;

	    if( check_htr_crc) {
	      uint16_t calc_htr_crc = fed->HTR_CRC_Calc(h);
	      if( calc_htr_crc != fed->HTR_CRC(h)) {
		printf("EvN %d FED %d HTR %d CRC error calculated: %04x stored: %04x\n",
		       fed->EvN(), fed->fed_id(), h, calc_htr_crc, fed->HTR_CRC(h));
		++nHtrCrcErr[fed->fed_id()-HCAL_MIN_FED][h];
	      }
	    }

	    if( qie_check)
	      for( int c=0; c<FEDGET_NUM_CHAN; c++)
		if( qie_check & fed->HTR_QIE_status( h, c)) {
		  printf("FED %3d EvN 0x%06x BcN: 0x%03x HTR %2d Ch %2d QIE Errors = %s\n",
			 fed->fed_id(), fed->EvN(), fed->BcN(), h, c,
			 QIE_status_names(fed->HTR_QIE_status(h,c)) );
		}

	    if( root_output) {
	      fprintf( rf, "HTR %d", h);
	      for( int c=0; c<FEDGET_NUM_CHAN; c++)
		fprintf( rf, " %d", fed->HTR_QIE_status( h, c));
	      fprintf( rf, "\n");
	    }

	    if( hamming) {
	      int fid = fed->fed_id();
	      if( fid > HCAL_MIN_FED)
		fid -= HCAL_MIN_FED;
	      int rc = check_hamming( fed->HTR_Data(h), fed->HTR_nWords(h), fid, h);
	      if( rc && (hamming > 1))
		printf("...in FED %d HTR %2d EvN %d (payload length %d)\n",
		       fed->fed_id(), h, fed->EvN(), fed->HTR_nWords(h) );
	    }


	    if( check_evn) {

	      // first event, just check against DCC
	      if( evt_ct ==0) {
		htr_evn[h] = fed->HTR_EvN(h);
	      } else {
		htr_evn[h]++;
		// just check HTR vs DCC
		//		if( fed->HTR_EvN(h) != htr_evn[h] || fed->EvN() != fed->HTR_EvN(h)) {
		if( fed->EvN() != fed->HTR_EvN(h)) {
		  printf("ERR Event Number FED= 0x%06x (%d)  HTR(%d) EvN=0x%06x (%d) [last=0x%06x]\n",
			 fed->EvN(), fed->EvN(), h, fed->HTR_EvN(h), fed->HTR_EvN(h),
			 htr_evn[h]);
		  dump_this = true;
		  htr_evn[h] = fed->HTR_EvN(h);
		}
	      }
	    }

	    if( check_bcn) {
	      if( fed->HTR_BcN(h) != fed->BcN()) {
		printf("ERR Event 0x%06x (%d)  HTR(%d) BcN=0x%03x  DCC BcN=0x%03x\n",
		       fed->EvN(), fed->EvN(), h, fed->HTR_BcN(h), fed->BcN() );
		dump_this = true;
	      }
	    }

	    if( check_orn) {
	      if( (fed->OrN() & 0x1f) != (fed->HTR_OrN(h) & 0x1f)) {
		printf("ERR Event 0x%06x (%d)  HTR(%d) OrN=0x%03x  DCC OrN=0x%03x\n",
		       fed->EvN(), fed->EvN(), h, fed->HTR_OrN(h), fed->OrN() );
		dump_this = true;
	      }
	    }

	    if( htr_status || lrb_status) {
	      if( (fed->HTR_status(h) & htr_status) != 0 ||
		  (fed->HTR_lrb_err(h) & lrb_status) != 0) {
		printf("ERR HTR(%d) status = %04x  LRB errors = %02x\n", h, fed->HTR_status(h), fed->HTR_lrb_err(h));
		dump_this = true;
	      }
	    }
	    
	    if( dcc_htr_status) {
	      if( ((fed->HTR_epbvtc(h) ^ 0x3c) & dcc_htr_status) != 0) {
		printf("ERR HTR(%d) DCC HTR status (E,P,B,V,T,C) = %02x\n",
		       h, fed->HTR_epbvtc(h));
	      }
	    }

	  }


	}
	if( root_output)
	  fprintf( rf, "END\n");
      }

      //// OLD:
      //if( dump_this || (!unsup || unsup_evt) && (!check_num && (dump_level != 0))) {
      //// NEW:
      if( dump_this || ((!unsup || unsup_evt) && (!check_num && (dump_level != 0)))) {
	fed->Dump( dump_level);
      }

      ++evt_ct;
      fposn = ftell( fp);
      if( !(evt_ct % 10000))
	printf("%d events processed\n", evt_ct);
    }

  }

 bail_out:

  evt_span = evt_last - evt_first;

  printf("Processed 0x%x (%d) events with EvN span %d (%d-%d)\n", evt_ct, evt_ct, evt_span,
	 evt_first, evt_last);
  cout << "Total elapsed BX: " << ev_tsum << endl;
  cout << "Time span (BX):   " << (ev_t_last-ev_t_first) << endl;

  double avg_bx = (double)ev_tsum / (double)evt_span;
  double avg_rate = 1.0 / (25e-9 * avg_bx);
  printf("Average spacing = %g   Average rate = %g Hz\n",
	 avg_bx, avg_rate);

  if( hamming) {
    for( int f=0; f<HCAL_NUM_FED; f++)
      for( int h=0; h<FEDGET_NUM_HTR; h++)
	for( int i=0; i<MAX_HTR_PAYLOAD; i++)
	  if( ham_err[f][h][i]) {
	    printf("FED %3d HTR %2d Word %3d   Errors: %6d  Bits: %08x\n",
		   f, h, i, ham_err[f][h][i], ham_bits[f][h][i]);
	  }
  }
  
  if( check_htr_crc) {
    bool errs = false;
    for( int f=0; f<HCAL_NUM_FED; f++) {
      for( int h=0; h<FEDGET_NUM_HTR; h++) {
	if( nHtrCrcErr[f][h]) {
	  errs = true;
	  printf("FED: %d  HTR %2d had %d HTR CRC errors\n", f+HCAL_MIN_FED, h, nHtrCrcErr[f][h]);
	}
      }
    }
    if( !errs)
      printf("There are no HTR CRC errors\n");
  }
}



const char *show_tts( int t)
{
  switch( t) {
  case 1:
    return "OFW";
  case 2:
    return "SYN";
  case 4:
    return "BSY";
  case 8:
    return "RDY";
  default:
    return "ERR";
  }
}






//
// check hamming in specially-formatted data from LRB test version
//
//   d   - pointer to 32-bit HTR payload
//  nw   - number of words to examine
// fed   - FED number 0-31 for display
// htr   - HTR spigot number 0-14 for display
//
int check_hamming( uint32_t* d, uint32_t nw, int fed, int htr) {
  int h0, h1, h2, h3, h4, p;
  int rc = 0;

  uint32_t t;
  uint32_t c, c1, c2;

  for( unsigned int i=1; i<nw; i++) {

    h0 = ((d[i]>>0)&1) ^
      ((d[i]>>1)&1) ^
      ((d[i]>>3)&1) ^
      ((d[i]>>4)&1) ^
      ((d[i]>>6)&1) ^
      ((d[i]>>8)&1) ^
      ((d[i]>>10)&1) ^
      ((d[i]>>11)&1) ^
      ((d[i]>>13)&1) ^
      ((d[i]>>15)&1) ^
      ((d[i]>>17)&1);
    h1 = ((d[i]>>0)&1) ^
      ((d[i]>>2)&1) ^
      ((d[i]>>3)&1) ^
      ((d[i]>>5)&1) ^
      ((d[i]>>6)&1) ^
      ((d[i]>>9)&1) ^
      ((d[i]>>10)&1) ^
      ((d[i]>>12)&1) ^
      ((d[i]>>13)&1) ^
      ((d[i]>>16)&1) ^
      ((d[i]>>17)&1);
    h2 = ((d[i]>>1)&1) ^
      ((d[i]>>2)&1) ^
      ((d[i]>>3)&1) ^
      ((d[i]>>7)&1) ^
      ((d[i]>>8)&1) ^
      ((d[i]>>9)&1) ^
      ((d[i]>>10)&1) ^
      ((d[i]>>14)&1) ^
      ((d[i]>>15)&1) ^
      ((d[i]>>16)&1) ^
      ((d[i]>>17)&1);
    h3 = ((d[i]>>4)&1) ^
      ((d[i]>>5)&1) ^
      ((d[i]>>6)&1) ^
      ((d[i]>>7)&1) ^
      ((d[i]>>8)&1) ^
      ((d[i]>>9)&1) ^
      ((d[i]>>10)&1);
    h4 = ((d[i]>>11)&1) ^
      ((d[i]>>12)&1) ^
      ((d[i]>>13)&1) ^
      ((d[i]>>14)&1) ^
      ((d[i]>>15)&1) ^
      ((d[i]>>16)&1) ^
      ((d[i]>>17)&1);
    p = ((d[i]>>0)&1) ^
      ((d[i]>>1)&1) ^
      ((d[i]>>2)&1) ^
      ((d[i]>>4)&1) ^
      ((d[i]>>5)&1) ^
      ((d[i]>>7)&1) ^
      ((d[i]>>10)&1) ^
      ((d[i]>>11)&1) ^
      ((d[i]>>12)&1) ^
      ((d[i]>>14)&1) ^
      ((d[i]>>17)&1);

    t =				// calculated word with hamming
      (d[i]&0x3ffff) |
      (h0<<18) |
      (h1<<19) |
      (h2<<20) |
      (h3<<21) |
      (h4<<22) |
      (p<<23);

    c = d[i];
    c1 = (d[i] ^ t) & 0xffffff;	// difference between calculated and actual
    c2 = c & 0xc0000000;	// LRB reported error flags

    if( c1 || c2) {

      if( hamming == 2) {
	printf("FED %3d HTR %3d Word %3d: 0x%08x  %08x  %08x\n", fed, htr, i, c, c1, c2);
      } else if( hamming > 2) {
	// dump data stream
	printf("FED %3d HTR %3d Word %3d: 0x%08x  %08x  %08x\n", fed, htr, i, c, c1, c2);
	printf("Context:\n");
	for( unsigned int k=max(i-5,1); k<=min(nw,i+5); k++) {
	  printf("FED %3d HTR %3d Word %3d: 0x%08x %s\n", fed, htr, k, d[k],
		 (k==i)?"<-- error":"");
	}

      }

      ++rc;
      ++ham_err[fed][htr][i];
      ham_bits[fed][htr][i] |= c1;
    }
  }
  return rc;
}
