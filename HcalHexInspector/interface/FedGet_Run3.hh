#ifndef _MY_FEDGET_RUN3_HH
#define _MY_FEDGET_RUN3_HH

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>

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

class FedGet_Run3 {
public:
  //! Unpack a FED payload.  Set up pointers for fast access.
  /// \param raw raw 32-bit data (first word of CDF header)
  /// \param size size of data in 32-bit words
  /// \param DCC firmware version (16 bits)
  FedGet_Run3( uint32_t* raw, uint32_t size, uint32_t DCCversion);

  // Some constants within this module

  // Ignore FE data errors <= this BCN
  static const int FEDGET_ORBIT_MSG_BCN=2;
  static const int FEDGET_NUM_UHTR=15;
  static const int FEDGET_NUM_CHAN=24;
  static const int HCAL_NUM_FED=32;
  static const int HCAL_MIN_FED=700;
  static const int HCAL_MAX_TS=20;

  // number of bit values used
  static const int QIE_STATUS_MAX=5;
  // individual error codes
  static const int QIE_STATUS_ERR=1;
  static const int QIE_STATUS_CAPID=2;
  static const int QIE_STATUS_EMPTY=4;
  static const int QIE_STATUS_LEN=8;
  static const int QIE_STATUS_OK=0x10;

  //! Pointer to raw data
  uint32_t* raw() { return m_raw; }
  //! Size of data in 32-bit words
  uint32_t size() { return m_size; }

  //! Set hamming mode
  void SetHamming( bool h) { m_hamming=h; }

  //! Calculate CRC on data 
  //  (compare with value returned by FedGet_Run3::CRC())
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

  //! uHTR header inserted by DCC for uHTR h = 0..14
  uint32_t uHTR_Hdr( int h) { return m_raw[6+h]; }
  //! Number of (32 bit) words for uHTR
  uint16_t uHTR_nWords( int h) { return uHTR_Hdr(h) & 0x3ff; }
  //! uHTR empty event
  bool uHTR_EE(int h) { return (uHTR_nWords(h) <= 4); }

  //! uHTR status bits (E,P,B,V,T,C) field from DCC header
  uint16_t uHTR_epbvtc( int h) { return (uHTR_Hdr(h) >> 10) & 0x3f; }
  //! uHTR status/error bits (low 8 bits only) from DCC header
  uint8_t uHTR_err( int h) { return (uHTR_Hdr(h) >> 24) & 0xff; }
  //! uHTR LRB error bits from DCC header
  uint8_t uHTR_lrb_err( int h) { return (uHTR_Hdr(h) >> 16) & 0xff; }
  //! uHTR CRC from trailer
  uint16_t uHTR_CRC(int h) { return uHTR_EE(h) ? 0 : ( (uHTR_End(h)[-2] >> 16) & 0xffff); }
  //! uHTR CRC - calculated
  uint16_t uHTR_CRC_Calc(int h);

  // uHTR payload area
  uint32_t* uHTR_Payload() { return &m_raw[24]; } ///< pointer to start of 1st uHTR payload
  uint32_t* uHTR_Data(int h) { return m_uhtr[h]; } ///< pointer to particular uHTR payload
  uint32_t* uHTR_End(int h) { return &m_uhtr[h][uHTR_nWords(h)]; } ///< pointer to uHTR end + 1

  // uHTR items -- ok for even EE
  //! EvN from uHTR
  uint32_t uHTR_EvN(int h) { return (m_uhtr[h][0] & 0xff) | ((m_uhtr[h][0] >> 8) & 0xffff00); }
  //! BcN from uHTR
  uint16_t uHTR_BcN(int h) { return m_uhtr[h][2] & 0xfff; }
  //! OrN from uHTR
  uint8_t uHTR_OrN(int h) { return (m_uhtr[h][1] >> 27) & 0x1f; }
  //! Submodule ID from uHTR
  uint16_t uHTR_id(int h) { return (m_uhtr[h][1] >> 16) & 0x3ff; }
  //! 16-bit uHTR status/error word
  uint16_t uHTR_status(int h) { return m_uhtr[h][1] & 0xffff; }

