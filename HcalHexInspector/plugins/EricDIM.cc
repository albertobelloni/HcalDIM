// FIll root histos with fixed pattern to test for bugs in CMSSW
// #define BOGUS_FILL

//
// "EricDIM" - Eric's Data Integrity Monitor
// Make a series of plots from HCAL data
//
// Requires various other classes:  EH1, DataIntegrity, FedGet, FedErrors, EH1_TGraph
//
// 28 Jul 08, esh - add front-end data integrity plots
// 01 Apr 08, esh - add dump flags for input to EricDump
//

// Don't use until they fix it!

// system include files
#include <memory>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <strings.h>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"


// TFileService is broken... use raw Root classes instead (can't write non-histograms)
#include "TFile.h"
#include "TDirectory.h"

#include <DataFormats/FEDRawData/interface/FEDRawDataCollection.h>
#include <DataFormats/FEDRawData/interface/FEDHeader.h>
#include <DataFormats/FEDRawData/interface/FEDTrailer.h>
#include <DataFormats/FEDRawData/interface/FEDNumbering.h>

#include "TH1.h"
#include "TH2.h"
#include "TGraphAsymmErrors.h"

// Private include files
#include "HcalDIM/HcalHexInspector/interface/FedErrors.hh"
#include "HcalDIM/HcalHexInspector/interface/crc16d64.hh"
#include "HcalDIM/HcalHexInspector/interface/meta.hh"
#include "HcalDIM/HcalHexInspector/interface/DataIntegrity.hh"

using namespace std;
using namespace edm;

void print_qie_histo( int* histo, int max_count);

//
// class declaration
//

class HcalHexInspector : public edm::EDAnalyzer {
public:
  explicit HcalHexInspector(const edm::ParameterSet&);
  ~HcalHexInspector();
  void SetupHistograms( int);

private:
  
  // parameters from config file
  std::vector<int> ids_;	// vector of IDs as parameter
  std::set<int> FEDids_;	// list of FEDs to look at (probably should
                                // do away with this)
  int *fed_ids;			// simple array of ids  (copy of above)

  int32_t dumpLevel_;		// if > 0, dump unpacked data

  bool debug_;			// enable various debug output
  bool doCRC_;			// calculate CRC (expensive!)
  uint32_t DCCVersion_;		// required DCC firmware version for correct unpacking
  std::string RootFileName_;	// where to put the output plots

  int first_eventnumber_;       // starting event number
  int last_eventnumber_;        // ending event number
  int abort_count_;             // stop after processing this many records
  bool dumpBinary_;		// dump binary data in deadbeef format
  std::string BinaryFileName_;	// where to put the beef

  bool check_qie_data_;		// check front-end link data
  bool histo_qie_data_;		// experimental histograms of QIE data

  bool suppress_qie_ok_;	// suppress AOK and ZS states in QIE plots

  bool write_roots_;		// Explicitly write TH2 objects to file

  bool dumpError_;		// dump binary data in deadbeef format
  std::string ErrorFileName_;	// file name for error summary

  FILE* binFP;			// file pointer for binary dumps

  int check_evn_count_;		// number of EvN mismatch to print
  int check_orn_count_;		// number of OrN mismatch to print
  int check_bcn_count_;		// number of BcN mismatch to print

  int remain_evn_count;
  int remain_orn_count;
  int remain_bcn_count;

  bool time_to_quit;		// flag: abort now

  bool SetupHistogramsCalled;

  virtual void beginJob(const edm::EventSetup&) ;
  virtual void analyze(const edm::Event&, const edm::EventSetup&);
  virtual void endJob() ;

  // pointers to our Root objects
  TH1D * h_payloadSize;		// payload size - overall
  TH1D * h_payloadPart[ERIC_PART_COUNT]; // payload size - by partition
  TH1D * h_payloadByFED[ERIC_NUM_FED]; // payload size - by FED

  // size of various components of event for size investigations
  TH1D * h_payloadSizeTP;	// payload size - TP words
  TH1D * h_payloadSizeNDD;	// payload size - number of DAQ data words

  TH1D * h_zs_sumPart[ERIC_PART_COUNT];	// sliding zs sum max by partition
  TH1D * h_zs_sumFED[ERIC_NUM_FED];	// sliding zs sum max by FED

