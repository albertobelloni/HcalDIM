#ifndef _EH1_TGRAPH_HH
#define _EH1_TGRAPH_HH

#include "HcalDIM/HcalHexInspector/interface/EH1.hh"
#include "TGraphAsymmErrors.h"

/** \brief Copy data from an <b>EH1</b> histogram to an existing <b>TGraphAsymmErrors</b>
 *
 */
void EH1_TGraph( EH1* h, TGraphAsymmErrors* m_tg);

#endif
