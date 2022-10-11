#ifndef _EH1_HH
#define _EH1_HH

#include <stdlib.h>

/** \brief Support for time-series plots (i.e. trigger rate vs event number)
 * 
 *  Essentially a variation on a 1D histogram, with the following
 *  extensions:
 *  
 *  Keep track of minimum, maxiumum and number of entries filled in
 *  each bin.  Automatic re-bin if X value exceeds current upper edge (xup).
 * 
 *  The histogram is created as a TH1, with number of bins, lower edge (xlow)
 *  and upper edge (xup) specified.  If a value is filled with a bin number
 *  above xlow, all data is re-binned in the lower half of the set
 *  of bins, and the new value booked as usual.
 * 
 *  The data may be plotted as desired, but generally is displayed in
 *  ROOT as a TGraph object, with the min and max values shown as
 *  unequal error bars
 * 
 *  For consistency with ROOT histograms, bins are numbered as
 *  follows:
 * 
 *  - bin = 0;       underflow bin
 *  - bin = 1;       first bin with low-edge xlow INCLUDED
 *  - bin = nbins;   last bin with upper-edge xup EXCLUDED
 *  - bin = nbins+1; overflow bin
 * 
 *  The underflow bin is for any value filled below xlow.  The
 *  overflow bin is not used if kCanRebin is set (default), otherwise
 *  it records entries above xup.
 *
 *  N.B. This class was written for diagnostic plots, and as such no
 *  particular care was taken to ensure maximum arithmetic accuracy.
 */

class EH1 {
 public:
  //! Constructor
  /// \param name histogram name (avoid blanks)
  /// \param title histogram title
  /// \param nbins Number of bins
  /// \param xlow Left edge of lowest bin
  /// \param xup Right edge of highest bin
  EH1( char *name, char *title, int nbins, double xlow, double xup);
  //! Increment the bin corresponding to x by w
  /// \param x value to select bin
  /// \param w value to fill
  void Fill( double x, double w);
  //! Return number of bins
  int GetNumBins() { return m_nbins; }
  //! Return current value of xlow (bottom edge of lowest bin)
  double GetXlow() { return m_xlow; }
  //! Return current value of xup (top edge of highest bin)
  double GetXup() { return m_xup; }
  //! Return pointer to title string
  char *GetTitle() { return m_title; }
  //! Return pointer to name string
  char *GetName() { return m_name; }
  //! Return the bin number corresponding to x
  int GetBin( double x);
  //! Return the X coordinate at the center of the specified bin
  double GetBinCenter( int b);
  //! Return the mean value filled to the bin
  double GetBinMean( int b);
  //! Return total value stored in bin
  double GetBinTotal( int bin) { return m_total[bin]; }
  //! Return number of entries in bin
  long GetBinCount( int bin) { return m_count[bin]; }
  //! Return maximum value filled in bin
  double GetBinMax( int bin) { return m_max[bin]; }
  //! Return minimum value filled in bin
  double GetBinMin( int bin) { return m_min[bin]; }
  //! Return total number of fills
  long GetTotalCount() { return m_total_count; }
  //! Rebin the histogram
  /// \param n number of bins to merge to one
  void ReBin( int n);
  //! Set an option
  /// \param n option to set.  Currently only supports kCanRebin;
  void SetBit( int n);
  //! Clear an option
  /// \param n option to reset.  Currently only supports kCanRebin;
  void ResetBit( int n);
  //! Set debug flag
  void SetDebug( bool d);
  //! Set title
  void SetTitle( char *t);
  //! Set x and y axis labels (NULL to omit one)
  void SetAxisLabels( const char *x, const char *y);
  //! Get X axis label or NULL for none
  char* GetXaxisLabel() { return m_xaxis_label; }
  //! Get Y axis label or NULL for none
  char* GetYaxisLabel() { return m_yaxis_label; }
  //! Dump histo to stdout for debugging (non-zero bins only)
  void Dump();
  static void *my_calloc( size_t n, size_t s); ///< private allocator... abort() on failure
 private:
  //! FIll a particular bin
  /// \param b bin number 0..nbins+1)
  /// \param w value to fill
  void FillBin( int b, double w); ///< Fill a specific bin with a value
  void MergeBin( int dst, int src); ///< Merge src bin to dst 
  void CopyBin( int dst, int src); ///< Copy src bin to dst 
  void ClearBin( int b);	///< Clear the specified bin
  bool m_kCanRebin;		///< Flag: automatic re-bin on overflow
  int m_nbins;			///< Number of bins
  double* m_total;		///< Array of summed bin contents
  double* m_min;		///< Array of minimum bin values
  double* m_max;		///< Array of maximum bin values
  long* m_count;		///< Array of bin count
  double m_xlow;		///< Current lower edge of bottom bin
  double m_xup;	                ///< Current upper edge of top bin
  char* m_name;			///< Name of histogram (used for plotting)
  char* m_title;		///< Title of histogram (used for plotting)
  char* m_xaxis_label;		///< Label for X axis or NULL for none
  char* m_yaxis_label;		///< Label for Y axis or NULL for none 
  bool m_debug;			///< debug flag
  int m_total_count;		///< Total number of entries in all bins
};

#endif