  // <><> occupancy is not trivial... work in progress
  //  TH1D * h_occuAll;		// overall channel occupancy
  //  TH1D * h_occuPart[ERIC_PART_COUNT]; // channel occupancy by partition
  // <><>

  TH1D * h_qie_test;		// QIE test histogram

  DataIntegrity* m_DI;
  TH2D * h_ovp;			// overview plot
  TH2D * h_fhp;			// FED/HTR plot
  TH2D * h_fhdp;		// FED/HTR digi plot

  // by-FED plots
  TH2D* h_tsp[ERIC_NUM_FED];
  TH2D* h_hdp[ERIC_NUM_FED];
  TGraphAsymmErrors* h_dsp[ERIC_NUM_FED];
  TGraphAsymmErrors* h_trp[ERIC_NUM_FED];
  TGraphAsymmErrors* h_drp[ERIC_NUM_FED];

  TFile* fs;

  // subdirectories for by-FED plots
  TDirectory* s_fed[ERIC_NUM_FED];

  // start time (from first event in file) for each FED (seconds)
  double FedStartTime[ERIC_NUM_FED];

  int nev;			// count number of events accepted
  int nseen;			// count number of events seen
  
  uint16_t htr_status_summary[ERIC_NUM_FED][FEDGET_NUM_HTR];
  uint8_t lrb_status_summary[ERIC_NUM_FED][FEDGET_NUM_HTR];

  // Overall error summary by FED
  FedErrors FedErrorSummary[ERIC_NUM_FED];

  // keep track of front-end status with our own histogram
  int QIE_status[ERIC_NUM_FED][FEDGET_NUM_HTR][FEDGET_NUM_CHAN][QIE_STATUS_MAX];
  // count events with a payload by HTR
  int HTR_evt_count[ERIC_NUM_FED][FEDGET_NUM_HTR];
  // count events by channel
  int QIE_count[ERIC_NUM_FED][FEDGET_NUM_HTR][FEDGET_NUM_CHAN];
  // number of incorrect time-sample counts (should be = ndd for non-ZS running)
  int QIE_TS_wrong[ERIC_NUM_FED][FEDGET_NUM_HTR][FEDGET_NUM_CHAN];
  // total number of TS to calculate mean
  int QIE_TS_total[ERIC_NUM_FED][FEDGET_NUM_HTR][FEDGET_NUM_CHAN];

  // New method to access raw data: the token
  edm::EDGetTokenT<FEDRawDataCollection> fedCollection_token;


};

