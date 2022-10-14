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
   
   // Unclear why this does not work, but HCAL FEDs are 700-731

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
   const FEDRawData& data = rawdata->FEDData(731);
   size_t size=data.size();
   
   // Let us look in detail at the data
   cout << "Event Number " << evn
     //	<< " FED " << fedList[0]
	<< " FED " << "731"
	<< " size " << size
	<< " words" << endl;
   
   // access raw data for this FED
   uint32_t* payload=(uint32_t*)(data.data());
   size_t size32 = data.size()/sizeof(uint32_t);
   
   // Find length of data? size32 = number of 32-bit words?
   cout << " FED " << "731"
	<< " size32 " << size32
	<< " payload " << payload << endl;

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
