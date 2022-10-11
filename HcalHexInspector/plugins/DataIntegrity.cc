#include <stdio.h>
#include <math.h>

#include "HcalDIM/HcalHexInspector/interface/DataIntegrity.hh"

#include "TH2.h"
#include "TGraphAsymmErrors.h"

#include "HcalDIM/HcalHexInspector/interface/EH1_TGraph.hh"
#include "HcalDIM/HcalHexInspector/interface/FedErrors.hh"

// can't figure out how to do this within the class!!
const char* DataIntegrity_ovp_label[] =  {  "DigSiz", "HtrCRC", 
                                      "Fixed", "CRC", "Size", "Fmt Err",
				      "FRL", "HTR", "FEE", "Link Err",
				      "SYN", "TR", "EE", "Data Lost",
				      "OrN", "BcN", "EvN", "Spc", "Num Mis",
     				      "BZ", "OW", "HTR Sta",
				     "SYN", "BSY", "OFW", "RDY", "TTS Sta" };

const char* DataIntegrity_tts_label[] = { " ",
				    "OW", "BZ", "EE", "RL", "LE", "LW", "OD", "CK",
				    "BE", "09", "10", "11", "TM", "HM", "CT", "HTR State",
				    "Ready", "OFW", "Busy", "Sync", "Error", "TTS State",
                                    "EvN", "BcN", "OrN", "HTR Mis" };


// bin for HTR sync loss state
#define ERIC_TTS_HTRSyn 23
// first bin for Fed TTS stuff
#define ERIC_TTS_TTSbin 17
// first bin for HTR stuff
#define ERIC_TTS_HTRbin 1

#define ERIC_OVP_YBINS (sizeof(DataIntegrity_ovp_label)/sizeof(DataIntegrity_ovp_label[0]))
#define ERIC_TTS_YBINS (sizeof(DataIntegrity_tts_label)/sizeof(DataIntegrity_tts_label[0]))

