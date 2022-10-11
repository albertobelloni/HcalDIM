#ifndef _DATA_INTEGRITY_HH
#define _DATA_INTEGRITY_HH

#include "TGraphAsymmErrors.h"
#include "TH2.h"
#include "HcalDIM/HcalHexInspector/interface/EH1.hh"
#include "HcalDIM/HcalHexInspector/interface/FedErrors.hh"

#define ERIC_OVP_CRATES 18
#define ERIC_OVP_CRATE_WID 3
#define ERIC_OVP_XBINS (ERIC_OVP_CRATES*ERIC_OVP_CRATE_WID)

//-- define parameters for HTR/Digi plot
#define ERIC_HDP_X0 4
#define ERIC_HDP_Y0 4
#define ERIC_HDP_XS 3
#define ERIC_HDP_YS 4
#define ERIC_HDP_XBINS (24*ERIC_HDP_XS+ERIC_HDP_X0)
#define ERIC_HDP_YBINS (15*ERIC_HDP_YS+ERIC_HDP_Y0)

// define scale, offset for FHP
#define ERIC_FHP_X0 6
#define ERIC_FHP_Y0 6
#define ERIC_FHP_XS 3
#define ERIC_FHP_YS 4
#define ERIC_FHP_XBINS (32*ERIC_FHP_XS+ERIC_FHP_X0)
#define ERIC_FHP_YBINS (15*ERIC_FHP_YS+ERIC_FHP_Y0)

// define partitions for FedToParititon
#define ERIC_PART_HBHEa 0
#define ERIC_PART_HBHEb 1
#define ERIC_PART_HBHEc 2
#define ERIC_PART_HF 3
#define ERIC_PART_HO 4

#define ERIC_PART_COUNT 5


#define HCAL_NUM_HTR 15

/** \brief Manage data integrity plots for HCAL
 *
 * Setup and fill a set of histograms and plots for HCAL data integrity checking.
 * Expects that actual ROOT TH and TGraph objects are created elsewhere, i.e.
 * in a CMSSW job or stand-alone program.  This class handles setting
 * the labels, bins etc and filling in the data.  The rendering of the plots
 * or storing in a root file is again handled outside this class.
 */

class DataIntegrity {

public:

  /** \brief Register a set of plots
   *
   * \param nFeds number of FEDs to plot (0 for no by-FED plots)
   * \param Feds list of FED numbers (700-731 inclusive)
   * \param overViewPlot 2D histogram for overview plot (NULL to omit plot)
   * \param fedHtrPlot 2D histogram for error plot by FED/HTR
   * \param TTSstatePlots plots of TTS state versus event number
   * \param HTRDigiPlots plots of HTR QIE data state versus channel
   * \param dataSizePlots plots of data size versus event number
   * \param trigRatePlots plots of trigger rate versus event number
   * \param dataRatePlots plots of data rate versus event number
   * \param debug enable debug output
   *
   * The pointers should each point to an array of size nFeds, of
   * pointers to already-created ROOT objects of the specified type.
   * If nFEDs=0, NULL pointers may be supplied.  The number of bins or
   * points need not be set... they will be set in by this method.
   *
   */
  DataIntegrity( int nFeds,
		 int *Feds,
		 TH2D* overViewPlot,
		 TH2D* fedHtrPlot,
		 TH2D* fedHtrDigiPlot,
		 TH2D** TTSstatePlots,
		 TH2D** HTRDigiPlots,
		 TGraphAsymmErrors** dataSizePlots,
		 TGraphAsymmErrors** trigRatePlots,
		 TGraphAsymmErrors** dataRatePlots,
		 bool debug
		 );


  /** \brief Fill FED/HTR overview plot from FedErrors error summary
   *
   * \param fed FED number (700..731)
   * \param fe FedErrors object with some errors recorded
   *
   * This method will fill the errors in the fe object to the 2D histogram
   * for the overview plot.  It may be called once to fill all the errors in a run,
   * or several times (i.e. in a loop over events)
   */
  void Fill_fedHtrPlot( int fed, FedErrors& fe);