//
// Setup histograms, open files -- only after we have the run number
//
void HcalHexInspector::SetupHistograms( int runNumber)
{
  if( !SetupHistogramsCalled) {

    // make file name 
    if( RootFileName_ == "Not.Specified.root") {
      char tmp[50];
      snprintf( tmp, 50, "EricDIM%06d.root", runNumber);
      RootFileName_ = tmp;
    }

    fs = new TFile( RootFileName_.c_str(), "recreate");

    if( dumpError_) {

      if( ErrorFileName_ == "Not.Specified.txt") {
	char tmp[50];
	snprintf( tmp, 50, "EricDIM%06d_errors.txt", runNumber);
	ErrorFileName_ = tmp;
      }

      if( ( binFP = fopen( ErrorFileName_.c_str(), "w")) == NULL) {
	fprintf( stderr, "Error opening binary dump file %s\n", ErrorFileName_.c_str());
	abort();
      }      
    }

    if( dumpBinary_) {

      if( BinaryFileName_ == "Not.Specified.dat") {
	char tmp[50];
	snprintf( tmp, 50, "EricDIM%06d_raw.dat", runNumber);
	BinaryFileName_ = tmp;
      }

      if( ( binFP = fopen( BinaryFileName_.c_str(), "w")) == NULL) {
	fprintf( stderr, "Error opening binary dump file %s\n", BinaryFileName_.c_str());
	abort();
      }

    }

    if( debug_) printf("HcalHexInspector::SetupHistograms( %d)... root file: %s  bin file: %s\n",
		       runNumber, RootFileName_.c_str(), BinaryFileName_.c_str());

    // create empty histograms... need to use the full constructor though
    h_ovp = new TH2D("dummy2","dummy",10,0,1,10,0,1);
    h_fhp = new TH2D("dummy3","dummy",10,0,1,10,0,1);
    h_fhdp = new TH2D("dummy4","dummy",10,0,1,10,0,1);

    // loop over the FEDs, create a list and the histograms
    fed_ids = (int *)EH1::my_calloc( ids_.size(), sizeof(int));
    int k=0;
    for (std::vector<int>::iterator i=ids_.begin(); i!=ids_.end(); i++) {
      char name[40];
      char title[100];

      FEDids_.insert(*i);
      fed_ids[k] = *i;

      // make a ROOT directory for the FED
      sprintf( name, "FED_%03d", *i);
      if( debug_) printf("Making directory %s for fed %d\n", name, k);
      fs->mkdir( name);
      s_fed[k] = fs->GetDirectory( name);
      fs->cd(name);

      // book the TTS states plot
      snprintf( name, sizeof(name), "FED%dTTSStates", *i);
      if( debug_) printf("Booking TTS states plot %s for fed %d\n", name, k);
      h_tsp[k] = new TH2D( name ,"dummy title",10,0,1,10,0,1);

      // book the HTR digi plot
      snprintf( name, sizeof(name), "FED%dDigiErrors", *i);
      if( debug_) printf("Booking Fed Digi plot %s for fed %d\n", name, k);
      h_hdp[k] = new TH2D( name ,"dummy title",10,0,1,10,0,1);

      // book the data size plot
      snprintf( name, sizeof(name), "FED%dDataSize", *i);
      if( debug_) printf("Booking data size plot %s for fed %d\n", name, k);
      h_dsp[k] = new TGraphAsymmErrors(10);
      h_dsp[k]->SetName( name);

      //    if( debug_)
      //      printf("Creating new TGraphAsymmErrors as s_fed[%d] with name %s\n", k, name);

      // book the trigger time plot
      snprintf( name, sizeof(name), "FED%dTriggerRate", *i);
      if( debug_) printf("Booking trigger time plot %s for fed %d\n", name, k);
      h_trp[k] = new TGraphAsymmErrors(10);
      h_trp[k]->SetName(name);

      // book the data rate plot (not used yet)
      snprintf( name, sizeof(name), "FED%dDataRate", *i);
      if( debug_) printf("Booking data rate plot %s for fed %d\n", name, k);
      h_drp[k] = new TGraphAsymmErrors(10);
      h_drp[k]->SetName(name);

      // per-FED sliding ZS sum
      snprintf( name, sizeof(name), "SlidingPairSumMaxFED%d", *i);
      snprintf( title, sizeof(title), "FED %d Sliding Pair Sum Maxes", *i);
      h_zs_sumFED[k] = new TH1D( name, title, 100, 0, 100);

      // per-FED payload size
      snprintf( name, sizeof(name), "PayloadSizeFED%d", *i);
      snprintf( title, sizeof(title), "FED %d Payload Size (32-bit words)", *i);
      h_payloadByFED[k] = new TH1D( name, title, 100, 0, 4000);

      fs->cd();			// back to top directory

      ++k;
    }

    // create a DataIntegrity plot set
    m_DI = new DataIntegrity( FEDids_.size(),
			      fed_ids,
			      h_ovp,
			      h_fhp,
			      h_fhdp,
			      h_tsp,
			      h_hdp,
			      h_dsp,
			      h_trp,
			      h_drp,
			      debug_);

    m_DI->SuppressQIEOK( suppress_qie_ok_);

    // histograms of data size
    h_payloadSize = new TH1D( "DataSize", "Payload Size (32-bit words)", 100, 0, 4000);
    h_payloadSizeTP = new TH1D( "DataSizeTP", "Number of TP words per HTR", 100, 0, 100);
    h_payloadSizeNDD = new TH1D( "DataSizeNDD", "Number of DAQ data words per HTR", 100, 0, 1000);

    // histogram of occupancy
    //    h_occuAll = new TH1D( "OccupancyAll", "Channel Occupancy (per HTR, overall)", 100, 0, 1);

    for( int i=0; i<ERIC_PART_COUNT; i++) {
      char name[40];
      char title[80];

      // per-partition payload size
      snprintf( name, sizeof(name), "PayloadSize%s", DataIntegrity::PartitionName[i]);
      snprintf( title, sizeof(title), "%s Payload Size (32-bit words)", DataIntegrity::PartitionName[i]);
      h_payloadPart[i] = new TH1D( name, title, 100, 0, 4000);

      // per-partition sliding ZS sum
      snprintf( name, sizeof(name), "SlidingPairSumMax%s", DataIntegrity::PartitionName[i]);
      snprintf( title, sizeof(title), "%s Sliding Pair Sum Maxes", DataIntegrity::PartitionName[i]);
      h_zs_sumPart[i] = new TH1D( name, title, 100, 0, 100);

      // per-partition occupancy
      //      snprintf( name, sizeof(name), "%sOccupancy", DataIntegrity::PartitionName[i]);
      //      snprintf( title, sizeof(title), "%s Occupancy (per HTR)", DataIntegrity::PartitionName[i]);
      //      h_occuPart[i] = new TH1D( name, title, 100, 0, 1);
    }

    // histogram of QIE sum
    h_qie_test = new TH1D( "testQIESum", "QIE Sum (fC)", 100, 0, 10000);
  }


}