DataIntegrity::DataIntegrity( int nFeds,
			      int *Feds,
			      TH2D* ovp,
			      TH2D* fhp,
			      TH2D* fhdp,
			      TH2D** ttsp,
			      TH2D** hdp,
			      TGraphAsymmErrors** dsp,
			      TGraphAsymmErrors** trp,
			      TGraphAsymmErrors** drp,
			      bool debug
			      ):h_ovp(ovp), h_fhp(fhp), h_fhdp(fhdp), h_ttsp(ttsp), h_hdp(hdp), p_dsp(dsp), p_trp(trp), p_drp(drp),m_debug(debug)
{
  char name[40];

  m_suppress_qie_ok = false;

  if( m_debug)
    printf("DataIntegrity::DataIntegrity() - initialize with %d FEDs\n", nFeds);

  //--- Initialization of FED list
  for( int i=0; i<ERIC_NUM_FED; i++)
    m_map_fed[i] = -1;
  m_nfed = nFeds;


  int p = 0;
  if( nFeds) {
    for( int i=0; i<nFeds; i++) {
      if( Feds[i] >= ERIC_MIN_FED && Feds[i] < (ERIC_MIN_FED+ERIC_NUM_FED)) {
	m_map_fed[Feds[i]-ERIC_MIN_FED] = p++;
	m_fed_num[i] = Feds[i];
	if( m_debug)
	  printf("...fed %d num=%d map[%d] = %d\n",
		 i, Feds[i], Feds[i], p-1);
      }
    }
  }


  //--- set up overview plot
  if( ovp) {
  
    if( m_debug)
      printf("Setup overview plot\n");

    // set name and title
    snprintf( name, sizeof(name), "DataIntegrity");
    ovp->SetName( name);
    
    // set number of bins and limits
    ovp->SetBins( ERIC_OVP_XBINS, 0, ERIC_OVP_XBINS, ERIC_OVP_YBINS, 0, ERIC_OVP_YBINS);

    // label X axis with crate numbers
    TAxis *xaxis = ovp->GetXaxis();

    for( int c=0; c<ERIC_OVP_CRATES; c++) {
      char tmp[20];
      double x = c * (double)ERIC_OVP_CRATE_WID;
      int b1 = xaxis->FindBin( x);
      int b2 = xaxis->FindBin( x+1);
      // now look up the FED
      for( int i=0; i<ERIC_NUM_FED; i++) {
	if( FedToCrate[i] == c) {
	  sprintf( tmp, "%2d %3d", c, i+ERIC_MIN_FED);
	  xaxis->SetBinLabel( b1, tmp);
	}
	if( FedToCrate[i] == ( (double)c + 0.5)) {
	  sprintf( tmp, "%2d/%3d", c, i+ERIC_MIN_FED);
	  xaxis->SetBinLabel( b2, tmp);
	}
      }
    }


    // label Y axis with stuff
    TAxis *yaxis = ovp->GetYaxis();
    for( unsigned int iy=0; iy<ERIC_OVP_YBINS; iy++) {
      int b = yaxis->FindBin( iy);
      yaxis->SetBinLabel( b, DataIntegrity_ovp_label[iy]);
    }

    ovp->SetStats( kFALSE);
  }


  //--- setup FED/HTR plot
  if( fhp) {
    
    // set name and title
    snprintf( name, sizeof(name), "FED HTR Overview");
    fhp->SetName( name);

    fhp->SetBins( ERIC_FHP_XBINS, 0, ERIC_FHP_XBINS, ERIC_FHP_YBINS, 0, ERIC_FHP_YBINS);
    fhp->SetStats( kFALSE);

    // label X axis with FEDs
    TAxis *xaxis = fhp->GetXaxis();
    for( int c=0; c<32; c += 2) {
      char tmp[20];
      int b1 = xaxis->FindBin( ERIC_FHP_X0+c*ERIC_FHP_XS);
      sprintf( tmp, "   %3d", c+ERIC_MIN_FED);
      xaxis->SetBinLabel( b1, tmp);
    }

    // label Y axis with stuff
    TAxis *yaxis = fhp->GetYaxis();
    for( unsigned int h=0; h<15; h++) {
      char tmp[20];
      int b = yaxis->FindBin( ERIC_FHP_Y0+h*ERIC_FHP_YS);
      sprintf(tmp, "HTR %2d", h);
      yaxis->SetBinLabel( b, tmp);
    }
  }



  //--- setup FED/HTR digi plot
  // same geometry as the fed/htr plot
  if( fhdp) {
    
    // set name and title
    snprintf( name, sizeof(name), "FED HTR Digi Overview");
    fhdp->SetName( name);

    fhdp->SetBins( ERIC_FHP_XBINS, 0, ERIC_FHP_XBINS, ERIC_FHP_YBINS, 0, ERIC_FHP_YBINS);
    fhdp->SetStats( kFALSE);
    fhdp->Fill(0.0,0.0);

    // label X axis with FEDs
    TAxis *xaxis = fhdp->GetXaxis();
    for( int c=0; c<32; c += 2) {
      char tmp[20];
      int b1 = xaxis->FindBin( ERIC_FHP_X0+c*ERIC_FHP_XS);
      sprintf( tmp, "   %3d", c+ERIC_MIN_FED);
      xaxis->SetBinLabel( b1, tmp);
    }

    // label Y axis with stuff
    TAxis *yaxis = fhdp->GetYaxis();
    for( unsigned int h=0; h<15; h++) {
      char tmp[20];
      int b = yaxis->FindBin( ERIC_FHP_Y0+h*ERIC_FHP_YS);
      sprintf(tmp, "HTR %2d", h);
      yaxis->SetBinLabel( b, tmp);
    }
  }





  //--- setup by-FED plots
  // book EH1 histograms
  if( nFeds) {
    if( m_debug)
      printf("Setup by-FED plots\n");

    h_dsp = (EH1 **)EH1::my_calloc( nFeds, sizeof(EH1));
    h_trp = (EH1 **)EH1::my_calloc( nFeds, sizeof(EH1));
    h_drp = (EH1 **)EH1::my_calloc( nFeds, sizeof(EH1));

    for(int i=0; i<nFeds; i++) {
      char name[40], title[80];
      if( m_debug)
	printf("FED %d (number %d)\n", i, m_fed_num[i]);

      // HTR Digi plot
      if( h_hdp != NULL) {
	snprintf( name, sizeof(name), "FED%dDigiErrors", m_fed_num[i]);
	h_hdp[i]->SetName( name);
	h_hdp[i]->SetStats( kFALSE);
	h_hdp[i]->SetBins( ERIC_HDP_XBINS, 0, ERIC_HDP_XBINS, ERIC_HDP_YBINS, 0, ERIC_HDP_YBINS);
	h_hdp[i]->Fill(0.0,0.0);
	
	// label X axis with channels
	TAxis *xaxis = h_hdp[i]->GetXaxis();
	for( unsigned int c=0; c<24; c++) {
	  char tmp[20];
	  int b1 = xaxis->FindBin( ERIC_HDP_X0+c*ERIC_HDP_XS);
	  sprintf( tmp, "ch %2d", c);
	  xaxis->SetBinLabel( b1, tmp);
	}

	// lable Y axis with HTRs
	TAxis *yaxis = h_hdp[i]->GetYaxis();
	for( unsigned int h=0; h<15; h++) {
	  char tmp[20];
	  int b = yaxis->FindBin( ERIC_FHP_Y0+h*ERIC_FHP_YS);
	  sprintf(tmp, "HTR %2d", h);
	  yaxis->SetBinLabel( b, tmp);
	}
      }
	  

      // TTS state
      if( h_ttsp != NULL) {
	snprintf( name, sizeof(name), "FED%dTTSstate", m_fed_num[i]);
	h_ttsp[i]->SetName( name);
	h_ttsp[i]->SetStats( kFALSE);
	h_ttsp[i]->SetBins( 50, 0, 100, ERIC_TTS_YBINS, 0, ERIC_TTS_YBINS);
	TAxis *yaxis = h_ttsp[i]->GetYaxis();
	for( unsigned int iy=0; iy<ERIC_TTS_YBINS; iy++) {
	  int b = yaxis->FindBin( iy);
	  yaxis->SetBinLabel( b, DataIntegrity_tts_label[iy]);
	}
	//h_ttsp[i]->SetBit(TH1::kCanRebin);
	h_ttsp[i]->SetCanExtend(TH1::kXaxis);
	if( m_debug)
	  printf("DataIntegrity::DataIntegrity(): set FED %d name=%s title=%s and bins\n",
		 i, name, title);
      }
      
      // Data size
      snprintf( name, sizeof(name), "FED%dDataSize", m_fed_num[i]);
      snprintf( title, sizeof(title), "FED %d Data Payload (32-bit words)", m_fed_num[i]);
      h_dsp[i] = new EH1( name, title, NUM_EVENT_BINS, 0, EVENT_NUM_MAX);

      // Trigger rate
      snprintf( name, sizeof(name), "FED%dTrigRate", m_fed_num[i]);
      snprintf( title, sizeof(title), "FED %d Trigger Time from Run Start (sec)", m_fed_num[i]);
      h_trp[i] = new EH1( name, title, NUM_EVENT_BINS, 0, EVENT_NUM_MAX);

      // Data rate
      snprintf( name, sizeof(name), "FED%dDataRate", m_fed_num[i]);
      snprintf( title, sizeof(title), "FED %d Data Rate(bytes/sec)", m_fed_num[i]);
      h_drp[i] = new EH1( name, title, NUM_EVENT_BINS, 0, EVENT_NUM_MAX);
    }
  }


}

