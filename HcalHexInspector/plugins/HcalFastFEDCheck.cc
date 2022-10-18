//
// HcalFastFEDCheck.cc - sample analyzer
//

//
// Original Author:  Alberto Belloni
//         Created:  Thu Oct 13 16:19:46 CEST 2022
// $Id$
//
//
// system include files

#include <memory>
#include <iostream>
#include <ios>

#include <DataFormats/FEDRawData/interface/FEDRawDataCollection.h>
#include <DataFormats/FEDRawData/interface/FEDHeader.h>
#include <DataFormats/FEDRawData/interface/FEDTrailer.h>
#include <DataFormats/FEDRawData/interface/FEDNumbering.h>

//#include "EventFilter/HcalRawToDigi/interface/HcalUnpacker.h"
//#include "CalibFormats/HcalObjects/interface/HcalDbService.h"
//#include "CalibFormats/HcalObjects/interface/HcalDbRecord.h"
#include "EventFilter/HcalRawToDigi/interface/HcalFEDList.h"

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
//#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Event.h"
//#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Framework/interface/MakerMacros.h"

//#include "FWCore/Framework/interface/EventSetup.h"
//#include "FWCore/Utilities/interface/ESGetToken.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"


//
// class declaration
//

using namespace std;
using namespace edm;

//// Private include files
//#include "HcalDIM/HcalHexInspector/interface/FedErrors.hh"
//#include "HcalDIM/HcalHexInspector/interface/crc16d64.hh"
//#include "HcalDIM/HcalHexInspector/interface/meta.hh"
//#include "HcalDIM/HcalHexInspector/interface/DataIntegrity.hh"
#include "HcalDIM/HcalHexInspector/interface/FedGet_Run3.hh"

class HcalFastFEDCheck : public edm::one::EDAnalyzer<> {
public:
  explicit HcalFastFEDCheck(const edm::ParameterSet&);
  ~HcalFastFEDCheck();
  
  
private:
  virtual void beginJob(const edm::EventSetup&) ;
  virtual void analyze(const edm::Event&, const edm::EventSetup&);
  virtual void endJob() ;
  
  // parameters from config file
  int32_t dumpLevel_;		// if > 0, dump unpacked data
  int32_t fedNumber_;		// FED number, uHTR 1100-1199, HTR 700-731

  
  // ----------member data ---------------------------
  edm::EDGetTokenT<FEDRawDataCollection> tok_data_;
  //edm::ESGetToken<HcalDbService, HcalDbRecord> tok_dbService_;
  //HcalUnpacker unpacker_;
  

//HcalFEDList* fedList_;

};

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
HcalFastFEDCheck::HcalFastFEDCheck(const edm::ParameterSet& iConfig)

{

  dumpLevel_=iConfig.getUntrackedParameter<int32_t>("dumpLevel",0);  
  fedNumber_=iConfig.getUntrackedParameter<int32_t>("fedNumber",1100);

  //unpacker_ = HcalUnpacker(...)

  //now do what ever initialization is needed
  tok_data_ = 
    consumes<FEDRawDataCollection>(edm::InputTag("rawDataCollector"));
  //tok_dbService_ = esConsumes<HcalDbService, HcalDbRecord>();


  //fedList_ = new HcalFEDList();

}


HcalFastFEDCheck::~HcalFastFEDCheck()
{
 
   // do anything here that needs to be done at desctruction time
   // (e.g. close files, deallocate resources etc.)
  
  //delete fedList_;

}


//
// member functions
//

