//
// DCCv4 output format info
//

#define MAX_WORDS 4096		// maximum event size
#define MY_SENTINEL 0xdeadbeef

// CDF stuff (word numbers are 32-bit words)
#define CDF_HDR_SIZE 4		// CDF header size
#define CDF_TRL_SIZE 2		// CDF trailer size

#define EVN_WD 1		// event number word
#define EVN_BIT 0		// event number lsb bit#
#define EVN_MASK 0xffffff

#define BX_ID_WD 0
#define BX_ID_BIT 20
#define BX_ID_MASK 0xfff

#define SRC_ID_WD 0
#define SRC_ID_BIT 8
#define SRC_ID_MASK 0xfff

// DCC header
#define DCC_HDR_SIZE 2

// stuff in HTR headers
#define HH_SIZE 18		// number of HTR headers

#define HH_WC_MASK 0x3ff
#define HH_HTR_ERR_BIT 24
#define HH_HTR_ERR_MASK 0xff
#define HH_LRB_ERR_BIT 16
#define HH_LRB_ERR_MASK 0xff

#define HH_ENA_BIT 15
#define HH_PRSNT_BIT 14
#define HH_BCN_OK_BIT 13
#define HH_EVN_OK_BIT 12


// stuff within HTR data

// header info... offset from start of block
#define HTR_DATA 4		// offset to start of data

#define HTR_ID_WORD 1
#define HTR_ID_BIT 16
#define HTR_ID_MASK 0xff

#define HTR_BCN_WORD 2
#define HTR_BCN_BIT 0
#define HTR_BCN_MASK 0xfff

#define HTR_NTP_WORD 2
#define HTR_NTP_BIT 24
#define HTR_NTP_MASK 0xff

// trailer info... offset from start of next HTR
#define HTR_LRB_TRAILER -1
#define HTR_NSAMP -2

#define HTR_NDD_BIT 0
#define HTR_NDD_MASK 0xff

#define HTR_NSAMP_BIT 11
#define HTR_NSAMP_MASK 0x1f