//
// constructors and destructor
//
HcalHexInspector::HcalHexInspector(const edm::ParameterSet& pset)
{

  // get config parameters
  ids_ = pset.getUntrackedParameter<std::vector<int> >("feds",std::vector<int>());
  first_eventnumber_=pset.getUntrackedParameter<int>("first_eventnumber",0);  
  last_eventnumber_=pset.getUntrackedParameter<int>("last_eventnumber",0x1000000);  
  check_evn_count_ = pset.getUntrackedParameter<int>("check_evn_count", 0);
  check_bcn_count_ = pset.getUntrackedParameter<int>("check_bcn_count", 0);
  check_orn_count_ = pset.getUntrackedParameter<int>("check_orn_count", 0);
  abort_count_=pset.getUntrackedParameter<int>("abort_count",0x1000000);  
  dumpLevel_=pset.getUntrackedParameter<int32_t>("dumpLevel",0);  
  debug_=pset.getUntrackedParameter<bool>("debug",false);  
  DCCVersion_ = pset.getUntrackedParameter<uint32_t>("DCCversion",0);
  doCRC_ = pset.getUntrackedParameter<bool>("doCRC",false);  
  RootFileName_ = pset.getUntrackedParameter<string>("RootFileName","Not.Specified.root");
  dumpBinary_=pset.getUntrackedParameter<bool>("dumpBinary",false);  
  BinaryFileName_ = pset.getUntrackedParameter<string>("BinaryFileName","Not.Specified.dat");  
  dumpError_=pset.getUntrackedParameter<bool>("dumpError",false);  
  ErrorFileName_ = pset.getUntrackedParameter<string>("ErrorFileName","Not.Specified.txt");  
  check_qie_data_ = pset.getUntrackedParameter<bool>("CheckQIEData",false);  
  histo_qie_data_ = pset.getUntrackedParameter<bool>("HistoQIEData",false);  
  suppress_qie_ok_ = pset.getUntrackedParameter<bool>("SuppressQIEOK",false);
  write_roots_ = pset.getUntrackedParameter<bool>("ForceWriteHistos",false);
  

  printf("dumpLevel_ = %d\n", dumpLevel_);
  printf("EvN range ( %d - %d )\n", first_eventnumber_, last_eventnumber_);

  memset( FedStartTime, 0, sizeof(FedStartTime));
  nev = 0;
  nseen = 0;

  time_to_quit = false;

  memset( htr_status_summary, 0, sizeof(htr_status_summary));
  memset( lrb_status_summary, 0, sizeof(lrb_status_summary));
  memset( QIE_status, 0, sizeof( QIE_status));
  memset( HTR_evt_count, 0, sizeof( HTR_evt_count));
  memset( QIE_count, 0, sizeof( QIE_count));
  memset( QIE_TS_wrong, 0, sizeof( QIE_TS_wrong));
  memset( QIE_TS_total, 0, sizeof( QIE_TS_total));

  remain_evn_count = check_evn_count_;
  remain_orn_count = check_orn_count_;
  remain_bcn_count = check_bcn_count_;

  SetupHistogramsCalled = false;

  fedCollection_token = consumes<FEDRawDataCollection>(edm::InputTag("rawDataCollector"));

  printf("HcalHexInspector::HcalHexInspector() finished\n");
}


