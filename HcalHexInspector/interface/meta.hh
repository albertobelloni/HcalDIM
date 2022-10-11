#ifndef _META_HH_INCLUDED
#define _META_HH_INCLUDED
//
// Data format for saving all meta-data in event order
//

#define META_FILE_ID  0xb00bcafe
#define META_EVT_ID  0xb00bbabe
#define META_FED_ID  0xb00bafed

// Per-FED format
typedef struct {
  // CDF header items
  uint32_t EvN;
  uint32_t BcN;
  uint32_t fed_id;
  uint32_t Evt_Length;
  uint8_t TTS;

  // DCC header items
  uint32_t OrN;			// OrN from 2nd CDF header
  uint32_t DCCHdr0;		// 1st word after 2nd CDF header
  uint32_t DCCHdr1;		// 2nd word after 2nd CDF header

  // per-HTR items
  uint32_t HTR_nWords[FEDGET_NUM_HTR];		// nWords from DCC
  uint32_t HTR_hdr[FEDGET_NUM_HTR][4];		// entire HTR header
  uint32_t HTR_trl[FEDGET_NUM_HTR][2];		// entire HTR trailer
} a_fed;


void copy_meta( FedGet& fed, a_fed* meta);
void my_dump_meta( a_fed* meta, int dump_level);


#endif