void DataIntegrity::Fill_fedHtrPlot( int fed, FedErrors& fe)
{
  int xb = (fed - ERIC_MIN_FED) * ERIC_FHP_XS + ERIC_FHP_X0;
  for( int h=0; h<15; h++) {
    int yb = h * ERIC_FHP_YS + ERIC_FHP_Y0;
    if( fe.htrMisEvN[h])
      h_fhp->Fill( xb, yb+2, fe.htrMisEvN[h]);
    if( fe.htrMisBcN[h])
      h_fhp->Fill( xb, yb+1, fe.htrMisBcN[h]);
    if( fe.htrMisOrN[h]) {
      h_fhp->Fill( xb, yb, fe.htrMisOrN[h]);
    }
    if( fe.htrLrbCErr[h])
      h_fhp->Fill( xb+1, yb+2, fe.htrLrbCErr[h]);
    if( fe.htrLrbUErr[h])
      h_fhp->Fill( xb+1, yb+1, fe.htrLrbUErr[h]);
    if( fe.htrCRCErr[h])
      h_fhp->Fill( xb+1, yb, fe.htrCRCErr[h]);
  }
}


void DataIntegrity::Fill_fedHtrDigiPlot( int fed, FedErrors& fe)
{
  int xb = (fed - ERIC_MIN_FED) * ERIC_FHP_XS + ERIC_FHP_X0;
  for( int h=0; h<15; h++) {
    int yb = h * ERIC_FHP_YS + ERIC_FHP_Y0;

    // summ htrDigiState[h][ch][s] by HTR
    int ts[QIE_STATUS_MAX];
    memset( ts, 0, sizeof(ts));
    for( int s=0; s<QIE_STATUS_MAX; s++) { // loop over digi states
      for( int ch=0; ch<FEDGET_NUM_CHAN; ch++) { // sum over channels
	ts[s] += fe.htrDigiState[h][ch][s];
      }
    }

    if( !m_suppress_qie_ok) {
      h_fhdp->Fill( xb, yb+2, ts[4]);	// upper left - AOK
      h_fhdp->Fill( xb, yb+1, ts[2]);	// middle left - ZS

    }
    h_fhdp->Fill( xb+1, yb, ts[1]);	// lower right - CapID
    h_fhdp->Fill( xb+1, yb+1, ts[3]);	// middle right - Length
    h_fhdp->Fill( xb+1, yb+2, ts[0]);  // upper right - Err
  }
}





