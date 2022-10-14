
// #define CRC16

#include "HcalDIM/HcalHexInspector/interface/FedGet_Run3.hh"
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


// #define DEBUG

FedGet_Run3::FedGet_Run3( uint32_t* raw, uint32_t size, uint32_t DCCver) {

  memset( this, 0, sizeof(FedGet_Run3)); // clear everything (needed?)




  m_exceptions = false;

  m_DCCrev = DCCver;
  m_raw = raw;
  m_size = size;
  if( Evt_Length() * 2 != m_size) {
    fprintf( stderr, "FedGet_Run3():  Bad event length!  size=%d  CDF trailer=%d\n",
	    size, Evt_Length()*2 );
    if( m_exceptions)
      throw 1;
    else {
      fprintf( stderr, "FedGet_Run3():  WARNING!  Proceeding to unpack corrupted event\n");
      printf( "FedGet_Run3():  WARNING!  Proceeding to unpack corrupted event\n");
    }
  }
  // build table of pointers to uHTR payloads
  uint32_t* p = uHTR_Payload();	// start of uHTR payloads
  for( int i=0; i<FEDGET_NUM_UHTR; i++) {
    if( uHTR_nWords(i)) {
      m_uhtr[i] = p;
      p += uHTR_nWords(i);
      m_extra[i] = (uint16_t* )p - 12;	// pointer to 'Extra-Info1' at end of payload
    } else
      m_uhtr[i] = NULL;
  }
  memset( chan_unpacked, 0, sizeof(chan_unpacked));
  memset( m_chn, 0, sizeof(m_chn));
  memset( m_capid, 0, sizeof(m_capid));
  memset( m_chn_len, 0, sizeof(m_chn_len));
  m_hamming = false;
}

int FedGet_Run3::uHTR_QIE_Adc( int h, int c, int *Adc) {
  if( !chan_unpacked[h])
    /*uint16_t* stuff =*/ uHTR_QIE_chan(h, 0); // force unpacking

  int ns = uHTR_QIE_chan_len( h, c);
  if( ns == 0)
    return 0;
  if( uHTR_QIE_status(h, c) != QIE_STATUS_OK)
    return 0;
  uint16_t* qr = uHTR_QIE_chan( h, c);
  for( int i=0; i<ns; i++) {
    uint16_t q = qr[i];
    Adc[i] = (QIE_range(q) << 5) | (QIE_mant(q));
  }
  return ns;
}