  /** \brief Fill FED/HTR digi plot from FedErrors error summary
   *
   * \param fed FED number (700..731)
   * \param fe FedErrors object with some errors recorded
   *
   * This method will fill the errors in the fe object to the 2D histogram
   * for the overview plot.  It may be called once to fill all the errors in a run,
   * or several times (i.e. in a loop over events)
   */
  void Fill_fedHtrDigiPlot( int fed, FedErrors& fe);


  /** \brief Fill overview plot from FedErrors error summary
   *
   * \param fed FED number (700..731)
   * \param fe FedErrors object with some errors recorded
   *
   * This method will fill the errors in the fe object to the 2D histogram
   * for the overview plot.  It may be called once to fill all the errors in a run,
   * or several times (i.e. in a loop over events)
   */
  void Fill_overViewPlot( int fed, FedErrors& fe);

  /** \brief Fill Digi state plot from FedErrors error summary
   *
   * \param fed FED number (700..731)
   * \param fe FedErrors object with some errors recorded
   *
   * This method will fill the digi errors in the fe object to the 2D histogram
   * for the specified FED.  It may be called once to fill all the errors in a run,
   * or several times (i.e. in a loop over events)
   */

  void Fill_HTRDigiPlot( int fed, FedErrors& fe);

  /** \brief Fill TTS state histogram
   *
   * \param EvN event number
   * \param Fed FED number (700..731)
   * \param fe FedErrors instance for this FED with error status
   */
  void Fill_TTSstatePlot( int EvN, int Fed, int Fedstate, FedErrors fe);
  
  /** \brief Fill a size/rate plot
   *
   * \param plotID Which plot to fill
   * - 0 = data size plot
   * - 1 = trigger rate plot
   * - 2 = data rate plot
   */
  void Fill_SRPlot( int plotID, int EvN, int Fed, double value);

  /** \brief Copy data from histograms to plots */
  void UpdatePlots();

  /** \brief Record run number for use in plot titles */
  void SetRunNo( int r) { m_run = r; }

  /** \brief Enable/Disable filling of QIE "AOK" and "ZS" bins */
  void SuppressQIEOK( bool s) { m_suppress_qie_ok = s; }

#define ERIC_MIN_FED 700
#define ERIC_NUM_FED 32

  /** FED-to-Crate translation - is this cast in cement?
   * first element is crate# for FED 700 */
  static double FedToCrate[ERIC_NUM_FED];

  /** FED-to-Partition translation */
  static int FedToPartition[ERIC_NUM_FED];

  /** Map TTS states to histogram bins */
  static int map_tts[16];

// correspond to "ERIC_PART_xxx" symbols in DataIntegrity.hh
  static const char *PartitionName[ERIC_PART_COUNT];


  // Member variables
  int m_map_fed[ERIC_NUM_FED]; ///< Map (FED#-700) to plot# (-1 = not used)
  int m_fed_num[ERIC_NUM_FED]; ///< FED# (700..731) for each plot
  int m_nfed; ///< Number of FEDs listed

  // need to keep track of all the plots too
  TH2D* h_ovp;  ///< pointer to overview (data integrity) plot
  TH2D* h_fhp;  ///< pointer to FED/HTR overview plot
  TH2D* h_fhdp; ///< pointer to FED/HTR digi plot
  TH2D** h_ttsp;  ///< pointer to list of histograms
  TH2D** h_hdp;			///< pointer to list of 2D histograms
  TGraphAsymmErrors** p_dsp;  ///< pointer to list of data size plots
  TGraphAsymmErrors** p_trp;  ///< pointer to list of trigger time plots
  TGraphAsymmErrors** p_drp;  ///< pointer to list of data rate plots

  EH1** h_dsp;			///< pointer to list of EH1 histograms for data size 
  EH1** h_trp;			///< pointer to list of EH1 histograms for trigger time
  EH1** h_drp;			///< pointer to list of EH1 histograms for data rate

  bool m_debug;			///< debug flag

  bool m_suppress_qie_ok;	///< suppress AOK and ZS qie states

  int m_run;			///< run number for plot titles

  // number of bins on X axis in size/rate plots
#define NUM_EVENT_BINS 50
  // event number for upper edge of X axis in size/rate plots
#define EVENT_NUM_MAX 10

};


#endif