void DataIntegrity::Fill_overViewPlot( int fed_no, FedErrors& fe)
{
  // convert FED number to crate, then to X coordinate for plot
  double c = FedToCrate[ fed_no - ERIC_MIN_FED];
  double x = (int)c * ERIC_OVP_CRATE_WID;
  if( (c - floor(c)) != 0)
    ++x;

  h_ovp->Fill( x, 0.0, fe.nDigiLen);
  h_ovp->Fill( x, 1.0, fe.nHtrCRC);
  h_ovp->Fill( x, 2.0, fe.nFmtConst);
  h_ovp->Fill( x, 3.0, fe.nFmtCRC);
  h_ovp->Fill( x, 4.0, fe.nFmtSize);

  h_ovp->Fill( x, 6.0, fe.nErrFRL);
  h_ovp->Fill( x, 7.0, fe.nErrHTR);
  h_ovp->Fill( x, 8.0, fe.nErrFEE);  

  h_ovp->Fill( x, 10.0, fe.nLostDCC);
  h_ovp->Fill( x, 11.0, fe.nLostLRB);
  h_ovp->Fill( x, 12.0, fe.nLostHTR);

  h_ovp->Fill( x, 14.0, fe.nMisOrN);
  h_ovp->Fill( x, 15.0, fe.nMisBcN);
  h_ovp->Fill( x, 16.0, fe.nMisEvN);
  h_ovp->Fill( x, 17.0, fe.nSpcEvN);

  h_ovp->Fill( x, 19.0, fe.nHtrStatus[1]);
  h_ovp->Fill( x, 20.0, fe.nHtrStatus[0]);

  h_ovp->Fill( x, 22.0, fe.nTtsSyn);
  h_ovp->Fill( x, 23.0, fe.nTtsBsy);
  h_ovp->Fill( x, 24.0, fe.nTtsOfw);
  h_ovp->Fill( x, 25.0, fe.nTtsRdy);
}


void DataIntegrity::Fill_TTSstatePlot( int EvN, int Fed, int Fedstate, FedErrors fe)
{
  if( Fed < ERIC_MIN_FED || Fed >= ERIC_MIN_FED+ERIC_NUM_FED) {
    fprintf( stderr, "DataIntegrity::Fill_TTSstatePlot:  Bad FED ID=%d\n", Fed);
    return;
  }

  int i = m_map_fed[ Fed-ERIC_MIN_FED];
  if( h_ttsp != NULL) {
    h_ttsp[i]->Fill( EvN, map_tts[Fedstate] + ERIC_TTS_TTSbin); // FED TTS state
    for( int k=0; k<15; k++)
      if( fe.nHtrStatus[k])
	h_ttsp[i]->Fill( EvN, k+ERIC_TTS_HTRbin, fe.nHtrStatus[k]);
    if( fe.nMisEvN)
      h_ttsp[i]->Fill( EvN, ERIC_TTS_HTRSyn, fe.nMisEvN);
    if( fe.nMisBcN)
      h_ttsp[i]->Fill( EvN, ERIC_TTS_HTRSyn+1, fe.nMisBcN);      
    if( fe.nMisOrN)
      h_ttsp[i]->Fill( EvN, ERIC_TTS_HTRSyn+2, fe.nMisOrN);      
  }

}