uint16_t FedGet_Run3::uHTR_CRC_Calc(int h) {

#ifdef CRC16
  uint8_t* p = (uint8_t *)uHTR_Data(h); // byte pointer
  int n = uHTR_nWords(h) * 4;	// byte count
  uint16_t crc = crc_update( 0xffff, &p[2], n-8);
#else

  uint16_t* p = (uint16_t *)uHTR_Data(h);	// byte pointer
  int n = uHTR_nWords(h) * 2;	// word count
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

int FedGet_Run3::uHTR_QIE_zs_sum(int h, int c) {
  int data[HCAL_MAX_TS];
  int ns = uHTR_QIE_Adc( h, c, data);
  if( ns == 0)
    return -1;
  int maxsum = 0;
  // calculate maximum sum of two successive samples
  // per uHTR ZS algorithm.  
  for(int i=0; i<ns; ++i) {
    if (i==0) maxsum = data[i];
    else if ((data[i] + data[i-1] ) > maxsum)
      maxsum = data[i] + data[i-1];
  }
  return maxsum;
}



int FedGet_Run3::uHTR_QIE_fC( int h, int c, double *fC) {
  if( !chan_unpacked[h])
    /*uint16_t* stuff =*/ uHTR_QIE_chan(h, 0); // force unpacking
  int ns = uHTR_QIE_chan_len( h, c);
  if( ns == 0)
    return 0;
  if( uHTR_QIE_status(h, c) != QIE_STATUS_OK)
    return 0;
  uint16_t* qr = uHTR_QIE_chan( h, c);
  for( int i=0; i<ns; i++) {
    uint16_t q = qr[i];
    fC[i] = Adc2fc[ (QIE_range(q) << 5) | (QIE_mant(q))];
  }
  return ns;
}

double FedGet_Run3::uHTR_QIE_sum_fC( int h, int c) {
  double fc[HCAL_MAX_TS];
  double s = 0.0;

  int ns = uHTR_QIE_fC( h, c, fc);
  if( ns == 0 || ns > HCAL_MAX_TS)
    return 0.0;
  for( int i=0; i<ns; i++)
    s += fc[i];
  return s;
}

double FedGet_Run3::uHTR_QIE_mean_fC( int h, int c) {
  int ns = uHTR_QIE_chan_len( h, c);
  if( ns == 0)
    return 0.0;
  double m = uHTR_QIE_sum_fC( h, c);
  return m / (double)ns;
}

uint16_t* FedGet_Run3::uHTR_QIE_chan(int h, int c) {
#ifdef DEBUG
  printf("FedGet_Run3::uHTR_QIE_chan( %d, %d)...\n", h, c);
#endif
  if( !chan_unpacked[h]) {
#ifdef DEBUG
    printf("  unpacking with ndd=%d...\n", uHTR_ndd(h) );
#endif
    // build table of pointers to channels, respecting zero-suppression
    // also record number of words in each channel
    if( uHTR_ndd(h)) {		// any DAQ data?
      int ch0 = -1;		// previous channel
      int ch = 0;		// this channel
      uint16_t* q = uHTR_QIE(h);	// point to start of QIE data
      uint16_t* q0 = q;		// point to previous data
      for( int i=0; i<uHTR_ndd(h); i++) {
	ch = QIE_fiberID(*q)*3 + QIE_chipID(*q); // current channel
#ifdef DEBUG
	printf("%04x index chan i=%d fib=%d chip=%d ch=%d ch0=%d\n", *q, i,
	       QIE_fiberID(*q), QIE_chipID(*q),
	       ch, ch0);
#endif
	if( ch != ch0) {	// did the channel change?
	  m_chn[h][ch] = q;	// yes, it's start of a new channel
	  if( i != 0)		// calculate length if not first channel
	    m_chn_len[h][ch0] = q - q0;
#ifdef DEBUG
	  printf("  new ch at %d len=%d\n", q-uHTR_QIE(h), q-q0);
#endif
	  ch0 = ch;
	  q0 = q;
	}
	++q;
      }
      // now the length for the last channel, watch for 'FFFF' word!
      m_chn_len[h][ch] = m_extra[h] - q0;
#ifdef DEBUG
      printf("Setting last channel length [%d][%d] to %d\n",
	     h, ch, m_chn_len[h][ch]);
#endif      
      if( *(m_extra[h]-1) == 0xffff) {
#ifdef DEBUG
	printf("Adjusting -1 for ffff\n");
#endif	
	--m_chn_len[h][ch];
      }
    }
  }

  // count non-zero channels per uHTR
  m_nonZch[h] = 0;
  for( int ch=0; ch<FEDGET_NUM_CHAN; ch++)
    if( m_chn_len[h][ch])
      ++m_nonZch[h];

  chan_unpacked[h] = true;
#ifdef DEBUG
  printf("FedGet_Run3::uHTR_QIE_chan(%d,%d) returns 0x%x\n", h, c, m_chn[h][c]);
#endif
  return m_chn[h][c];
}

int FedGet_Run3::uHTR_nonZch( int h) {
  if( !chan_unpacked[h])
    /*uint16_t* stuff =*/ uHTR_QIE_chan(h, 0); // force unpacking
  return m_nonZch[h];
}

void FedGet_Run3::DumpRaw() {
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

void FedGet_Run3::DumpUHTR(int h) {
  if( uHTR_nWords(h)) {
    uint16_t* p = (uint16_t*)m_uhtr[h];
    int size16 = uHTR_nWords(h)*2;
    for( int i=0; i<size16; i++) {
      printf( "%04d: %04x\n", i, p[i]);
    }
  }
}


void FedGet_Run3::DumpOneQIE( uint16_t r) {
  printf(" %04x fib %d chip %d EV=%c%c rng %d mant %2d cap %d\n",
	 r,
	 QIE_fiberID(r), QIE_chipID(r),
	 QIE_dv(r) ? 'V' : 'n',
	 QIE_err(r) ? 'E' : ' ',
	 QIE_range(r), QIE_mant(r),
	 QIE_capID(r) );
}

// report QIE status
// use some hopefully sensible logic:
//   wrong nTS is independent of the other checks
//   if we see nDV/Err, don't check capID
//   no data is mutually exclusive with others
int FedGet_Run3::uHTR_QIE_status( int h, int c)
{
  // scan for DV/Err
  uint16_t* q = uHTR_QIE_chan( h, c);
  int rc = 0;

  if( q == NULL)		// no data at all
    return QIE_STATUS_EMPTY;

  int n = uHTR_QIE_chan_len(h,c);

  if( n == 0)			// length zero?
    return QIE_STATUS_EMPTY;

  if( n != uHTR_ns(h))		// check nTS
      rc |= QIE_STATUS_LEN;

  // done checking for first few bunches (orbit message?)
  if( BcN() <= FEDGET_ORBIT_MSG_BCN)
    return rc;

  for( int i=0; i<n; i++) {
    uint16_t r = q[i];
    if( !QIE_dv(r) || QIE_err(r)) // nDV/Err ends checking
      return rc | QIE_STATUS_ERR;
  }
  // all DV ok, check capID
  if( good_capID( h, c))
    return rc | QIE_STATUS_OK;
  else
    return rc | QIE_STATUS_CAPID;
}


void FedGet_Run3::DumpQIE( int h, int c) {
  uint16_t* q = uHTR_QIE_chan( h, c);
  if( q != NULL) {
    int n = uHTR_QIE_chan_len(h,c);
    if( n) {
      for( int i=0; i<n; i++) {
	uint16_t r = q[i];
	DumpOneQIE( r);
      }
    }
  }
}

void FedGet_Run3::DumpUHTRQIEZS( int h) {
  printf("FED %3d uHTR %d ZS criteria:\n", fed_id(), h);
  if( uHTR_nWords(h) && uHTR_ndd(h)) {
    for( int ch=0; ch<FEDGET_NUM_CHAN; ch++) {
      int zs = uHTR_QIE_zs_sum(h,ch);
      if( zs >= 0)
	printf("  ch %2d zs_sum %d\n", ch, zs);
    }
  }
}

void FedGet_Run3::DumpUHTRQIERaw( int h) {
  printf("uHTR %d raw DAQ data:\n", h);
  if( uHTR_nWords(h) && uHTR_ndd(h)) {
    DumpUHTRQIEZS( h);
    uint16_t* q = uHTR_QIE(h);
    for( int i=0; i<uHTR_ndd(h); i++)
      DumpOneQIE( q[i]);
  }
}

void FedGet_Run3::DumpUHTRQIE( int h) {
  printf("uHTR %2d:\n", h);
  for( int c=0; c<FEDGET_NUM_CHAN; c++) {
    int zs = uHTR_QIE_zs_sum(h,c);    
    int cl = uHTR_QIE_chan_len(h,c);
    // don't dump ZS'd samples
    if( cl) {
      printf("Ch %2d len=%d zs_sum=%d%s:\n", c, cl,
	     zs >= 0 ? zs : 0,
	     (cl != uHTR_ns(h)) ? " Wrong NTS!" : "");
      DumpQIE( h, c);
    }
  }
}

void FedGet_Run3::DumpQIEStatus( int h) {
  printf("uHTR %2d: [", h);
  for( int c=0; c<FEDGET_NUM_CHAN; c++) {
    switch( uHTR_QIE_status( h, c)) {
    case QIE_STATUS_OK:		// AOK
      putchar(' ');
      break;
    case QIE_STATUS_ERR:	// nDV/Err
      putchar('-');
      break;
    case QIE_STATUS_CAPID:
      putchar('*');		// capID
      break;
    case QIE_STATUS_EMPTY:
      putchar('.');		// missing
      break;
    case QIE_STATUS_LEN:
      putchar('L');		// wrong length
      break;
    default:
      putchar('?');		// unknown code
      break;
    }
  }
  printf("]\n");
}

void FedGet_Run3::DumpAllQIEStatus() {
  for( int h=0; h<FEDGET_NUM_UHTR; h++) {
    if (uHTR_nWords( h))
      DumpQIEStatus( h);
  }
}


void FedGet_Run3::DumpAllQIE() {
  printf("EvN %d (0x%d) FED %3d:\n", EvN(), EvN(), fed_id());
  for( int h=0; h<FEDGET_NUM_UHTR; h++) {
    if (uHTR_nWords( h)) {
      DumpUHTRQIE( h);
    }
  }
}

void FedGet_Run3::DumpAllTP() {
  printf("TP:\n");
  for( int h=0; h<FEDGET_NUM_UHTR; h++) {
    if( uHTR_ntp(h)) {
      DumpUHTRTP(h);
    }
  }
  printf("\n");
}

void FedGet_Run3::DumpUHTRTP( int h) {
    uint16_t *ptp = uHTR_TP( h);
    int n = uHTR_ntp(h);
    int last_tag = -1;
    printf("uHTR %2d TP: %2d ", h, n);
    for( int k=0; k<n; k++) {
      if( TP_Tag(ptp[k]) != last_tag && last_tag > 0)
	printf("\n              ");
      DumpOneTP( ptp[k]);
      last_tag = TP_Tag(ptp[k]);
    }
    printf("\n");
}

void FedGet_Run3::DumpOneTP( uint16_t tp) {
  printf(" [%2x %c%c %3x]", TP_Tag(tp), TP_Z(tp) ? 'Z':' ', TP_SOI(tp) ? 'S':' ', TP_data(tp));
}


void FedGet_Run3::DumpAllUHTR() {
  // count active spigots, find largest payload
  int nas = 0;
  int maxs = 0;
  int hno[FEDGET_NUM_UHTR];
  for( int h=0; h<FEDGET_NUM_UHTR; h++) {
    int n;
    if( (n = 2 * uHTR_nWords(h))) {
      hno[nas++] = h;
      if( n > maxs)
	maxs = n;
    }
  }
  // now display them
  if( nas) {
    printf("      uHTR#: ");
    for( int i=0; i<nas; i++) {
      int h = hno[i];
      if( m_hamming)
	printf( " %9d", h);
      else
	printf( " %4d", h);
    }
    printf("\n");

    if( m_hamming)
      maxs /= 2;

    for( int n=0; n<maxs; n++) {
      printf("%6s %04d: ", uHTRWordName(n), n);
      for( int i=0; i<nas; i++) {
	int h = hno[i];
	if( n < uHTR_nWords(h)*2) {
	  if( m_hamming)
	    printf(" %04x-%04x", (m_uhtr[h][n]>>16)&0xffff, (m_uhtr[h][n]&0xffff));
	  else
	    printf(" %04x", ((uint16_t*) m_uhtr[h])[n]);
	} else {
	  if( m_hamming)
	    printf("          ");
	  else
	    printf("     ");
	}
      }
      printf("\n");
    }
    printf("\n");    
  }
      
}

bool FedGet_Run3::good_capID(int h, int c) {
#ifdef DEBUG
  printf("FedGet_Run3::good_capID( %d, %d) EvN %d (0x%x) FED %d...\n", h, c,
	 EvN(), EvN(), fed_id());
#endif  
  if( uHTR_QIE_chan(h,c) == NULL) {
#ifdef DEBUG
    printf("  no data... return false\n");
#endif
    return false;
  }
  if( m_capid[h][c]) {		// capID state known?
#ifdef DEBUG
    printf("  known as %d, return %d\n", m_capid[h][c], m_capid[h][c]-1);
#endif    
    return( (bool)(m_capid[h][c]-1) );	// yes, return 0(bad) or 1(good)
  }
  uint16_t* q = uHTR_QIE_chan( h, c);
  // else check it
#ifdef DEBUG
  printf("cap_ID htr %d chan %d check length = %d\n", h, c, uHTR_QIE_chan_len(h,c));
#endif
  if( uHTR_QIE_chan_len(h,c) > 1) {
    for( int i=1; i<uHTR_QIE_chan_len(h,c); i++) {

#ifdef DEBUG
	printf( "cap_ID sample %d  %d -> %d\n",
		i, ((QIE_capID(q[i-1])+1)%4), QIE_capID(q[i]) );
#endif
      if( ((QIE_capID(q[i-1])+1)%4) != QIE_capID(q[i])) {
#ifdef DEBUG
	printf("  return bad ID\n");
#endif
	m_capid[h][c] = 1;
	return false;
      }
    }
  }
  m_capid[h][c] = 2;
#ifdef DEBUG
	printf("  return good ID\n");
#endif
  return true;
}


// calculate CRC
uint16_t FedGet_Run3::CalcCRC() {
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
void FedGet_Run3::Dump( int level) {
  Dump( stdout, level);
}

void FedGet_Run3::Dump( FILE *fp, int level) {
  fprintf( fp, "FED: %3d EvN: %06x  BcN: %03x  OrN: %08x  TTS: %x/%x EvTyp: %x  CalTyp: %x Size: %d\n",
	 fed_id(), EvN(), BcN(), OrN(), TTS(), fTTS(), EvtTyp(), CalTyp(), m_size );
  if( CalcCRC() != CRC())
    fprintf( fp, "CRC Error!  cal=%08x  data=%08x\n", CalcCRC(), CRC() );
  if( level > 1) {

    for( int i=0; i<15; i++) {
      if( uHTR_nWords(i)) {
	fprintf( fp, " %2d: id: %03x Size: %03d EvN: %06x BcN: %03x  OrN: %02x  HDR: %04x LRB: %02x ntp: %d  ndd: %3d ns: %d %s %s\n",
		 i, uHTR_id(i), uHTR_nWords(i), uHTR_EvN(i), uHTR_BcN(i), uHTR_OrN(i), uHTR_status(i),
		 uHTR_lrb_err(i),
		 uHTR_ntp(i), uHTR_ndd(i), uHTR_ns(i),
		 ( uHTR_epbvtc( i) & 1) ? "CRC" : "",
		 uHTR_us(i) ? "US" : "");
	  
	  
      }
    }

    printf("\n");
    for( int i=0; i<24; i++) {
      uint32_t addr = 4*i;
      if( (i%8) == 0)
	printf("%04x: ", addr);
      printf(" %08x", m_raw[i]);
      if( (i%8) == 7)
	printf("\n");
    }
    printf("\n");

    if( level >= 3)
      DumpAllTP();

    if( level == 4)		// level 4, uHTR payloads in hex
      DumpAllUHTR();
    else if( level == 5) 		// level 5, dump decoded QIE data
      DumpAllQIE();
    else if( level == 6) {
      DumpAllUHTR();
      DumpAllQIE();
    } else if( level == 7) {
      for( int h=0; h<FEDGET_NUM_UHTR; h++)
	DumpUHTRQIERaw(h);
    } else if( level == 8) {
      printf("Front-End Link data integrity ('-'=nDV/err '*'=capID err '.'=no data\n");
      DumpAllQIEStatus();
      printf("\n");
    } else if( level == 9) {
      printf("Front-End Link data integrity ('-'=nDV/err '*'=capID err '.'=no data\n");
      DumpAllQIEStatus();
      printf("\n");
      DumpAllUHTR();
      DumpAllQIE();
    }
  }
}


// decode bits of QIE
uint8_t FedGet_Run3::QIE_mant(uint16_t r) { return r & 0x1f; } ///< QIE mantissa
uint8_t FedGet_Run3::QIE_range(uint16_t r) { return (r>>5) & 3; } ///< QIE range
uint8_t FedGet_Run3::QIE_capID(uint16_t r) { return (r>>7) & 3; } ///< QIE cap ID
uint8_t FedGet_Run3::QIE_chipID(uint16_t r) { return (r>>11) & 3; } ///< QIE chip ID
uint8_t FedGet_Run3::QIE_fiberID(uint16_t r) { return (r>>13) & 7; } ///< QIE fiber ID
uint8_t FedGet_Run3::QIE_err(uint16_t r) { return (r>>10) & 1; } ///< QIE link error bit
uint8_t FedGet_Run3::QIE_dv(uint16_t r) { return( r>>9) & 1; } ///< QIE data valid bit

const char* FedGet_Run3::QIE_status_name( int s) {
  return qie_status_name[s];
}

const char* FedGet_Run3::QIE_status_names( int v) {
  int i, b;
  static char names[80];
  *names = '\0';
  for( i=0, b=1; i<QIE_STATUS_MAX; i++, b<<=1) {
    if( v & b) {
      strcat( names, " ");
      strcat( names, QIE_status_name( i));
    }
  }
  
  return names;
}

const char* FedGet_Run3::qie_status_name[] = 
  { "nDV/Err", "capID", "noData", "BadnTS", "AOK" };


char FedGet_Run3::hwname[] = {'H','d','r',' ','9','9','9'};
char* FedGet_Run3::uHTRWordName( int n)
{
  if( n <= 7)
    snprintf( hwname, sizeof(hwname), "Hdr %d", n+1);
  else
    strcpy( hwname, "-");
  
  return hwname;
}

