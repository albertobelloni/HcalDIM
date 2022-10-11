
#include "HcalDIM/HcalHexInspector/interface/FedErrors.hh"

#include <exception>
#include <string.h>
#include <stdio.h>

// #define DEBUG

FedErrors::FedErrors() {
#ifdef DEBUG
  printf("FedErrors::FedErrors()\n");
#endif
  Clear();
}

FedErrors::FedErrors( FedGet& fed, int toCheck)
{
#ifdef DEBUG
  printf("FedErrors::FedErrors( %d, %d)\n", fed.fed_id(), toCheck);
#endif
  Clear();
  AddErrors( fed, toCheck);
}

void FedErrors::Clear() {
  memset( this, 0, sizeof(FedErrors)); // clear everything (needed?)
  EvN_min = 0xffffff;
}

bool FedErrors::write( std::ostream* os)
{
  try {
    // header
    *os << FEDERRORS_VERSION << " " << m_fed_id;
    // per-FED items
    *os << " " << m_toCheck << " " << EvN_min << " " << EvN_max << " " << Evt_cnt;
    *os << " " << nTtsRdy << " " << nTtsOfw << " " << nTtsBsy << " " << nTtsSyn;
    *os << " " << nMisOrN << " " << nMisBcN << " " << nMisEvN << " " << nSpcEvN << " " << nLrbErr;
    *os << " " << nLostDCC << " " << nLostLRB << " " << nLostHTR;
    *os << " " << nHtrCRC << " " << nErrFEE << " " << nErrHTR << " " << nErrFRL;
    *os << " " << nFmtConst << " " << nFmtCRC << " " << nFmtSize;
    *os << " " << nErrCapID << " " << nDigiLen;
    *os << std::endl;
    // per-HTR items
    for( int i=0; i<FEDGET_NUM_HTR; i++) {
      *os << " " << htrHadData[i] << " " << htrMisOrN[i] << " " << htrMisBcN[i] << " " << htrMisEvN[i] << " " << htrLrbErr[i];
      *os << " " << htrLrbCErr[i] << " " << htrLrbUErr[i] << " " << htrCRCErr[i];
      *os << " " << htrSpcEvN[i];
    }
    *os << std::endl;
    // 16 HTR status bits
    for( int i=0; i<15; i++)
      *os << " " << nHtrStatus[i];
    *os << std::endl;    
    return true;
  } catch (std::exception& e) {
    return false;
  }
}

bool FedErrors::read( std::istream* is)
{
  int vers;

  try {
    // header
    *is >> vers >> m_fed_id;
    if( vers != FEDERRORS_VERSION) {
      std::cout << "ERROR!  Version mis-match in FedErrors::read()" << std::endl;
      return false;
    }
    // per-FED items
    *is >> m_toCheck >> EvN_min >> EvN_max >> Evt_cnt;
    *is >> nTtsRdy >> nTtsOfw >> nTtsBsy >> nTtsSyn;
    *is >> nMisOrN >> nMisBcN >> nMisEvN >> nSpcEvN >> nLrbErr;
    *is >> nLostDCC >> nLostLRB >> nLostHTR;
    *is >> nHtrCRC >> nErrFEE >> nErrHTR >> nErrFRL;
    *is >> nFmtConst >> nFmtCRC >> nFmtSize;
    *is >> nErrCapID >> nDigiLen;
    // per-HTR items
    for( int i=0; i<FEDGET_NUM_HTR; i++) {
      *is >> htrHadData[i] >> htrMisOrN[i] >> htrMisBcN[i] >> htrMisEvN[i] >> htrLrbErr[i];
      *is >> htrLrbCErr[i] >> htrLrbUErr[i] >> htrCRCErr[i];
      *is >> htrSpcEvN[i];
    }
    // 16 HTR status bits
    for( int i=0; i<15; i++)
      *is >> nHtrStatus[i];
    return true;
  } catch (std::exception& e) {
    return false;
  }
}