void DataIntegrity::Fill_HTRDigiPlot( int Fed, FedErrors& fe)
{
  if( Fed < ERIC_MIN_FED || Fed >= ERIC_MIN_FED+ERIC_NUM_FED) {
    fprintf( stderr, "DataIntegrity::Fill_HTRDigiPlot:  Bad FED ID=%d\n", Fed);
    return;
  }

  int i = m_map_fed[ Fed-ERIC_MIN_FED];
  if( h_hdp != NULL) {
    for( int h=0; h<15; h++) {
      int yb = h * ERIC_HDP_YS + ERIC_HDP_Y0;
      for( int c=0; c<24; c++) {
	int xb = c * ERIC_HDP_XS + ERIC_HDP_X0;
	if( m_debug) {
	  for( int k=0; k<QIE_STATUS_MAX; k++)
	    if( fe.htrDigiState[h][c][k])
	      printf("Fill_HTRDigiPlot( FED %d HTR %d ch %d state %d )\n",
		     Fed, h, c, k);
	}
	if( !m_suppress_qie_ok) {
	  h_hdp[i]->Fill( xb, yb+2, fe.htrDigiState[h][c][4]);	// upper left
	  h_hdp[i]->Fill( xb, yb+1, fe.htrDigiState[h][c][2]);	// middle left
	}
	h_hdp[i]->Fill( xb+1, yb, fe.htrDigiState[h][c][1]);	// lower right
	h_hdp[i]->Fill( xb+1, yb+1, fe.htrDigiState[h][c][3]);	// middle right
	h_hdp[i]->Fill( xb+1, yb+2, fe.htrDigiState[h][c][0]);  // upper right

      }
    }
  }
    
  
}


void DataIntegrity::Fill_SRPlot( int plotID, int EvN, int Fed, double value)
{
  if( m_debug) {
    printf("DataIntegrity::Fill_SRPlot( %d, %d, %d, %g)\n",
	   plotID, EvN, Fed, value);
  }

  if( Fed < ERIC_MIN_FED || Fed >= ERIC_MIN_FED+ERIC_NUM_FED) {
    fprintf( stderr, "DataIntegrity::Fill_SRPlot:  Bad FED ID=%d\n", Fed);
    return;
  }

  int i = m_map_fed[ Fed-ERIC_MIN_FED];
  if( i < 0) {
    fprintf( stderr, "DataIntegrity::Fill_SRPlot:  Bad FED ID=%d\n", Fed);
    return;
  }

  switch( plotID) {
  case 0:			// data size plot
    if( h_dsp != NULL && h_dsp[i] != NULL) {
      if( m_debug)
	printf("  h_dsp[%d]->Fill( %d, %g)\n", i, EvN, value);
      h_dsp[i]->Fill( EvN, value);
    }
    break;
  case 1:			// Trigger rate plot
    if( h_trp != NULL && h_trp[i] != NULL) {
      if( m_debug)
	printf("  h_trp[%d]->Fill( %d, %g)\n", i, EvN, value);
      h_trp[i]->Fill( EvN, value);
    }
    break;
  case 2:			// data rate plot
    if( h_drp != NULL && h_drp[i] != NULL)
      h_drp[i]->Fill( EvN, value);
    break;
  default:
    ;
  }
}

// Static data members
int DataIntegrity::map_tts[] = { 4, 1, 3, 4,  2, 4, 4, 4,
				  0, 4, 4, 4,  4, 4, 4, 4};


double DataIntegrity::FedToCrate[] = { 4, 4.5, // FED 700, 701
			 0, 0.5, // FED 702, 703
			 1, 1.5, // FED 704, 705
			 5, 5.5, // FED 706, 707
			 11, 11.5, // FED 708, 709
			 15, 15.5, // FED 710, 711
			 17, 17.5, // FED 712, 713
			 14, 14.5, // FED 714, 715
			 10, 10.5, // FED 716, 717
			 2, 2.5, // FED 718, 719
			 9, 9.5, // FED 720, 721
			 12, 12.5, // FED 722, 723
			 3, 3.5, // FED 724, 725
			 7, 7.5, // FED 726, 727
			 6, 6.5, // FED 728, 729
			 13, 13.5 // FED 730, 731
 };


