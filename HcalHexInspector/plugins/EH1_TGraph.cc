
#include "HcalDIM/HcalHexInspector/interface/EH1_TGraph.hh"

void EH1_TGraph( EH1* h, TGraphAsymmErrors* m_tg)
{
  Double_t x, y, eyl, eyh;

  int last = h->GetNumBins()-1;

  // find last non-zero bin
  for( int i=h->GetNumBins()-1;
       i >= 0 && h->GetBinCount(i) == 0;
       i--) {
    last = i-2;
  }

  if( last < 1)
    last = 1;

  m_tg->SetName( h->GetName());
  m_tg->SetTitle( h->GetTitle());
  m_tg->Set( last+1);

  for( int i=0; i<last+1; i++) {
    int b = i + 1;

    // deal with empty bins, but overlapping with previous or next bin
    if( h->GetBinCount(b) == 0) {
      if( i == 0)
	b = 2;
      else
	b = i;
    }

    x = h->GetBinCenter(b);
    y = h->GetBinMean(b);
    eyl = h->GetBinMin(b);
    eyh = h->GetBinMax(b);

    m_tg->SetPoint( i, x, y);
    m_tg->SetPointEYlow( i, y-eyl);
    m_tg->SetPointEYhigh( i, eyh-y);
  }

}
