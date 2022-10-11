#ifndef _FEDERRORS_HH
#define _FEDERRORS_HH

#include <iostream>
#include "HcalDIM/HcalHexInspector/interface/FedGet.hh"

// data version for write/read
#define FEDERRORS_VERSION 1

/** \brief Error checking class for FED data
 *
 * This class checks FED data for errors.  A set of error counters is
 * maintained as a set of public members, which may be accessed for
 * histogram filling.
 *
 * Various levels of checking are suppored, requiring differing
 * amounts of CPU time.
 */
class FedErrors {

public:

  /// Error-check options
  enum {check_None = 0, check_DCC = 1, check_HTR = 2, check_CRC = 4,
	check_Digi = 8, check_ALL = 0xffff };

  FedErrors();

  /** \brief Check FED data for errors
   * \param fed Unpacked FED object to check
   * \param toCheck what to check (any from list below with |)
   * Default is (check_DCC|check_HTR)
   * - <b>check_None</b> = don't check anything (initialize empty structure)
   * - <b>check_DCC</b> = check DCC/CDF errors
   * - <b>check_HTR</b> = check HTR header errors
   * - <b>check_CRC</b> = calculate and check CRC
   * - <b>check_Digi</b> = check all errors including capIDs and FEE links
   */
  FedErrors( FedGet& fed, int toCheck = (check_DCC|check_HTR));

  //! Serialize (in text) contents to stream
  /// \param os stream
  /// Returns true if OK, false on I/O error
  bool write( std::ostream* os);

  //! Initialize existing object from stream
  /// \param is stream
  /// Returns true if OK, false on I/O error
  bool read( std::istream* is);

  //! Add errors found in stream to existing object
  /// \param is stream
  /// \param toCheck flags for what to check (see constructor doco)
  /// Returns true if OK, false on I/O error
  bool AddErrors( std::istream* is, int toCheck);

  /// \brief Add errors to an existing object
  void AddErrors( FedGet& fed, int toCheck);
  /// \brief Clear an existing object
  void Clear();
  /// \brief Check errors in the overall event structure and DCC information
  void CheckDCCErrors();
  /// \brief Check errors in the HTR headers
  void CheckHTRErrors();
  /// \brief Check for errors in the digitized data (capID, link errors)
  void CheckDigiErrors();
  /// \brief Check CRC of payload
  void CheckCRC();
  /// \brief Set all member variables to fixed debug values
  void SetBogus();

  /// \brief Dump to standard output
  void Dump();

  FedGet* fed;   ///< pointer to the unpacked FED data

  int m_fed_id;			///< ID for this FED

  int m_toCheck;		///< check flag

  // Event range
  int EvN_min;
  int EvN_max;
  int Evt_cnt;

  // Error counters
  int nTtsRdy;	       ///< TTS reports "RDY"
  int nTtsOfw;	       ///< TTS reports "OFW"
  int nTtsBsy;	       ///< TTS reports "BSY"
  int nTtsSyn;               ///< TTS reports any other condition (SYN, ERR, disconnected)

  int nMisOrN;	       ///< some HTR OrN doesn't match DCC OrN
  int nMisBcN;	       ///< some HTR BcN doesn't match DCC BcN
  int nMisEvN;	       ///< some HTR EvN doesn't match DCC EvN
  int nSpcEvN;         ///< some HTR EvN doesn't match DCC EvN and .ne. 0xa00039
  int nLrbErr;         ///< some HTR spigot had LRB errors

  // counters by HTR
  int htrHadData[FEDGET_NUM_HTR];  ///< this HTR had a non-empty payload
  int htrMisOrN[FEDGET_NUM_HTR];    ///< this HTR OrN doesn't match DCC OrN
  int htrMisBcN[FEDGET_NUM_HTR];  ///< this HTR OrN doesn't match DCC BcN
  int htrMisEvN[FEDGET_NUM_HTR];  ///< this HTR OrN doesn't match DCC EvN
  int htrLrbErr[FEDGET_NUM_HTR];  ///< this HTR had LRB errors (any bit NZ)

  int htrLrbCErr[FEDGET_NUM_HTR]; ///< this HTR had corrected LRB errors
  int htrLrbUErr[FEDGET_NUM_HTR]; ///< this HTR and uncorrected LRB errors
  int htrCRCErr[FEDGET_NUM_HTR];  ///< this HTR had CRC errors

  int htrSpcEvN[FEDGET_NUM_HTR];  ///< special debug counter (EvN .ne. 0xa000f9)

  int nLostDCC;	       ///< lost data in DCC (TTC in SYN condition)
  int nLostLRB;              ///< lost data in LRB (E_TRUNC set in LRB error word)
  int nLostHTR;			///< HTR payload size < 10

  int nHtrCRC;   ///< HTR CRC flagged bad by DCC

  int nErrFEE;               ///< FEE link errors - not valid or lin error
  int nErrHTR;               ///< LRB links report error (uncorrected or corrected)
  int nErrFRL;	       ///< FRL data not valid (not defined)

  int nFmtConst;             ///< Format error:  constant bits not constant (various)
  int nFmtCRC;	       ///< CRC error in FRL (optional, CPU time)
  int nFmtSize;	       ///< Size error in payload (HTR sizes don't add up, etc)

  int nErrCapID;		///< CapID don't rotate if link data is valid

  int nHtrStatus[16];		///< Counters for each HTR status bit 

  // per-channel errors
  int nDigiLen;
  int htrDigiState[FEDGET_NUM_HTR][FEDGET_NUM_CHAN][QIE_STATUS_MAX];  ///< QIE input data state

};


#endif
