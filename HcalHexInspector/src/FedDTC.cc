
// #define CRC16

#include "HcalDIM/HcalHexInspector/interface/FedDTC.hh"
#include <stdlib.h>		// for NULL
#include <stdio.h>
#include <string.h>

#include "HcalDIM/HcalHexInspector/interface/crc16d64.hh"
#include "HcalDIM/HcalHexInspector/interface/Adc2fC.hh"

#ifdef CRC16
#include "crc16.hh"
#else
#include "HcalDIM/HcalHexInspector/interface/my_crc16.hh"
#endif

FedDTC::FedDTC( uint32_t* raw, uint32_t size, uint32_t DCCver) {

  memset( this, 0, sizeof(FedDTC)); // clear everything (needed?)

  m_DCCrev = DCCver;
  m_raw = raw;
  m_raw16 = (uint16_t *)raw;
  m_size = size;
  if( Evt_Length() * 2 != m_size) {
    fprintf( stderr, "FedDTC():  Bad event length!  size=%d  CDF trailer=0x%08x\n",
	    size, Evt_Length() );
    fprintf( stderr, "FedDTC():  WARNING!  Proceeding to unpack corrupted event\n");
    printf( "FedDTC():  WARNING!  Proceeding to unpack corrupted event\n");
  }
  // build table of pointers to UHTR payloads
  uint16_t* p = UHTR_Payload();	// start of UHTR payloads
  for( int i=0; i<FEDGET_NUM_UHTR; i++) {
    if( UHTR_nWords(i)) {
      m_htr[i] = p;
      p += UHTR_nWords(i);
    } else
      m_htr[i] = NULL;
  }
}


uint16_t FedDTC::UHTR_CRC_Calc(int h) {

#ifdef CRC16
  uint8_t* p = (uint8_t *)UHTR_Data(h); // byte pointer
  int n = UHTR_nWords(h) * 4;	// byte count
  uint16_t crc = crc_update( 0xffff, &p[2], n-8);
#else

  uint16_t* p = (uint16_t *)UHTR_Data(h);	// byte pointer
  int n = UHTR_nWords(h) * 2;	// word count
  n -= 3;			// stop before Pre-trailer 2

  uint16_t crc = 0xffff;

  for( int i=0; i<n; i++) {
    if( i == 0)
      //      crc = crc16( crc, p[i] & 0xff);
      crc = 0xffff;
    else
      crc = crc16( crc, p[i]);
  }

#endif

  return crc;
}


void FedDTC::DumpRaw() {
  for( unsigned int k=0; k<m_size; k++) {
    if( (k % 4) == 0)
      printf("%06x: ", k);
    printf( " %08x", m_raw[k]);
    if( ((k+1) % 4) == 0)
      printf("\n");
  }
  if( (m_size % 4) != 0)
    printf("\n");
}

void FedDTC::DumpHtr(int h) {
  if( UHTR_nWords(h)) {
    uint16_t* p = (uint16_t*)m_htr[h];
    int size16 = UHTR_nWords(h)*2;
    for( int i=0; i<size16; i++) {
      printf( "%04d: %04x\n", i, p[i]);
    }
  }
}


// calculate CRC
uint16_t FedDTC::CalcCRC() {
  uint16_t crc = 0xFFFF;
    
  for (unsigned int i=0; i < m_size; i+=2) {
    if(i == m_size -2)
      crc = crc16d64_(m_raw[i+1], m_raw[i]&0xFFFF, crc);
    else
      crc = crc16d64_(m_raw[i+1], m_raw[i], crc);
  }

  return crc;
}


// Dump for debug to stdout
void FedDTC::Dump( int level) {
  Dump( stdout, level);
}

void FedDTC::Dump( FILE *fp, int level) {

  fprintf( fp, "FED: %3d EvN: %06x  BcN: %03x  OrN: %08x  TTS: %x/%x EvTyp: %x  CalTyp: %x Size: %d\n",
	 fed_id(), EvN(), BcN(), OrN(), TTS(), fTTS(), EvtTyp(), CalTyp(), m_size );
  if( CalcCRC() != CRC())
    fprintf( fp, "CRC Error!  cal=%08x  data=%08x\n", CalcCRC(), CRC() );

  // level 2, dump UHTR headers
  if( level >= 2) {
    for( int i=0; i<FEDGET_NUM_UHTR; i++) {
      int nw;
      if( (nw = UHTR_nWords( i))) {
	printf("UHTR %2d [%4d] EvN %06x BcN %03x OrN %02x\n", i, nw,
	       UHTR_EvN(i), UHTR_BcN(i), UHTR_OrN(i));
	// level 3, dump UHTR payloads
	if( level >= 3) {
	  for( int n=0; n<nw; n++)
	    printf("%3d: %04x\n", n, UHTR_Data(i)[n]);
	}
      }
    }
  }

}