// ------------ method called to for each event  ------------
void
HcalFastFEDCheck::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
   using namespace edm;

    // get the raw data
   Handle<FEDRawDataCollection> rawdata;
   iEvent.getByToken(tok_data_,rawdata); // ALBERTO: fix label

   int evn = iEvent.id().event();
   
   // fedList_ is an object of class HcalFEDList, fedList is an std::vector
   HcalFEDList hcalFEDs;
   //vector<int> fedList  = fedList_->getListOfFEDs();
   vector<int> fedList  = hcalFEDs.getListOfFEDs();
   
   // Unclear why this does not work, but HCAL HTR FEDs are 700-731
   // HCAL uHTR FED are different range:
   // MINHCALuTCAFEDID = 1100
   // MAXHCALuTCAFEDID = 1199

   // Let us check if the list of FEDs makes sense
   for (unsigned int fed =0; fed < fedList.size(); ++fed)
     cout << "HCAL FED#: " << fed << endl;

   // Often have troubles with FEDs
   //if (fedList.size()==0) {
   //  cout << "Uhm, the FED list is empty? Bailing out" << endl;
   //  return;
   //}

   // For the time being, let us only look at the first FED in the list
   //const FEDRawData& data = rawdata->FEDData(fedList[0]);
   const FEDRawData& data = rawdata->FEDData(fedNumber_);
   size_t size=data.size();
   
   // Let us look in detail at the data
   if (dumpLevel_ > 3)
     cout << "Event Number " << evn
	  << " FED " << fedNumber_
	  << " size " << size
	  << " words" << endl;

   // access raw data for this FED
   uint32_t* payload=(uint32_t*)(data.data());
   const size_t size32 = data.size()/sizeof(uint32_t);

   if (size32==0) {
     cout << " No data?" << endl;
     return;
   }

   // Let us look in detail at the data
   if (dumpLevel_ > 2)
     cout << "Event Number " << evn
	  << " FED " << fedNumber_
	  << " size32 " << size32
	  << " words" << endl;

   // Note that this is first half of header (has total of 128 bits)
   uint64_t header = payload[0] + (uint64_t(1) << 32) * payload[1];
   uint64_t header2 = payload[2] + (uint64_t(1) << 32) * payload[3];
   uint64_t trailer = payload[size32-2] +
     (uint64_t(1) << 32) * payload[size32-1];
   if (dumpLevel_ > 2) {
     cout << "   Header: " << hex << header
	  << " " << header2 << dec << endl;
     cout << "   Trailer: " << hex << trailer << dec << endl;
   }

   // Split the 32-bit blocks into 16-bit ones
   uint16_t payload16[size32*2];
   for (unsigned int i=0; i<size32; ++i) {
     payload16[2*i] = payload[i] & 0xffff;
     payload16[2*i+1] = (payload[i] >> 16) & 0xffff;
     //cout << " Word " << (2*i) << ": "
     //	  << hex << payload16[2*i] << dec << endl
     //	  << " Word " << (2*i+1) << ": "
     //	  << hex << payload16[2*i+1] << dec << endl;
   }

   // For completeness, print the 32-bit words
   //for (unsigned int i=0; i<size32;++i)
   //  cout << "Word 32b " << i << ": " << hex << payload[i] << dec << endl;

   // Extract all the info from the header
   if (dumpLevel_ > 3)
     cout << " READING HEADER FOR EVENT " << evn << endl
	  << " Data length: " << (header & 0xfffff) << endl
	  << " BcN: " << ((header >> 20) & 0xfff) << endl
	  << " EvN: " << ((header >> 32) & 0xfffff) << endl
	  << " trailer-type EvN: " << ((header >> 32) & 0xff) << endl
	  << " AMC: " << ((header >> 56) & 0xff) << endl
	  << " CrateId: " << ((header2 >> 0) & 0xff) << endl
	  << " Slot: " << ((header2 >> 8) & 0xf) << endl
	  << " Presamples: " << ((header2 >> 12) & 0xf) << endl
	<< " OrN: " << ((header2 >> 16) & 0xffff) << endl
	  << " FW flavor: " << ((header2 >> 32) & 0xff) << endl
	  << " Evt type: " << ((header2 >> 40) & 0xf) << endl
	  << " Payload format=1: " << ((header2 >> 44) & 0xf) << endl
	  << " FW version: " << ((header2 >> 48) & 0xffff) << endl
	  << endl;

   // Extract all the info from the trailer
   if (dumpLevel_ > 3)
     cout << " READING TRAILER FOR EVENT " << evn << endl
	  << " Data length: " << (trailer & 0xfffff) << endl
	  << " Default=0: " << ((trailer >> 20) & 0xf) << endl
	  << " EvN: " << ((trailer >> 24) & 0xff) << endl
	  << " CRC32: " << ((trailer >> 32) & 0xffffffff) << endl
	  << endl;
   
   //for (unsigned int i=0; i<size32; ++i) {
   //  cout << "wd " << i << " " << hex << payload[i] << dec << endl;
   //  cout << " payload16 1 and 2 " << hex << payload16[2*i]
   //	  << " " << payload16[2*i+1] << dec << endl;
   //}
   //cout << endl;

   // Let us now print the event type
   for (unsigned i=0; i<size32*2; i++) {

     // Skip header and trailer (64 and 32 bits, respectively)
     if (i<8 || i>=(size32-1)*2)
       continue;

     // Channel data have 0 in bit 15 (counting from 0)
     if (((payload16[i] >> 15) & 1) == 0)
       continue;

     // This is the first word of each channel data block
     // If value is 1, we can check for flavor
     if (((payload16[i] >> 15) & 1) == 1)
       //cout << "Flavor: " << ((payload16[i] >> 12) & 3) << endl;
       if (((payload16[i] >> 12) & 7)==7)
	 if (dumpLevel_ > 0)
	   cout << "Found flavor 7 " << evn << endl;
   }


   // 358831 - 10h long
   // 358834 - 50' long
   // 358835 - 2h long


   // run our unpacker on it
   FedGet_Run3 fed( payload, size32, 0x2c36);
   
   // dump it.  Various values give various format for output.
   // 4 is a reasonable value
   fed.Dump( dumpLevel_ );

}


// ------------ method called once each job just before starting event loop  ------------
void 
HcalFastFEDCheck::beginJob(const edm::EventSetup&)
{
}

// ------------ method called once each job just after ending the event loop  ------------
void 
HcalFastFEDCheck::endJob() {
}

//define this as a plug-in
DEFINE_FWK_MODULE(HcalFastFEDCheck);
