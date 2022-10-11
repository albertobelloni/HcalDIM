//
// Sort a 'deadbeef' file by EvN, FED
// report some statistics
//

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "HcalDIM/HcalHexInspector/interface/FedGet.hh"

#define DCC_VERS 0x2c18

typedef struct {
  uint32_t EvN;
  long pos;
  uint32_t fnum;
  int fed_id;
} an_evt;

#define EV_MAX 10000

#define HTR_MAX 0x400

// compare function for qsort()
int compare (const void * a, const void * b)
{
  return ( ((an_evt*)a)->EvN - ((an_evt*)b)->EvN );
}


int main( int argc, char *argv[])
{
  FILE *fp;
  char* data_file = 0; // was uninitialized
  uint32_t h[2];
  uint32_t d[EV_MAX];

  uint32_t start_dump_evn = 0;
  uint32_t dump_num_events = 5;

  unsigned int fed_id = 0;
  bool check_htrs = false;
  /*bool do_sort = false;*/
  bool dump_raw = false;
  bool dump_payload = false;
  bool check_evn = false;
  int dump_level = 1;

  int err_lim = 9999;

  static uint16_t htr_status[32][FEDGET_NUM_HTR];
  static uint8_t lrb_status[32][FEDGET_NUM_HTR];
  uint16_t htr_mask = 0;

  uint32_t htr_sample[FEDGET_NUM_HTR][HTR_MAX];
  int htr_nwords[FEDGET_NUM_HTR];
  static bool have_sample[FEDGET_NUM_HTR];

  long n_htr = 0;

  int n_err = 0;

  if( argc > 1) {
    for( int i=1; i<argc; i++) {
      if( *argv[i] == '-') {
	switch( toupper( argv[i][1])) {
	case 'R':
	  dump_raw = true;
	  break;
	case 'E':
	  check_evn = true;
	  break;
	case 'D':
	  if( i+1 >= argc) {
	    printf("Need starting EvN for dump after -d\n");
	    exit(1);
	  }
	  start_dump_evn = strtoul( argv[i+1], NULL, 0);
	  ++i;
	  break;
	case 'V':
	  if( i+1 >= argc) {
	    printf("Need dump level after -v\n");
	    exit(1);
	  }
	  dump_level = strtoul( argv[i+1], NULL, 0);
	  ++i;
	  break;
	case 'N':
	  if( i+1 >= argc) {
	    printf("Need event count after -n\n");
	    exit(1);
	  }
	  dump_num_events = strtoul( argv[i+1], NULL, 0);
	  ++i;
	  break;
	case 'M':
	  if( i+1 >= argc) {
	    printf("Need HTR status mask after -m\n");
	    exit(1);
	  }
	  check_htrs = true;
	  htr_mask = strtoul( argv[i+1], NULL, 0);
	  ++i;
	  break;
	case 'F':
	  if( i+1 >= argc) {
	    printf("Need FED id after -f\n");
	    exit(1);
	  }
	  fed_id = strtoul( argv[i+1], NULL, 0);
	  ++i;
	  break;
	case 'S':
	  /*do_sort = true;*/
	  break;
	case 'H':
	  check_htrs = true;
	  break;
	case 'P':
	  dump_payload = true;
	  break;
	}
      } else {
	data_file = argv[i];
      }
    }
  } else {
    printf("Usage: dump_sort [options] binary_file\n");
    printf("       [-d start_EvN] [-n no_events] [-f fed_id] \n");
    printf("       [-r (raw)] [-v dumplevel]\n");
    printf("       [-h (check htr)] [-m htr_mask]\n");
    printf("       [-e (check evn)]\n");
    printf("       [-P (dump bad HTR payloads)\n");
    exit(1);
  }

  if( (fp = fopen( data_file, "r")) == NULL) {
    printf("Error opening %s\n", data_file);
    exit( 1);
  }

  int nev = 0;
  printf("--Pass 1 - count events\n");
  while( fread( h, 4, 2,  fp) == 2) {
    if( h[0] != 0xdeadbeef) {
      printf("Expecting 0xdeadbeef, saw 0x%08x\n", h[0]);
      exit(1);
    }
    if( h[1] < 10 || h[1] > EV_MAX) {
      printf("Event size %d is wrong\n", h[1]);
      exit(1);
    }

    fread( d, sizeof(uint32_t), h[1], fp);

    // Do HTR checking on first pass to save time
    if( check_htrs) {
      FedGet fed( d, h[1], DCC_VERS);
      int ifed = fed.fed_id() - 700;
      if( fed_id == 0 || fed.fed_id() == fed_id) {
	for( int h=0; h<FEDGET_NUM_HTR; h++) {
	  if( fed.HTR_nWords(h)) {
	      htr_status[ifed][h] |= fed.HTR_status(h);
	      lrb_status[ifed][h] |= fed.HTR_lrb_err(h);
	      ++n_htr;

	      if( !have_sample[h]) {
		have_sample[h] = true;
		memcpy( htr_sample[h], fed.HTR_Data(h), fed.HTR_nWords(h) * sizeof(uint32_t));
		htr_nwords[h] = fed.HTR_nWords(h);
	      }
	      bool dump=false;

	      if( check_evn) {
		if( fed.HTR_EvN(h) != fed.EvN()) {
		  if( ++n_err < err_lim) {
		    printf("HTR %d EvN=%06x  DCC EvN=%06x\n", h, fed.HTR_EvN(h), fed.EvN());
		    dump = true;
		  }
		}
	      }
		  
	      if( htr_mask && (fed.HTR_status(h) & htr_mask)) {
		if( ++n_err < err_lim) {
		  printf("** HTR Mask matched\n");
		  dump = true;
		}
	      }

	      if( fed.HTR_lrb_err(h)) {
		if( ++n_err < err_lim) {		
		  printf("** HTR %d LRB Error %02x seen\n", h, fed.HTR_lrb_err(h));
		  dump = true;
		}
	      }

	      if( dump) {
		fed.Dump(2);
		if( dump_payload) {
		  printf("HTR %d payload:\n", h);
		  for( int k=0; k<fed.HTR_nWords(h); k++) {
		    uint16_t lo = *(fed.HTR_Data(h)+k) & 0xffff;
		    uint16_t hi = (*(fed.HTR_Data(h)+k) >> 16) & 0xffff;		    
		    if( k < htr_nwords[h] ) {
		      printf("%03d: [%04x] %04x\n     [%04x] %04x\n", k,
			     htr_sample[h][k] & 0xffff, lo, (htr_sample[h][k]>>16) & 0xffff, hi);
		    } else {
		      printf("%03d: [    ] %04x\n     [    ] %04x\n", k, lo, hi);
		    }
		  }
		}
		if( dump_raw)
		  fed.DumpRaw();
	      }
	  }
	}
      }
    }
    ++nev;
  }
  printf("%d Event records seen\n", nev);


  if( check_htrs) {
    printf("%ld HTRs checked\n", n_htr);
    for( int f=0; f<32; f++) {
      printf("Fed %3d: ", f+700);
      for( int h=0; h<FEDGET_NUM_HTR; h++) {
	if( htr_status[f][h])
	  printf("%04x%c", htr_status[f][h],
		 (htr_status[f][h] & 0x8f) ? '*' : ' ');
	else
	  printf("     ");
      }
      printf("\n");
      printf("         ");
      for( int h=0; h<FEDGET_NUM_HTR; h++) {
	if( lrb_status[f][h])
	  printf("  %02x ", lrb_status[f][h]);
	else
	  printf("     ");
      }
      printf("\n");
    }

    exit(0);
  }

  printf("--Pass 2 - read headers\n");
  rewind( fp);
  an_evt* ev = (an_evt*)calloc( nev, sizeof( an_evt));
  if( ev == NULL) {
    printf("Error allocating event structure array\n");
    exit(1);
  }

  nev = 0;
  long cpos = ftell(fp);

  while( fread( h, 4, 2,  fp) == 2) {
    if( h[0] != 0xdeadbeef) {
      printf("Expecting 0xdeadbeef, saw 0x%08x\n", h[0]);
      exit(1);
    }
    if( h[1] < 10 || h[1] > EV_MAX) {
      printf("Event size %d is wrong\n", h[1]);
      exit(1);
    }

    fread( d, sizeof(uint32_t), h[1], fp);

    ev[nev].EvN = d[1] & 0xffffff;
    ev[nev].pos = cpos;
    ev[nev].fnum = nev;
    int fed = (d[0]>>8) & 0xfff;
    ev[nev].fed_id = fed;
    ++nev;
    cpos = ftell(fp);
  }

  // sort by EvN
  printf("Sort by EvN...\n");
  qsort( ev, nev, sizeof(an_evt), compare);

  printf("--Pass 3... copy event data out in event order\n");
  printf("  Dumping from EvN %d...%d\n", start_dump_evn, start_dump_evn+dump_num_events);
  printf("  Looking for FED %d\n", fed_id);

  rewind( fp);
  for( int i=0; i<nev; i++) {
    fseek( fp, ev[i].pos, SEEK_SET);

    fread( h, 4, 2,  fp);
    if( h[0] != 0xdeadbeef) {
      printf("Expecting 0xdeadbeef, saw 0x%08x\n", h[0]);
      exit(1);
    }
    if( h[1] < 10 || h[1] > EV_MAX) {
      printf("Event size %d is wrong\n", h[1]);
      exit(1);
    }
    fread( d, sizeof(uint32_t), h[1], fp);
    FedGet fed( d, h[1], DCC_VERS);

    //    printf("Considering whether to dump EvN %d\n", fed.EvN());

    if( fed.EvN() >= start_dump_evn && fed.EvN() < (start_dump_evn +dump_num_events) &&
	(fed_id == 0 || fed.fed_id() == fed_id)) {
      printf("Record %d EvN %d (0x%x)\n", ev[i].fnum, ev[i].EvN, ev[i].EvN);
      if( dump_level < 3) {
	fed.Dump(dump_level);
      } else {
	fed.Dump(2);
	if( dump_level == 3)
	  fed.DumpRaw();
	else
	  fed.DumpAllHtr();
      }
    }
  }

  printf("%d Errors seen\n", n_err);

  return 0;

}
