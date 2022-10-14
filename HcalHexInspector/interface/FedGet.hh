#ifndef _MY_FEDGET_HH
#define _MY_FEDGET_HH

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

// Ignore FE data errors <= this BCN
#define FEDGET_ORBIT_MSG_BCN 2

#define FEDGET_NUM_HTR 15
#define FEDGET_NUM_CHAN 24
#define HCAL_NUM_FED 32
#define HCAL_MIN_FED 700

#define HCAL_MAX_TS 20

// decode bits of QIE
uint8_t QIE_mant(uint16_t r);
uint8_t QIE_range(uint16_t r);
uint8_t QIE_capID(uint16_t r);
uint8_t QIE_chipID(uint16_t r);
uint8_t QIE_fiberID(uint16_t r);
uint8_t QIE_err(uint16_t r);
uint8_t QIE_dv(uint16_t r);

const char* QIE_status_name( int s);
const char* QIE_status_names( int v);

/** \brief Class to access items within HCAL FED Data with minimal unpacking
 *
 * The constructor takes a pointer to the raw data and it's length.
 * An array of pointers to HTR payloads is set up for quick access.
 * Accessor functions are provided to the various parts of the data payload.
 * An array of pointers to each HTR's channel data is calculated on demand.
 *
 * Some error-checking functions are provided.
 *
 * NOTE!  Does not currently take into account format variations due to
 * HTR firmware revisions.
 */

class FedGet {
public:
  //! Unpack a FED payload.  Set up pointers for fast access.
  /// \param raw raw 32-bit data (first word of CDF header)
  /// \param size size of data in 32-bit words
  /// \param DCC firmware version (16 bits)
  FedGet( uint32_t* raw, uint32_t size, uint32_t DCCversion);

  //! Pointer to raw data
  uint32_t* raw() { return m_raw; }
  //! Size of data in 32-bit words
  uint32_t size() { return m_size; }

  //! Set hamming mode
  void SetHamming( bool h) { m_hamming=h; }

  //! Calculate CRC on data (compare with value returned by FedGet::CRC())
  uint16_t CalcCRC();
 
  uint32_t EvtTyp() { return (m_raw[1]>>24) & 0xf; }  ///< Get Event type from CDF header
  uint32_t EvN() { return m_raw[1] & 0xffffff;  } ///< Get EvN from CDF header 
  uint32_t BcN() { return (m_raw[0] >> 20) & 0xfff;  } ///< Get BcN from CDF header
  uint32_t fed_id() { return (m_raw[0] >> 8) & 0xfff; } ///< Get FED ID from CDF header
  uint16_t CRC() { return (m_raw[m_size-2] >> 16) & 0xffff; } ///< Get CRC from CDF trailer
  //! Get event length (64-bit words) from CDF trailer
  uint32_t Evt_Length() { return m_raw[m_size-1] & 0xffffff; }
  uint8_t TTS() { return (m_raw[m_size-2] >> 4) & 0xf; } ///< Get TTS state from CDF trailer
  uint8_t fTTS() { return (m_raw[4] >> 8) & 0xf; } ///< Get TTS state from DCC header

  //! Get orbit number from DCC header (N.B. firmware version dependent!)
  uint32_t OrN() { return ( (m_DCCrev>=0x2c10) ?
			    (((m_raw[2] >> 4) & 0xfffffff) |
			     ((m_raw[3] << 28) & 0xf0000000))
			    : (m_raw[2] & 0xffffffff)); }

  //! Get calibration type from DCC header
  uint32_t CalTyp() { return (m_raw[3] >> 24) & 0xf; }

  //! Get 1st DCC header low 32 bits (2nd CDF header)
  uint32_t DCCHdr0() { return m_raw[2]; }
  //! Get 1st DCC header high 32 bits (2nd CDF header)
  uint32_t DCCHdr1() { return m_raw[3]; }
  //! Get 2nd DCC header low 32 bits (DCC summary word)
  uint32_t DCCHdr2() { return m_raw[4]; }
  //! Get 2nd DCC header high 32 bits (DCC summary word)
  uint32_t DCCHdr3() { return m_raw[5]; }