const char* DataIntegrity::PartitionName[] = { "HBHEa", "HBHEb", "HBHEc", "HF", "HO" };

int DataIntegrity::FedToPartition[ERIC_NUM_FED] = { ERIC_PART_HBHEa, ERIC_PART_HBHEa, // FED 700, 701
						    ERIC_PART_HBHEa, ERIC_PART_HBHEa, // FED 702, 703
						    ERIC_PART_HBHEa, ERIC_PART_HBHEa, // FED 704, 705
						    ERIC_PART_HBHEb, ERIC_PART_HBHEb, // FED 706, 707
						    ERIC_PART_HBHEb, ERIC_PART_HBHEb, // FED 708, 709
						    ERIC_PART_HBHEb, ERIC_PART_HBHEb, // FED 710, 711
						    ERIC_PART_HBHEc, ERIC_PART_HBHEc, // FED 712, 713
						    ERIC_PART_HBHEc, ERIC_PART_HBHEc, // FED 714, 715
						    ERIC_PART_HBHEc, ERIC_PART_HBHEc, // FED 716, 717
						    ERIC_PART_HF, ERIC_PART_HF, ERIC_PART_HF, // FED 718, 719, 720
						    ERIC_PART_HF, ERIC_PART_HF, ERIC_PART_HF, // FED 721, 722, 723
						    ERIC_PART_HO, ERIC_PART_HO,	// FED 724, 725
						    ERIC_PART_HO, ERIC_PART_HO,	// FED 726, 727
						    ERIC_PART_HO, ERIC_PART_HO,	// FED 728, 729
						    ERIC_PART_HO, ERIC_PART_HO	// FED 730, 731
};


						    

void DataIntegrity::UpdatePlots()
{
  char title[80];

  // set titles here, since we now know run number
  snprintf( title, sizeof(title), "HCAL Data Integrity Check - Run %d", m_run);
  h_ovp->SetTitle( title);
  h_fhp->SetTitle( title);
  snprintf( title, sizeof(title), "HCAL FED/HTR Digi Errors - Run %d", m_run);
  h_fhdp->SetTitle( title);

  for(int i=0; i<m_nfed; i++) {
    
    if( h_hdp != NULL) {
      snprintf( title, sizeof(title), "FED %d Digi Errors - Run %d", m_fed_num[i], m_run);
      h_hdp[i]->SetTitle( title);
    }

    if( h_ttsp != NULL) {
      snprintf( title, sizeof(title), "Fed %d TTS State - Run %d", m_fed_num[i], m_run);
      h_ttsp[i]->SetTitle( title);
    }

    if( h_dsp) {
      snprintf( title, sizeof(title), "FED %d Data Payload (32-bit words) - Run %d", m_fed_num[i], m_run);
      h_dsp[i]->SetTitle( title);
      h_dsp[i]->SetAxisLabels( "Event No", "32-bit words");
    }

    if( h_trp) {
      //      snprintf( title, sizeof(title), "FED %d Trigger Time from Run Start (seconds) - Run %d",
      snprintf( title, sizeof(title), "FED %d Trigger Time from Run Start (BX) - Run %d",
		m_fed_num[i], m_run);
      h_trp[i]->SetTitle( title);
      h_trp[i]->SetAxisLabels( "Event No", "Seconds");
    }

    if( h_drp) {
      snprintf( title, sizeof(title), "FED %d Data Rate(bytes/sec) - Run %d", m_fed_num[i], m_run);
      h_drp[i]->SetTitle( title);
    }
  }


  // set pointers to empty plots to NULL
  for( int i=0; i<m_nfed; i++) {
    if( h_dsp[i]->GetTotalCount() == 0)
      p_dsp[i] = NULL;
    else
      EH1_TGraph( h_dsp[i], p_dsp[i]);

    if( h_trp[i]->GetTotalCount() == 0)
      p_trp[i] = NULL;
    else
      EH1_TGraph( h_trp[i], p_trp[i]);

    if( h_drp[i]->GetTotalCount() == 0)
      p_drp[i] = NULL;
    else
      EH1_TGraph( h_drp[i], p_drp[i]);
  }
}
