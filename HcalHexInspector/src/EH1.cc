
#include "HcalDIM/HcalHexInspector/interface/EH1.hh"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

EH1::EH1(char *name, char *title, int nbins, double xlow, double xup)
{
  //  printf("EH1::EH1( '%s', '%s', %d, %f, %f)\n",
  //	 name, title, nbins, xlow, xup);

  m_name = (char *)my_calloc( strlen(name)+1, 1);
  strcpy( m_name, name);
  m_title = (char *)my_calloc( strlen(title)+1, 1);
  strcpy( m_title, title);
  m_total = (double *)my_calloc( nbins+2, sizeof(double));
  m_min = (double *)my_calloc( nbins+2, sizeof(double));
  m_max = (double *)my_calloc( nbins+2, sizeof(double));
  m_count = (long *)my_calloc( nbins+2, sizeof(long));
  m_xlow = xlow;
  m_xup = xup;
  m_nbins = nbins;
  m_kCanRebin = true;
  m_total_count = 0;

  m_debug = false;

  if( m_debug)
    printf("EH1::EH1() done\n");
}

void EH1::SetAxisLabels( const char *x, const char *y)
{
  if( x != NULL) {
    m_xaxis_label = (char *)my_calloc(strlen(x)+1, 1);
    strcpy( m_xaxis_label, x);
  } else
    m_xaxis_label = NULL;

  if( y != NULL) {
    m_yaxis_label = (char *)my_calloc(strlen(y)+1, 1);
    strcpy( m_yaxis_label, y);
  } else
    m_yaxis_label = NULL;

  
}

void EH1::SetTitle( char *title)
{
  m_title = (char *)my_calloc( strlen(title)+1, 1);
  strcpy( m_title, title);
}

void EH1::SetDebug( bool d)
{
  m_debug = d;
}


int EH1::GetBin( double x) 
{
  int rc;

  if( x < m_xlow) {
    rc = 0;
  } else if( x >= m_xup) {
      rc = m_nbins+1;
  } else {
    rc = 1 + (int)(m_nbins * ((x - m_xlow) / (m_xup - m_xlow)));
  }
  if( m_debug)
    printf("EH1::GetBin( %f) = %d\n", x, rc);
  return rc;
}

double EH1::GetBinCenter( int b)
{
  double bs = (m_xup - m_xlow)/(double)m_nbins;
  double x = (b-1)*bs + m_xlow + (bs/2.0);
  if( m_debug)
    printf("EH1::GetBinCenter( %d) = %f\n", b, x);
  return x;
}

double EH1::GetBinMean( int b)
{
  double x;

  if( GetBinCount(b))
    x= m_total[b] / (double)m_count[b];
  else
    x = 0;

  if( m_debug)
    printf("EH1::GetBinMean( %d) = %f\n", b, x);

  return x;
}

void EH1::Fill( double x, double w)
{
  int b = GetBin(x);
  // rebin as required
  if(  b > m_nbins && m_kCanRebin) {
    while( b = GetBin(x), b > m_nbins)
      ReBin( 2);
  }
  if( b > m_nbins)
    FillBin( m_nbins, w);
  else
    FillBin( b, w);

  ++m_total_count;
}

void EH1::FillBin( int b, double w)
{
  if( m_count[b]) {		// bin is not empty
    if( w > m_max[b])
      m_max[b] = w;
    if( w < m_min[b])
      m_min[b] = w;
  } else {
    m_max[b] = w;
    m_min[b] = w;
  }
  m_total[b] += w;
  m_count[b]++;
  if( m_debug)
    printf("Fill bin %d with value %f (max=%f min=%f count=%ld)\n", b, w, m_min[b], m_max[b], m_count[b]);

}

void EH1::ReBin( int n)
{
  if( m_debug)
    printf("EH1::Rebin( %d)\n", n);

  if( n < 2 || n >= m_nbins)	// sanity check
    return;
  int nc = m_nbins / n;		// number of complete cycles
  int np = m_nbins % n;		// number of leftover bins
  if( np)
    fprintf( stderr, "Warning! Re-binning where nbins (%d) modulo rebin factor (%d) <> 0\n",
	     m_nbins, n);
  int src, dst;


  if( nc) {
    // pass 1... merge into nth bins
    if( m_debug)
      printf("Pass 1...\n");
    for( int i=0; i<nc; i++) {
      dst = i * n;		// destination bin
      if( m_debug)
	printf("  dst = %d\n", dst);
      for( int j=1; j<n; j++) {	// source bin
	src = dst + j;
	MergeBin( dst+1, src+1); // merge downwards
      }
    }

    // pass 2... compact
    if( m_debug)
      printf("Pass 2...\n");
    for( int i=1; i<nc; i++) {
      dst = i * n;		// destination bin
      if( m_debug)
	printf("  dst = %d\n", dst);
      CopyBin( i+1, dst+1);
    }

    // pass 3... zero the empty bins
    for( int i=nc; i<m_nbins; i++)
      ClearBin( i+1);

    // finally, reset xup
    m_xup += (m_xup-m_xlow);
  }  
}

void EH1::ClearBin( int b)
{
  m_total[b] = m_min[b] = m_max[b] = m_count[b] = 0;
  
  if( m_debug)
    printf("EH1::ClearBin( %d)\n", b);
}

void EH1::MergeBin( int dst, int src)
{
  m_total[dst] += m_total[src];
  if( m_min[src] < m_min[dst]) m_min[dst] = m_min[src];
  if( m_max[src] > m_max[dst]) m_max[dst] = m_max[src];
  m_count[dst] += m_count[src];
  m_total[src] = m_count[src] = 0;

  if( m_debug)
    printf("EH1::MergeBin( %d, %d)\n", dst, src);
}

void EH1::CopyBin( int dst, int src)
{
  m_total[dst] = m_total[src];
  m_min[dst] = m_min[src];
  m_max[dst] = m_max[src];
  m_count[dst] = m_count[src];

  if( m_debug)
    printf("EH1::CopyBin( %d, %d)\n", dst, src);
}

void EH1::Dump()
{
  printf("Name: %s  Title: %s  nbins: %d  xlow: %f  xup: %f\n",
	 m_name, m_title, m_nbins, m_xlow, m_xup);
  printf(  "Bin   count  minimum   maximum  total\n");
  if( GetBinCount(0))
    printf("UNDER %5ld  %8.2g  %8.2g  %8.2g\n",
	   GetBinCount(0), GetBinMin(0), GetBinMax(0), GetBinTotal(0));
  for( int i=1; i<=m_nbins; i++)
    if( GetBinCount(i))
      printf("%5d %5ld  %8.2g  %8.2g  %8.2g\n",
	     i, GetBinCount(i), GetBinMin(i), GetBinMax(i), GetBinTotal(i));
  if( GetBinCount(m_nbins+1))
    printf(" OVER %5ld  %8.2g  %8.2g  %8.2g\n",
	   GetBinCount(m_nbins+1), GetBinMin(m_nbins+1),
	   GetBinMax(m_nbins+1), GetBinTotal(m_nbins+1));
  
}

void* EH1::my_calloc( size_t n, size_t s)
{
  void* p;

  if( (p = calloc( n, s)) == NULL) {
    fprintf( stderr, "EH1::my_calloc(%d,%d) failed!\n", (int) n, (int) s);
    abort();
  }

  return p;
}
