//
// Test.cc - sample analyzer do dump events
//   using unpack and dump tools from EricDIM
//

//
// Original Author:  Eric Shearer Hazen
//         Created:  Thu Jun 11 18:57:46 CEST 2009
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


// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
//
// class declaration
//

using namespace std;
using namespace edm;

// Private include files
#include "HcalDIM/HcalHexInspector/interface/FedErrors.hh"
#include "HcalDIM/HcalHexInspector/interface/crc16d64.hh"
#include "HcalDIM/HcalHexInspector/interface/meta.hh"
#include "HcalDIM/HcalHexInspector/interface/DataIntegrity.hh"

class Test : public edm::one::EDAnalyzer<> {
   public:
      explicit Test(const edm::ParameterSet&);
      ~Test();


   private:
      virtual void beginJob(const edm::EventSetup&) ;
      virtual void analyze(const edm::Event&, const edm::EventSetup&);
      virtual void endJob() ;

      // ----------member data ---------------------------
      edm::EDGetTokenT<FEDRawDataCollection> fedCollection_token;

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
Test::Test(const edm::ParameterSet& iConfig)

{
   //now do what ever initialization is needed
   //fedCollection_token = consumes<FEDRawDataCollection>(iConfig.getParameter<edm::InputTag>("rawDataCollector"));
  fedCollection_token = consumes<FEDRawDataCollection>(edm::InputTag("rawDataCollector"));
}


Test::~Test()
{
 
   // do anything here that needs to be done at desctruction time
   // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

// ------------ method called to for each event  ------------
void
Test::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
   using namespace edm;

    // get the raw data
   Handle<FEDRawDataCollection> rawdata;
   iEvent.getByToken(fedCollection_token,rawdata); // ALBERTO: fix label

   int evn = iEvent.id().event();
   
   // loop over FED numbers if you like, or just use a fixed values as below
   //
   const FEDRawData& data = rawdata->FEDData( 700);
   size_t size=data.size();

   // select big events (arbitrary value!)
   //
   if( size > 1700) {
     cout << "Event Number " << evn << " FED 700 size " << size << " words" << endl;

     // access raw data for this FED
     uint32_t* payload=(uint32_t*)(data.data());
     size_t size32 = data.size()/sizeof(uint32_t);

     // run our unpacker on it
     FedGet fed( payload, size32, 0x2c36);

     // dump it.  Various values give various format for output.
     // 4 is a reasonable value
     fed.Dump( 4);
   }
}


// ------------ method called once each job just before starting event loop  ------------
void 
Test::beginJob(const edm::EventSetup&)
{
}

// ------------ method called once each job just after ending the event loop  ------------
void 
Test::endJob() {
}

//define this as a plug-in
DEFINE_FWK_MODULE(Test);