void FedErrors::SetBogus() {
  Clear();

  nTtsRdy = 1;
  nTtsOfw = 2;
  nTtsBsy = 3;
  nTtsSyn = 4;

  nMisOrN = 5;
  nMisBcN = 6;
  nMisEvN = 7;
  nSpcEvN = 8;
  nLrbErr = 9;

  for( int h=0; h<FEDGET_NUM_HTR; h++) {
    htrHadData[h] = 1;
    htrMisOrN[h] = 5;
    htrMisBcN[h] = 6;
    htrMisEvN[h] = 7;
    htrLrbErr[h] = 9;
    htrLrbCErr[h] = 10;
    htrLrbUErr[h] = 11;
    htrSpcEvN[h] = 8;
  }

  nLostDCC = 12;
  nLostLRB = 13;
  nLostHTR = 14;
  nHtrCRC = 15;
  nErrFEE = 16;
  nFmtConst = 17;
  nFmtCRC = 18;
  nFmtSize = 19;
  nErrCapID = 20;
  nDigiLen = 21;

}

// add to existing object
void FedErrors::AddErrors( FedGet& fed, int toCheck)
{
  this->fed = &fed;
  m_fed_id = fed.fed_id();

  // take basic information
  int e = fed.EvN();

#ifdef DEBUG
  printf("FedErrors::AddErrors( %d, %d) EvN %d\n", fed.fed_id(), toCheck, e);
#endif

  ++Evt_cnt;
  if( e > EvN_max)
    EvN_max = e;
  if( e < EvN_min)
    EvN_min = e;

  if( toCheck & check_DCC)
    CheckDCCErrors();

  if( toCheck & check_HTR)
    CheckHTRErrors();

  if( toCheck & check_Digi)
    CheckDigiErrors();

  if( toCheck & check_CRC)
    CheckCRC();
  
}


/// Check errors in the overall event structure and DCC information
void FedErrors::CheckDCCErrors() {

  uint32_t* payload = fed->raw();
  uint32_t size32 = fed->size();

  // Record the TTS state
  switch( fed->TTS()) {
  case 1:			// OFW
    nTtsOfw++;
    break;
  case 4:			// BSY
    nTtsBsy++;
    break;
  case 8:			// RDY
    nTtsRdy++;
    break;
  case 2:			// sync lost
    nTtsSyn++;
    nLostDCC++;
    break;
  default:			// Error, disconnected
    nTtsSyn++;
  }
  
  // check constant bits in CDF
  if( ((payload[1] >> 28) & 0xf) != 5 ||
      ((payload[3] >> 28) & 0xf) != 0 ||
      ((payload[size32-1] >> 28) & 0xf) != 0xa) {
    nFmtConst++;
  }
}

/// Check CRC
void FedErrors::CheckCRC() {
  if( fed->CalcCRC() != fed->CRC()) {
    nFmtCRC++;		// report CRC error
    nErrFRL++;		// and FRL error
  }
}

/// Check errors in the HTR headers
void FedErrors::CheckHTRErrors() {

  for( int h=0; h<FEDGET_NUM_HTR; h++) {
    uint16_t nw = fed->HTR_nWords(h);
    if( nw) {			// any data for this HTR?
      ++htrHadData[h];		// yes, record that fact
      if( nw < 4) {		// payload is too small (DCC truncated data)
	nLostLRB++;		// LRB truncated data
      } else {
	if( nw == 4) 	// HTR empty event?
	  nLostHTR++;	// HTR lost data

	// we can perform most checks even on an empty event

	// check HTR CRC
	if( fed->HTR_epbvtc(h) & 1) {
	  ++nHtrCRC;
	  ++htrCRCErr[h];
	}
	// we have at least the HTR header and trailer... proceeed
	// check the EvN, BcN, OrN
	if( fed->EvN() != fed->HTR_EvN(h)) {
#ifdef DEBUG
	  printf("ERROR EvN %d (0x%x) FED %d htr %d EvN=%d (0x%x)\n",
		 fed->EvN(), fed->EvN(), fed->fed_id(), h, fed->HTR_EvN(h), fed->HTR_EvN(h));
	  //	  fed->Dump(4);
#endif	    
	  nMisEvN++;
	  ++htrMisEvN[h];
	  if( fed->HTR_EvN(h) != 0xa00039) {
	    ++htrSpcEvN[h];
	    ++nSpcEvN;
	  }
	}
	if( fed->BcN() != fed->HTR_BcN(h)) {
	  nMisBcN++;
	  ++htrMisBcN[h];
	}
	if( (fed->OrN() & 0x1f) != fed->HTR_OrN(h)) {
	  nMisOrN++;
	  ++htrMisOrN[h];
	}
	if( fed->HTR_lrb_err(h)) { // any non-zero bits
	  ++htrLrbErr[h];
	  ++nLrbErr;
	  if( fed->HTR_lrb_err(h) & 1)
	    ++htrLrbCErr[h];
	  if( fed->HTR_lrb_err(h) & 2)
	    ++htrLrbUErr[h];
	}

	// check all HTR status bits
	for( int k=0, b=1; k<16; k++, b<<=1)
	  if( fed->HTR_status(h) & b)
	    ++nHtrStatus[k];

	// check for lost data in LRB
	if( fed->HTR_lrb_err(h) & 4)
	  nLostLRB++;
	  
	// check for link errors in data from this HTR
	if( fed->HTR_lrb_err(h) & 3) // check bits UE and CE in LRB trailer
	  nErrHTR++;

      }
    }
  }
}