HcalHexInspector::~HcalHexInspector()
{
}


//
// member functions
//

// ------------ method called to for each event  ------------
void
HcalHexInspector::analyze(const edm::Event& e, const edm::EventSetup& iSetup)
{
  try {

    ++nseen;
    if( nseen > abort_count_ || time_to_quit) {
      cout << "ABORT after " << nseen << " events because ";
      if( time_to_quit)
	cout << "found requested number of mis-matches" << endl;
      else
	cout << "abort_count_ (" << abort_count_ << ") events processed" << endl;
      endJob();
      abort();
    }

    int evnted=e.id().event();
    if( nseen < 10) {
      cout << "--- Run: " << e.id().run()
	   << " Event: " << e.id().event() << " nev=" << nev << endl;
    }
    if (evnted<first_eventnumber_ || evnted>last_eventnumber_) {
      if( nseen < 10)
	printf("Rejected event %d\n", evnted);
      return;
    }

    if( nev == 0) {		// do "first event" tasks
      // set up the histograms
      SetupHistograms( e.id().run());

      // capture the run number for plot titles
      m_DI->SetRunNo( e.id().run());
    }
    ++nev;

    // get the raw data
    Handle<FEDRawDataCollection> rawdata;
    e.getByToken(fedCollection_token,rawdata); // ALBERTO: fix label

    uint32_t evn = e.id().event();

    if( dumpLevel_)
      printf("--------------------- Event %d ------------------------\n", evn);

    for (int i = 0; i<FEDNumbering::lastFEDId(); i++){
      const FEDRawData& data = rawdata->FEDData(i);
      size_t size=data.size();

      if (size>0 && (FEDids_.empty() || FEDids_.find(i)!=FEDids_.end())) {
	  
	FEDHeader header(data.data());
	FEDTrailer trailer(data.data()+size-8);

	// access raw data
	uint32_t* payload=(uint32_t*)(data.data());
	size_t size32 = data.size()/sizeof(uint32_t);

	// make raw binary dump (disables normal histogramming!)
	if( dumpBinary_) {
	  uint32_t h[2];
	  h[0] = 0xdeadbeef;
	  h[1] = size32;
	  fwrite( h, sizeof(uint32_t), 2, binFP);
	  fwrite( payload, sizeof(uint32_t), size32, binFP);
	}

	// run our unpacker on it
	FedGet fed( payload, size32, DCCVersion_);

	// print unpacked stuff if requested
	if( dumpLevel_) {
	  cout << "--- Run: " << e.id().run()
	       << " Event: " << e.id().event() << endl;
	  fed.Dump( dumpLevel_);
	}

	// get integer FED 0..31
	int i_fed = fed.fed_id() - ERIC_MIN_FED;
	for( int h=0; h<FEDGET_NUM_HTR; h++) {
	  if( fed.HTR_nWords(h)) {
	    htr_status_summary[i_fed][h] |= fed.HTR_status(h);
	    lrb_status_summary[i_fed][h] |= fed.HTR_lrb_err(h);

	    // check for EvN, BcN, OrN mismatch if requested
	    // print specified number of mismatches
	    if( remain_evn_count) {
	      if( fed.EvN() != fed.HTR_EvN(h)) {
		printf("EvN MISMATCH FED %d EvN 0x%06x BcN 0x%03x OrN 0x%x HTR %d EvN was 0x%06x\n",
		       fed.fed_id(), fed.EvN(), fed.BcN(), fed.OrN(), h,
		       fed.HTR_EvN(h));
		if( --remain_evn_count == 0)
		  time_to_quit = true;
	      }
	    }
	    if( remain_bcn_count) {
	      if( fed.BcN() != fed.HTR_BcN(h)) {
		printf("BcN MISMATCH FED %d EvN 0x%06x BcN 0x%03x OrN 0x%x HTR %d BcN was 0x%03x\n",
		       fed.fed_id(), fed.EvN(), fed.BcN(), fed.OrN(), h,
		       fed.HTR_BcN(h));
		if( --remain_bcn_count == 0)
		  time_to_quit = true;
	      }
	    }
	    if( remain_orn_count) {
	      if( (fed.OrN() & 0x1f) != fed.HTR_OrN(h)) {
		printf("OrN MISMATCH FED %d EvN 0x%06x BcN 0x%03x OrN 0x%x HTR %d OrN was 0x%03x\n",
		       fed.fed_id(), fed.EvN(), fed.BcN(), fed.OrN(), h,
		       fed.HTR_OrN(h));
		if( --remain_orn_count == 0) {
		  time_to_quit = true;
		}
	      }
	    }

	    // check QIE data if requested
	    // increment histogram array to keep track of status
	    if( check_qie_data_) {
	      if( debug_ && nseen < 10)
		printf("Checking QIE data\n");
	      ++HTR_evt_count[i_fed][h];
	      for( int c=0; c<FEDGET_NUM_CHAN; c++) {
		{
		  int s = fed.HTR_QIE_status(h, c);
		  if( debug_ && nseen < 10)
		    printf("FED %d HTR %d ch %d status=0x%x\n", fed.fed_id(), h, c, s);
		  for( int b=0; b<QIE_STATUS_MAX; b++) {
		    if( s & (1<<b))
		      QIE_status[i_fed][h][c][b]++;
		  }
		}
		QIE_count[i_fed][h][c]++;
		QIE_TS_total[i_fed][h][c] += fed.HTR_QIE_chan_len( h, c);
		if( fed.HTR_QIE_chan_len( h, c) != fed.HTR_ns(h)) {
		  if( debug_)
		    printf("FED %d HTR %d Ch %d expected %d TS, saw %d\n",
			   i_fed, h, c, fed.HTR_ns(h), fed.HTR_QIE_chan_len( h, c));
		  QIE_TS_wrong[i_fed][h][c]++;
		}
	      }
	    }
	  }
	}

	// parse the unpacked data, check for errors
	FedErrors fe( fed, FedErrors::check_DCC | FedErrors::check_HTR | FedErrors::check_Digi);

	// keep summary by FED
	FedErrorSummary[i_fed].AddErrors( fed,
					  FedErrors::check_DCC |
					  FedErrors::check_HTR | FedErrors::check_Digi);

#ifdef BOGUS_FILL
	fe.SetBogus();
#endif

	m_DI->Fill_overViewPlot( fed.fed_id(), fe);

	m_DI->Fill_fedHtrPlot( fed.fed_id(), fe);

	m_DI->Fill_fedHtrDigiPlot( fed.fed_id(), fe);

#ifdef BOGUS_FILL
	m_DI->Fill_SRPlot( 0, 1, 700, 100);
#else
	m_DI->Fill_SRPlot( 0, evn, fed.fed_id(), size32);
#endif

	m_DI->Fill_HTRDigiPlot( fed.fed_id(), fe);

	// calculate time, fill time in trigger rate plot with raw BX
	double et = ((double)fed.OrN() * 3563.0 + (double)fed.BcN());
	// adjust start time for 0 at start of run for this FED
	if( FedStartTime[i_fed] == 0.0)
	  FedStartTime[i_fed] = et;
	et -= FedStartTime[i_fed];

#ifdef BOGUS_FILL
	m_DI->Fill_SRPlot( 1, 1, 700, 5);
#else
	m_DI->Fill_SRPlot( 1, evn, fed.fed_id(), et);
#endif
	if( debug_)
	  printf("Fill payloadSize=%d for EvN %d FED %d\n", (int) size32, fed.EvN(), fed.fed_id());

	// fill payload size
	h_payloadSize->Fill( size32);
	int part = DataIntegrity::FedToPartition[i_fed];
	h_payloadPart[part]->Fill( size32);
	h_payloadByFED[i_fed]->Fill( size32);

	// fill zs_sum by HTR/ch and FED
	for( int h=0; h<FEDGET_NUM_HTR; h++) {
	  if( fed.HTR_nWords(h)) {

	    // size of event components
	    h_payloadSizeTP->Fill( fed.HTR_ntp(h));
	    h_payloadSizeNDD->Fill( fed.HTR_ndd(h));

	    if( !fed.HTR_us(h)) {	// don't fill unsupressed HTRs

	      for( int ch=0; ch<FEDGET_NUM_CHAN; ch++) {
		int z = fed.HTR_QIE_zs_sum( h, ch);
		if( z >= 0) {
		  h_zs_sumPart[part]->Fill( z);
		  int n = m_DI->m_map_fed[i_fed];
		  h_zs_sumFED[n]->Fill( z);
		}
	      }
	    }
	  }
	}

	// fill TTS state plot
	m_DI->Fill_TTSstatePlot( evn, fed.fed_id(), fed.TTS(), fe);

      }
    }

#ifdef THIS_IS_AN_EVENTSETUP_EXAMPLE
    ESHandle<SetupData> pSetup;
    iSetup.get<SetupRecord>().get(pSetup);
#endif

  } catch( std::exception& e) {
    cout << "EXCEPTION CAUGHT in analyze()" << endl;
    endJob();
    abort();
  }

}