  // These are all zero/null for EE
  //! Number of trigger primitives
  uint16_t uHTR_ntp(int h) { return uHTR_EE(h) ? 0 : ((m_uhtr[h][2] >> 24) & 0xff); }
  //! Number of DAQ data
  uint16_t uHTR_ndd(int h) { return uHTR_EE(h) ? 0 : (uHTR_End(h)[-2] & 0x3ff); }
  //! Number of time samples
  uint8_t uHTR_ns(int h) { return uHTR_EE(h) ? 0 : ((uHTR_End(h)[-2] >> 11) & 0x1f); }
  //! Unsuppressed data flag
  bool uHTR_us(int h) { return uHTR_EE(h) ? false : ( ((m_uhtr[h][3] >> 15) & 1) != 0); }
  // QIE data
  //! Pointer to start of TP for uHTR
  uint16_t* uHTR_TP(int h) { return uHTR_EE(h) ? NULL : (uint16_t*)(&m_uhtr[h][4]); }
  //! Pointer to start of QIE data for uHTR
  uint16_t* uHTR_QIE(int h) { return uHTR_EE(h) ? NULL : uHTR_TP(h)+uHTR_ntp(h); }
  //! Pointer to QIE data for a particular channel
  uint16_t* uHTR_QIE_chan(int h, int c);
  //! Number of QIE data for a particular channel
  int uHTR_QIE_chan_len(int h, int c) { return uHTR_EE(h) ? 0 : m_chn_len[h][c]; }
  //! Raw ADC data (0..127) samples in int array.  Return no. samples
  int uHTR_QIE_Adc( int h, int c, int *Adc);
  //! Convert QIE data for a channel to fC in user-supplied buffer.  Return no. samples.
  int uHTR_QIE_fC( int h, int c, double *fC);
  //! Calculate sum of all QIE samples (0 if no data)
  double uHTR_QIE_sum_fC( int h, int c);
  //! Calculate mean of all QIE samples (0 if no data)
  double uHTR_QIE_mean_fC( int h, int c);
  //! Calculate ZS critirium for a channel ("Sliding pair sum max"). --> Returns -1 on error
  int uHTR_QIE_zs_sum(int h, int c);
  ///! Number of non-zero data channels
  int uHTR_nonZch( int h);

  ///! Tag for a TP
  int TP_Tag( uint16_t tp) { return (tp >> 11) & 0x1f; }
  ///! Z bit for a TP
  int TP_Z( uint16_t tp) { return (tp>>10)&1; }
  //! SOI bit for a TP
  int TP_SOI( uint16_t tp) { return (tp>>9)&1; }
  //! TP data for a TP
  int TP_data( uint16_t tp) { return (tp & 0x1f); }

  //! Check if capID rotated for a uHTR, channel
  bool good_capID(int h, int c);

  //! check status of a channel and return a bit mask
  // 
  int uHTR_QIE_status(int h, int c);

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
  const static char* qie_status_name[];

  // short title (6 chars max) for uHTR word
  static char hwname[16];
  char* uHTRWordName(int);

  //! Dump for debug
  void Dump( int level);	///< dump 0=nothing 1=DCC 2=uHTR headers
                                // 3=TP 4=formatted 5=qie 6=both 7=QIE raw  8=QIE status
  void Dump( FILE* fp, int level); ///< dump to file
  void DumpAllQIE();		///< dump formatted QIE for all uHTR
  void DumpUHTRQIE( int h);	///< dump formatted QIE for one uHTR
  void DumpQIE( int h, int c);	///< dump one channel formatted QIE
  void DumpOneQIE( uint16_t r);	///< dump one sample formatted QIE
  void DumpUHTRQIERaw( int h);	///< dump raw qie
  void DumpQIEStatus( int h);	///< dump ascii string representation of 24 channels
  void DumpAllQIEStatus();	///< dump ascii status for all uHTRs
  void DumpUHTRQIEZS( int h);	///< dump uHTR QIE ZS sums
  void DumpAllTP();		///< dump all uHTR TP's
  void DumpUHTRTP( int h);	///< dump one uHTR's TP's
  void DumpOneTP( uint16_t tp); ///< dump one TP

  //! Dump in hex
  void DumpRaw();

  //! Dump one uHTR payload
  void DumpUHTR(int h);

  //! Dump all uHTRs in parallel columns
  void DumpAllUHTR();

  //! access raw uHTR payload
  uint32_t* uHTR_raw(int h) { return ( m_uhtr[h]); }

private: 
  uint32_t m_DCCrev;		///< DCC revision 
  uint32_t* m_raw;		///< pointer to raw data 
  uint32_t m_size;		///< data size 
  uint32_t* m_uhtr[FEDGET_NUM_UHTR]; ///< pointers to uHTR payloads
  uint16_t* m_chn[FEDGET_NUM_UHTR][FEDGET_NUM_CHAN]; ///< pointer to channel data
  int m_chn_len[FEDGET_NUM_UHTR][FEDGET_NUM_CHAN]; ///< words per channel
  uint16_t* m_extra[FEDGET_NUM_UHTR]; ///< pointer to Extra-Info1 at end of uHTR payload
  bool chan_unpacked[FEDGET_NUM_UHTR];  ///< Flag:  unpacking done
  uint8_t m_capid[FEDGET_NUM_UHTR][FEDGET_NUM_CHAN]; ///< capID: 0=unk 1=bad 2=OK
  bool m_hamming;		///< special hamming LRB firmware in use
  int m_nonZch[FEDGET_NUM_UHTR];	///< number of non-zero channels
  bool m_exceptions;		///< throw exceptions on gross unpacker errors
};
  
#endif
