#ifndef _MY_FEDDTC_HH
#define _MY_FEDDTC_HH

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define FEDGET_NUM_UHTR 12
#define HCAL_NUM_FED 32
#define HCAL_MIN_FED 700

#define UHTR_EVN_MASK 0xffffff
#define UHTR_BCN_MASK 0xfff
#define UHTR_ORN_MASK 0x1f

/** \brief Class to access items within HCAL FED Data with minimal unpacking
 *
 * The constructor takes a pointer to the raw data and it's length.
 * An array of pointers to UHTR payloads is set up for quick access.
 * Accessor functions are provided to the various parts of the data payload.
 * An array of pointers to each UHTR's channel data is calculated on demand.
 *
 * Some error-checking functions are provided.
 *
 * NOTE!  Does not currently take into account format variations due to
 * UHTR firmware revisions.
 */

class FedDTC {
public:
  //! Unpack a FED payload.  Set up pointers for fast access.
  /// \param raw raw 32-bit data (first word of CDF header)
  /// \param size size of data in 32-bit words
  /// \param DCC firmware version (16 bits)
  FedDTC( uint32_t* raw, uint32_t size, uint32_t DCCversion);

  //! Pointer to raw data as 32-bit words
  uint32_t* raw() { return m_raw; }
  //! Pointer to raw data as 16-bit words
  uint16_t* raw16() { return m_raw16; }
  //! Size of data in 32-bit words
  uint32_t size() { return m_size; }

  //! Calculate CRC on data (compare with value returned by FedDTC::CRC())
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

  //! Get orbit number from DTC header
  uint32_t OrN() { return (((m_raw[2] >> 4) & 0xfffffff) |
			   ((m_raw[3] << 28) & 0xf0000000)); }

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

  //! UHTR header inserted by DCC for UHTR h = 0..11
  uint16_t UHTR_Hdr( int h) { return m_raw16[12+h]; }
  //! Number of (32 bit) words for UHTR
  // must round up if odd
  uint16_t UHTR_nWords( int h) { return (UHTR_Hdr(h) & 0xfff) + (UHTR_Hdr(h) & 1); }

  //! UHTR status bits (E,P,V,C) field from DCC header
  uint16_t UHTR_epvc( int h) { return (UHTR_Hdr(h) >> 12) & 0xf; }
  //! UHTR LRB error bits from DCC header
  uint8_t UHTR_lrb_err( int h) { return (UHTR_Hdr(h) >> 16) & 0xff; }
  //! UHTR CRC - calculated
  uint16_t UHTR_CRC_Calc(int h);

  // UHTR payload area
  uint16_t* UHTR_Payload() { return &m_raw16[24]; } ///< pointer to start of 1st UHTR payload
  uint16_t* UHTR_Data(int h) { return m_htr[h]; } ///< pointer to particular UHTR payload
  uint16_t* UHTR_End(int h) { return &m_htr[h][UHTR_nWords(h)]; } ///< pointer to UHTR end + 1

  // UHTR items -- ok for even EE
  //! EvN from UHTR
  uint32_t UHTR_EvN(int h) { return (m_htr[h][0] & 0xff) | ((m_htr[h][1] << 8) & 0xffff00); }
  //! BcN from UHTR
  uint16_t UHTR_BcN(int h) { return m_htr[h][4] & 0xfff; }
  //! OrN from UHTR
  uint8_t UHTR_OrN(int h) { return (m_htr[h][3] >> 11) & UHTR_ORN_MASK; }
  //! Submodule ID from UHTR
  uint16_t UHTR_id(int h) { return (m_htr[h][3] & 0x7ff); }
  //! 16-bit UHTR status/error word
  uint16_t UHTR_status(int h) { return m_htr[h][2]; }

  //! Dump for debug
  void Dump( int level);	///< dump 0=nothing 1=DCC 2=UHTR headers
                                // 3=TP 4=formatted 5=qie 6=both 7=QIE raw  8=QIE status
  void Dump( FILE* fp, int level); ///< dump to file

  //! Dump in hex
  void DumpRaw();

  //! Dump one UHTR payload
  void DumpHtr(int h);

  //! Dump all UHTRs in parallel columns
  void DumpAllHtr();

  //! access raw UHTR payload
  uint16_t* UHTR_raw(int h) { return ( m_htr[h]); }

private: 
  uint32_t m_DCCrev;		///< DCC revision 
  uint32_t* m_raw;		///< pointer to raw data (32)
  uint16_t* m_raw16;		///< pointer to raw data (16)
  uint32_t m_size;		///< data size 
  uint16_t* m_htr[FEDGET_NUM_UHTR]; ///< pointers to UHTR payloads
};
  
#endif