// ------------ method called once each job just before starting event loop  ------------
void 
HcalHexInspector::beginJob(const edm::EventSetup&)
{
}

// ------------ method called once each job just after ending the event loop  ------------
void 
HcalHexInspector::endJob() {

  if( debug_)
    printf( "HcalHexInspector::endJob()...\n");

  printf("Ending job.  Scanned %d events, processed %d events\n", nseen, nev);

  if( dumpError_) {
    ofstream* er_of = new ofstream( ErrorFileName_.c_str() );
    if( !*er_of) {
      cout << "Error opening error file " << ErrorFileName_ << endl;
    } else {
      for( int i=0; i<ERIC_NUM_FED; i++) {
	if( FedErrorSummary[i].Evt_cnt)
	  FedErrorSummary[i].write( er_of);
      }
    }
  }

  if( dumpBinary_) {
    fclose( binFP);
  }

  if( debug_)
    printf( "HcalHexInspector::endJob()...m_DI->UpdatePlots()\n");
  m_DI->UpdatePlots();

  if( debug_)
    printf( "HcalHexInspector::endJob()...fs->Write()\n");
  fs->Write();

  // this should be hidden somewhere
  // write all the Root objects to their subdirectories

  // Apparently not needed, and result in duplicates in root file

  if( write_roots_) {
    h_payloadSize->Write();
    h_payloadSizeTP->Write();
    h_payloadSizeNDD->Write();
    for( int i=0; i<ERIC_PART_COUNT; i++) {
      if( h_payloadPart[i])
	h_payloadPart[i]->Write();
      if( h_zs_sumPart[i])
	h_zs_sumPart[i]->Write();
    }

    if( h_ovp)
      //      fs->WriteObject( h_ovp, h_ovp->GetName());
      h_ovp->Write();
    if( h_fhp)
      //      fs->WriteObject( h_fhp, h_fhp->GetName());
      h_fhp->Write();
    if( h_fhdp)
      //      fs->WriteObject( h_fhdp, h_fhdp->GetName());
      h_fhdp->Write();
  }

  for( unsigned int i=0; i<FEDids_.size(); i++) {
    if( debug_)
      printf( "HcalHexInspector::endJob()...writing plot objects for fed %d\n", i);
    if( write_roots_) {
      // Apparently not needed, and result in duplicates in root file
      if( h_payloadByFED[i])
	h_payloadByFED[i]->Write();
      if( h_tsp[i]) {
	if( debug_) printf(" %s", h_tsp[i]->GetName());
	s_fed[i]->WriteObject( h_tsp[i], h_tsp[i]->GetName());
      }
      if( h_hdp[i]) {
	if( debug_) printf(" %s", h_hdp[i]->GetName());
	s_fed[i]->WriteObject( h_hdp[i], h_hdp[i]->GetName());
      }
      if( h_zs_sumFED[i])
	s_fed[i]->WriteObject( h_zs_sumFED[i], h_zs_sumFED[i]->GetName());
    }
    if( h_dsp[i]) {
      if( debug_) printf(" %s", h_dsp[i]->GetName());
      s_fed[i]->WriteObject( h_dsp[i], h_dsp[i]->GetName());
    }
    if( h_trp[i]) {
      if( debug_) printf(" %s", h_trp[i]->GetName());
      s_fed[i]->WriteObject( h_trp[i], h_trp[i]->GetName());
    }
    if( h_drp[i]) {
      if( debug_) printf(" %s", h_drp[i]->GetName());
      s_fed[i]->WriteObject( h_drp[i], h_drp[i]->GetName());
    }
    if( debug_)
      printf("\n");
  }


  // dump the HTR status word and LRB error summary
  for( int f=0; f<32; f++) {
    printf("Fed %3d: ", f+700);
    for( int h=0; h<FEDGET_NUM_HTR; h++) {
      if( htr_status_summary[f][h])
	printf("%04x%c", htr_status_summary[f][h],
	       (htr_status_summary[f][h] & 0x8f) ? '*' : ' ');
      else
	printf("     ");
    }
    printf("\n");
    printf("         ");
    for( int h=0; h<FEDGET_NUM_HTR; h++) {
      if( lrb_status_summary[f][h])
	printf("  %02x ", lrb_status_summary[f][h]);
      else
	printf("     ");
    }
    printf("\n");
  }

  // dump the QIE status
  // try to minimise the verbosity by summarizing cases which are all the same
  if( check_qie_data_) {
    printf("\nQIE data check (only not-OK channels shown):\n");
    for( int f=0; f<32; f++) {

      // summarize the whole FED histogram
      printf("FED %d\n", f+HCAL_MIN_FED);
      int tmp_fed[QIE_STATUS_MAX];
      int n_fed_data = 0;
      memset( tmp_fed, 0, sizeof(tmp_fed));
      for( int h=0; h<FEDGET_NUM_HTR; h++) {
	if( HTR_evt_count[f][h]) { // any data?
	  n_fed_data += HTR_evt_count[f][h];
	  for( int c=0; c<FEDGET_NUM_CHAN; c++)
	    for( int s=0; s<QIE_STATUS_MAX; s++)
	      tmp_fed[s] += QIE_status[f][h][c][s];
	}
      }
      print_qie_histo( tmp_fed, n_fed_data);
      printf("\n");

      for( int h=0; h<FEDGET_NUM_HTR; h++) {
	// see if this HTR has any data
	if( HTR_evt_count[f][h]) {
	  // summarize the histogram array by HTR
	  int tmp_qie[QIE_STATUS_MAX];
	  memset( tmp_qie, 0, sizeof(tmp_qie));
	  for( int c=0; c<FEDGET_NUM_CHAN; c++)
	    for( int s=0; s<QIE_STATUS_MAX; s++)
	      tmp_qie[s] += QIE_status[f][h][c][s];

	  printf("  HTR %2d ", h);
	  print_qie_histo( tmp_qie, HTR_evt_count[f][h]);
	  printf("\n");

	  // check TS size
	  for( int c=0; c<FEDGET_NUM_CHAN; c++) {
	    if( QIE_TS_wrong[f][h][c])
	      printf("    Ch %d Wrong number of TS %d/%d times\n",
		     c,
		     QIE_TS_wrong[f][h][c],
		     QIE_count[f][h][c]);
	  }
		     

	  // see if all channels in the HTR are the same
	  int n_nonz = 0;
	  int s_nonz = 0;
	  for( int s=0; s<QIE_STATUS_MAX; s++) {
	    if( tmp_qie[s]) {
	      ++n_nonz;
	      s_nonz = s;
	    }
	  }

	  if( n_nonz == 1) {	// only 1 histo bin populated?
	    const char* s = QIE_status_name(s_nonz);
	    printf("  HTR %2d all QIE = %s\n", h, s);
	  } else {		// nope, print a full line for this HTR
	    printf("  HTR %2d\n", h);
	    for( int c=0; c<FEDGET_NUM_CHAN; c++) {
	      printf("    Ch %d ", c);
	      print_qie_histo( QIE_status[f][h][c], QIE_count[f][h][c]);
	      printf("\n");
	    }
	  }
	}
      }
    }
    printf("--End of QIE data check--\n");
  } // if( check_qie_data_)
  
  if( debug_)
    printf("HcalHexInspector::endJob() -- closing root file\n");
  fs->Close();

}

//define this as a plug-in
DEFINE_FWK_MODULE(HcalHexInspector);


// print a QIE status histogram concisely
void print_qie_histo( int* histo, int max_count) {
  for( int s=0; s<QIE_STATUS_MAX; s++) {
    if( histo[s] != 0) {
      if( histo[s] == max_count)
	printf(" %s", QIE_status_name(s));
      else
	printf(" %s=%d", QIE_status_name(s), histo[s]);
    }
  }
}