  //! HTR header inserted by DCC for HTR h = 0..14
  uint32_t HTR_Hdr( int h) { return m_raw[6+h]; }
  //! Number of (32 bit) words for HTR
  uint16_t HTR_nWords( int h) { return HTR_Hdr(h) & 0x3ff; }
  //! HTR empty event
  bool HTR_EE(int h) { return (HTR_nWords(h) <= 4); }

  //! HTR status bits (E,P,B,V,T,C) field from DCC header
  uint16_t HTR_epbvtc( int h) { return (HTR_Hdr(h) >> 10) & 0x3f; }
  //! HTR status/error bits (low 8 bits only) from DCC header
  uint8_t HTR_err( int h) { return (HTR_Hdr(h) >> 24) & 0xff; }
  //! HTR LRB error bits from DCC header
  uint8_t HTR_lrb_err( int h) { return (HTR_Hdr(h) >> 16) & 0xff; }
  //! HTR CRC from trailer
  uint16_t HTR_CRC(int h) { return HTR_EE(h) ? 0 : ( (HTR_End(h)[-2] >> 16) & 0xffff); }
  //! HTR CRC - calculated
  uint16_t HTR_CRC_Calc(int h);

  // HTR payload area
  uint32_t* HTR_Payload() { return &m_raw[24]; } ///< pointer to start of 1st HTR payload
  uint32_t* HTR_Data(int h) { return m_htr[h]; } ///< pointer to particular HTR payload
  uint32_t* HTR_End(int h) { return &m_htr[h][HTR_nWords(h)]; } ///< pointer to HTR end + 1

  // HTR items -- ok for even EE
  //! EvN from HTR
  uint32_t HTR_EvN(int h) { return (m_htr[h][0] & 0xff) | ((m_htr[h][0] >> 8) & 0xffff00); }
  //! BcN from HTR
  uint16_t HTR_BcN(int h) { return m_htr[h][2] & 0xfff; }
  //! OrN from HTR
  uint8_t HTR_OrN(int h) { return (m_htr[h][1] >> 27) & 0x1f; }
  //! Submodule ID from HTR
  uint16_t HTR_id(int h) { return (m_htr[h][1] >> 16) & 0x3ff; }
  //! 16-bit HTR status/error word
  uint16_t HTR_status(int h) { return m_htr[h][1] & 0xffff; }

  // These are all zero/null for EE
  //! Number of trigger primitives
  uint16_t HTR_ntp(int h) { return HTR_EE(h) ? 0 : ((m_htr[h][2] >> 24) & 0xff); }
  //! Number of DAQ data
  uint16_t HTR_ndd(int h) { return HTR_EE(h) ? 0 : (HTR_End(h)[-2] & 0x3ff); }
  //! Number of time samples
  uint8_t HTR_ns(int h) { return HTR_EE(h) ? 0 : ((HTR_End(h)[-2] >> 11) & 0x1f); }
  //! Unsuppressed data flag
  bool HTR_us(int h) { return HTR_EE(h) ? false : ( ((m_htr[h][3] >> 15) & 1) != 0); }
  // QIE data
  //! Pointer to start of TP for HTR
  uint16_t* HTR_TP(int h) { return HTR_EE(h) ? NULL : (uint16_t*)(&m_htr[h][4]); }
  //! Pointer to start of QIE data for HTR
  uint16_t* HTR_QIE(int h) { return HTR_EE(h) ? NULL : HTR_TP(h)+HTR_ntp(h); }
  //! Pointer to QIE data for a particular channel
  uint16_t* HTR_QIE_chan(int h, int c);
  //! Number of QIE data for a particular channel
  int HTR_QIE_chan_len(int h, int c) { return HTR_EE(h) ? 0 : m_chn_len[h][c]; }
  //! Raw ADC data (0..127) samples in int array.  Return no. samples
  int HTR_QIE_Adc( int h, int c, int *Adc);
  //! Convert QIE data for a channel to fC in user-supplied buffer.  Return no. samples.
  int HTR_QIE_fC( int h, int c, double *fC);
  //! Calculate sum of all QIE samples (0 if no data)
  double HTR_QIE_sum_fC( int h, int c);
  //! Calculate mean of all QIE samples (0 if no data)
  double HTR_QIE_mean_fC( int h, int c);
  //! Calculate ZS critirium for a channel ("Sliding pair sum max"). --> Returns -1 on error
  int HTR_QIE_zs_sum(int h, int c);
  ///! Number of non-zero data channels
  int HTR_nonZch( int h);