/// Check for errors in the digitized data (capID, link errors)
void FedErrors::CheckDigiErrors() {

  //  uint32_t size = 0;

  for( int h=0; h<FEDGET_NUM_HTR; h++) {
    uint16_t nw = fed->HTR_nWords(h);
    if( nw > 10) {		// any payload for this HTR?
      if( fed->HTR_ndd(h)) {	// any DAQ data?
	for( int ch=0; ch<FEDGET_NUM_CHAN; ch++) {
	  int qs = fed->HTR_QIE_status( h, ch);

	  for( int b=0; b<QIE_STATUS_MAX; b++)
	    if( qs & (1<<b))
	      ++htrDigiState[h][ch][b];

  	  if( qs & QIE_STATUS_LEN)
  	    ++nDigiLen;

	  if( qs & QIE_STATUS_ERR)
	    ++nErrFEE;

	  if( !(qs & QIE_STATUS_CAPID)) {
	    ++nErrCapID;
	  }

	} // for( ch)
      } // (if ndd)
    } // if( nw > 10)
  } // (for htr...)
}


void FedErrors::Dump()
{
  printf("FED %d Errors for %d events in EvN range %d (0x%x) - %d (0x%x)\n",
	 fed->fed_id(), Evt_cnt, EvN_min, EvN_min, EvN_max, EvN_max);
  printf("   nTtsRdy %5d  nTtsOfw %5d  nTtsBsy %5d nTtsSyn %5d\n", nTtsRdy, nTtsOfw, nTtsBsy, nTtsSyn);
  printf("   nMisOrn %5d  nMisBcn %5d  nMisEvn %5d nSpcEvN %5d nLrbErr %5d\n",
	 nMisOrN, nMisBcN, nMisEvN, nSpcEvN, nLrbErr);
  printf("  nLostDCC %5d nLostLRB %5d nLostHTR %5d NErrFee %5d nErrHTR %5d nErrFRL %5d\n",
	 nLostDCC, nLostLRB, nLostHTR, nErrFEE, nErrHTR, nErrFRL);
  printf("  nFmtConst %5d nFmtCRC %5d nFmtSize %5d nErrCapID %5d\n",
	 nFmtConst, nFmtCRC, nFmtSize, nErrCapID);
  
  for( int i=0; i<FEDGET_NUM_HTR; i++) {
    printf("Spigot %2d nBlk %d", i, htrHadData[i]);
    if( htrHadData[i]) {
      printf("  htrMisOrN %5d htrMisBcN %5d htrMisEvN %5d htrLrbErr %5d htrSpcEvN %5d\n",
	     htrMisOrN[i], htrMisBcN[i], htrMisEvN[i], htrLrbErr[i], htrSpcEvN[i]);
      for( int ch=0; ch<FEDGET_NUM_CHAN; ch++) {
	int te = 0;
	for( int b=0; b<QIE_STATUS_MAX; b++)
	  te += htrDigiState[i][ch][b];
	if( te) {
	  printf(" Chan %2d ", ch);
	  for( int b=0; b<QIE_STATUS_MAX; b++)
	    if( htrDigiState[i][ch][b])
	      printf("%s: %d", QIE_status_name( b), htrDigiState[i][ch][b]);
	  printf("\n");
	}
      }
    } else {
      printf(" (no data)\n");
    }
  }
}
