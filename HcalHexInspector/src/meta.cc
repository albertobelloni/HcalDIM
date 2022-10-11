
#include <stdio.h>
#include <strings.h>

#include "HcalDIM/HcalHexInspector/interface/FedGet.hh"
#include "HcalDIM/HcalHexInspector/interface/meta.hh"

void copy_meta( FedGet& fed, a_fed* meta)
{
  uint32_t* h;
  uint16_t hn;

  bzero( meta, sizeof(a_fed));

  meta->EvN = fed.EvN();
  meta->BcN = fed.BcN();
  meta->fed_id = fed.fed_id();
  meta->Evt_Length = fed.Evt_Length();
  meta->TTS = fed.TTS();
  meta->OrN = fed.OrN();
  meta->DCCHdr0 = fed.DCCHdr2();
  meta->DCCHdr1 = fed.DCCHdr3();
  for(int i=0; i<FEDGET_NUM_HTR; i++) {
    hn = meta->HTR_nWords[i] = fed.HTR_nWords(i);
    h = fed.HTR_raw(i);
    if( hn >= 6) {
      for( int j=0; j<4; j++)
	meta->HTR_hdr[i][j] = h[j];
      meta->HTR_trl[i][0] = h[hn-2];
      meta->HTR_trl[i][1] = h[hn-1];
    } else {
      if( h != NULL) {
	meta->HTR_hdr[i][0] = h[0];
	meta->HTR_trl[i][1] = h[1];
      }
    }
  }
}


void my_dump_meta( a_fed* meta, int dump_level) {
  printf("META: Evn 0x%06x OrN %06x BcN %03x Fed %3d Len %4d TTS %x DCCh %08x %08x\n",
	 meta->EvN, meta->OrN, meta->BcN, meta->fed_id,
	 meta->Evt_Length, meta->TTS, meta->DCCHdr0, meta->DCCHdr1 );
  if( dump_level > 1) {
    for( int i=0; i<FEDGET_NUM_HTR; i++) {
      printf("    HTR %2d NW %3d HDR %08x %08x %08x %08x TRL %08x %08x\n",
	     i, meta->HTR_nWords[i],
	     meta->HTR_hdr[i][0], meta->HTR_hdr[i][1], meta->HTR_hdr[i][2], meta->HTR_hdr[i][3],
	     meta->HTR_trl[i][0], meta->HTR_trl[i][1]);
    }
  }
}