  ///! Tag for a TP
  int TP_Tag( uint16_t tp) { return (tp >> 11) & 0x1f; }
  ///! Z bit for a TP
  int TP_Z( uint16_t tp) { return (tp>>10)&1; }
  //! SOI bit for a TP
  int TP_SOI( uint16_t tp) { return (tp>>9)&1; }
  //! TP data for a TP
  int TP_data( uint16_t tp) { return (tp & 0x1f); }

  //! Check if capID rotated for a HTR, channel
  bool good_capID(int h, int c);

  //! check status of a channel and return a bit mask
  // 
  int HTR_QIE_status(int h, int c);

  // number of bit values used
#define QIE_STATUS_MAX 5
  // individual error codes
#define QIE_STATUS_ERR 1
#define QIE_STATUS_CAPID 2
#define QIE_STATUS_EMPTY 4
#define QIE_STATUS_LEN 8
#define QIE_STATUS_OK 0x10

  //! Dump for debug
  void Dump( int level);	///< dump 0=nothing 1=DCC 2=HTR headers
                                // 3=TP 4=formatted 5=qie 6=both 7=QIE raw  8=QIE status
  void Dump( FILE* fp, int level); ///< dump to file
  void DumpAllQIE();		///< dump formatted QIE for all HTR
  void DumpHtrQIE( int h);	///< dump formatted QIE for one HTR
  void DumpQIE( int h, int c);	///< dump one channel formatted QIE
  void DumpOneQIE( uint16_t r);	///< dump one sample formatted QIE
  void DumpHTRQIERaw( int h);	///< dump raw qie
  void DumpQIEStatus( int h);	///< dump ascii string representation of 24 channels
  void DumpAllQIEStatus();	///< dump ascii status for all HTRs
  void DumpHTRQIEZS( int h);	///< dump HTR QIE ZS sums
  void DumpAllTP();		///< dump all HTR TP's
  void DumpHtrTP( int h);	///< dump one HTR's TP's
  void DumpOneTP( uint16_t tp); ///< dump one TP

  //! Dump in hex
  void DumpRaw();

  //! Dump one HTR payload
  void DumpHtr(int h);

  //! Dump all HTRs in parallel columns
  void DumpAllHtr();

  //! access raw HTR payload
  uint32_t* HTR_raw(int h) { return ( m_htr[h]); }

private: 
  uint32_t m_DCCrev;		///< DCC revision 
  uint32_t* m_raw;		///< pointer to raw data 
  uint32_t m_size;		///< data size 
  uint32_t* m_htr[FEDGET_NUM_HTR]; ///< pointers to HTR payloads
  uint16_t* m_chn[FEDGET_NUM_HTR][FEDGET_NUM_CHAN]; ///< pointer to channel data
  int m_chn_len[FEDGET_NUM_HTR][FEDGET_NUM_CHAN]; ///< words per channel
  uint16_t* m_extra[FEDGET_NUM_HTR]; ///< pointer to Extra-Info1 at end of HTR payload
  bool chan_unpacked[FEDGET_NUM_HTR];  ///< Flag:  unpacking done
  uint8_t m_capid[FEDGET_NUM_HTR][FEDGET_NUM_CHAN]; ///< capID: 0=unk 1=bad 2=OK
  bool m_hamming;		///< special hamming LRB firmware in use
  int m_nonZch[FEDGET_NUM_HTR];	///< number of non-zero channels
  bool m_exceptions;		///< throw exceptions on gross unpacker errors
};
  
#endif
