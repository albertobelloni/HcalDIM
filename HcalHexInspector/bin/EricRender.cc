//
// Read an EricDIM root file and render the plots
//

#include <TRint.h>
#include <TROOT.h>

#include "TFile.h"
#include "TKey.h"
#include "TPaletteAxis.h"
#include "TCanvas.h"
#include "TLine.h"
#include "TText.h"
#include "TPostScript.h"
#include "TDirectory.h"
#include "TGraphAsymmErrors.h"
#include "TH2.h"

static const char* psfile = "Eric_Plots.ps";

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

static bool debug = false;

static int level = 0;

void EricRender(char*);
void ProcessDir( TDirectory* d, TCanvas* canv, TPostScript* myps);

int main(int argc, char *argv[])
{
  TRint* theApp = new TRint("EricRender", &argc, argv, 0, 0, 1); // do not show splash screen

  // code starts here
  EricRender(argv[1]);

  // work in command line mode
  theApp->Run();
  delete theApp;

  return 0;

}

void EricRender( char *fname)
{
  TFile* f = TFile::Open( fname);

  gROOT->SetStyle("Plain");
  /*gStyle->SetPalette(1);*/
  /*TGaxis::SetMaxDigits(4);*/

  TCanvas* canv = new TCanvas("canv", "EricDIM Plots", 800, 600);
  canv->SetLogz();

  canv->SetRightMargin( 0.125);

  TPostScript* myps = new TPostScript( psfile, 100112);
  myps->Range(25,20);

  //  myps.Range( 20, 15);

  ProcessDir( f, canv, myps);

  myps->Close();
}

void ProcessDir( TDirectory* d, TCanvas* canv, TPostScript* myps) {

  ++level;

  // Iterate over everything in the directory
  // Two passes, non-directory objects first

  if( debug)
    printf("Pass 1:  non-directory objects\n");

  TIter next(d->GetListOfKeys());
  TKey *key;
  while ((key=(TKey*)next())) {

    const char *cnam = key->GetClassName();
    const char *nam = key->GetName();

    if( debug)
      printf("** Class %s object name %s\n", cnam, nam);

    // not a directory
    if( strcmp( cnam, "TDirectoryFile")) {

      if( !strcmp( cnam, "TGraphAsymmErrors")) {
	TGraphAsymmErrors* g;
	d->GetObject( nam, g);
	if( g) {
	  g->Draw("LA*");
	  canv->Update();
	} else {
	  printf("...Error retrieving %s (a %s) from directory\n",
		 nam, cnam);
	}

      } else if ( !strcmp( cnam, "TH2C")) {

	TH2C* h;
	d->GetObject( nam, h);

	// suppress empty plots only at levels below top
	if( h->GetEntries() > 0 || level == 1) {

	  /*const char* t = h->GetTitle();*/
	  if( h) {
	    // fuss with the color palette to make the text smaller
	    h->Draw("COLZ");
	    canv->Update();
	    /*TList* tl = h->GetListOfFunctions();*/
	    TPaletteAxis *palette = (TPaletteAxis*)h->GetListOfFunctions()->FindObject("palette");
	    palette->SetLabelSize(0.05);
	  } else {
	    if( !h)
	      printf("...Error retrieving %s (a %s) from directory\n",
		     nam, cnam);
	    else
	      printf("..Histogram %s (a %s) is empty\n", nam, cnam);
	  }
	}

      } else if ( !strcmp( cnam, "TH2D")) {

	TH2D* h2d;
	int ne;
	d->GetObject( nam, h2d);
	ne = h2d->GetEntries();

	// suppress empty plots only at levels below top
	if( ne > 0 || level == 1) {

	  if( ne == 0) {
	    printf("Processing empty TH2D \"%s\"... one entry to avoid errors\n", nam);
	    h2d->Fill(0.,0.);
	  }

	  const char* t = h2d->GetTitle();
	  if( debug)
	    printf("Drawing TH2D Name = %s Title = %s\n", nam, t);
	  if( h2d) {

	    h2d->Draw("COLZ");

	    // Attention, Bricolage!  If this is the fed_htr plot, draw
	    // some lines between the cells on it.  This should really
	    // be in the root file somehow
	    if( !strcmp("FED HTR Overview",nam) || !strcmp("FED HTR Digi Overview",nam) ) {
	      TLine a;

	      // draw a box around each grid item
	      for( int ix=0; ix<32; ix++) {
		for( int iy=0; iy<15; iy++) {
		  double x = ERIC_FHP_X0+ix*ERIC_FHP_XS;
		  double y = ERIC_FHP_Y0+iy*ERIC_FHP_YS;
		  a.DrawLine( x, y, x, y+ERIC_FHP_YS-1);
		  a.DrawLine( x, y, x+ERIC_FHP_XS-1, y);
		  a.DrawLine( x+ERIC_FHP_XS-1, y+ERIC_FHP_YS-1, x, y+ERIC_FHP_YS-1);
		  a.DrawLine( x+ERIC_FHP_XS-1, y+ERIC_FHP_YS-1, x+ERIC_FHP_XS-1, y);
		}
	      }

	      // now a little legend in the upper right
	      double x = ERIC_FHP_X0 + 26.0*ERIC_FHP_XS;
	      double y = ERIC_FHP_Y0 + 15.0*ERIC_FHP_YS+1;
	      double sx = ERIC_FHP_XS-1;
	      double sy = ERIC_FHP_YS-1;

	      a.DrawLine( x, y, x, y+2*sy);
	      a.DrawLine( x, y, x+2*sx, y);
	      a.DrawLine( x+2*sx, y+2*sy, x, y+2*sy);
	      a.DrawLine( x+2*sx, y+2*sy, x+2*sx, y);

	      a.DrawLine( x+sx, y, x+sx, y+2*sy);
	      a.DrawLine( x, y+4*sy/3, x+2*sx, y+4*sy/3);
	      a.DrawLine( x, y+2*sy/3, x+2*sx, y+2*sy/3);

	      TText tx;
	      tx.SetTextSize( 0.02);

	      if( !strcmp("FED HTR Overview",nam)) {
		tx.DrawText( x-5 , y, "OrN");
		tx.DrawText( x-5 , y+2, "BcN");
		tx.DrawText( x-5, y+4, "EvN");
		tx.DrawText( x+5, y, "CRC");
		tx.DrawText( x+5, y+2, "UErr");
		tx.DrawText( x+5, y+4, "CErr");
	      }

	      if( !strcmp("FED HTR Digi Overview",nam) ) {
		tx.DrawText( x-5 , y, "-");
		tx.DrawText( x-5 , y+2, " ZS");
		tx.DrawText( x-5, y+4, "AOK");
		tx.DrawText( x+5, y, "CapID");
		tx.DrawText( x+5, y+2, "Length");
		tx.DrawText( x+5, y+4, "Err");
	      }

	      // make a note if empty
	      if( ne == 0)
		tx.DrawText( ERIC_FHP_X0, ERIC_FHP_Y0-3, "No Errors... so plot is empty");

	    }

	    // Attention, Bricolage!  If this is a HTR Digi plot, draw
	    // some lines between the cells on it.  This should really
	    // be in the root file somehow
	    if( strstr( nam, "DigiErrors") != NULL) {
	      TLine a;

	      // draw a box around each grid item
	      for( int ix=0; ix<24; ix++) {
		for( int iy=0; iy<15; iy++) {
		  double x = ERIC_HDP_X0+ix*ERIC_HDP_XS;
		  double y = ERIC_HDP_Y0+iy*ERIC_HDP_YS;
		  a.DrawLine( x, y, x, y+ERIC_HDP_YS-1);
		  a.DrawLine( x, y, x+ERIC_HDP_XS-1, y);
		  a.DrawLine( x+ERIC_HDP_XS-1, y+ERIC_HDP_YS-1, x, y+ERIC_HDP_YS-1);
		  a.DrawLine( x+ERIC_HDP_XS-1, y+ERIC_HDP_YS-1, x+ERIC_HDP_XS-1, y);
		}
	      }

	      // now a little legend in the upper right
	      double x = ERIC_HDP_X0 + 16.0*ERIC_HDP_XS;
	      double y = ERIC_HDP_Y0 + 16.0*ERIC_HDP_YS-2;
	      double sx = ERIC_HDP_XS-1;
	      double sy = ERIC_HDP_YS-1;

	      a.DrawLine( x, y, x, y+2*sy);
	      a.DrawLine( x, y, x+2*sx, y);
	      a.DrawLine( x+2*sx, y+2*sy, x, y+2*sy);
	      a.DrawLine( x+2*sx, y+2*sy, x+2*sx, y);

	      a.DrawLine( x+sx, y, x+sx, y+2*sy);
	      a.DrawLine( x, y+4*sy/3, x+2*sx, y+4*sy/3);
	      a.DrawLine( x, y+2*sy/3, x+2*sx, y+2*sy/3);

	      TText tx;
	      tx.SetTextSize( 0.02);
	      tx.DrawText( x-5 , y, "-");	    // lower left
	      tx.DrawText( x+5, y, "CapID");	    // lower right

	      tx.DrawText( x-5, y+2, " ZS");	    // middle left
	      tx.DrawText( x+5, y+2, "Length");	    // middle right

	      tx.DrawText( x-5, y+4, "AOK");	    // upper left
	      tx.DrawText( x+5, y+4, "Err");	    // upper right
	    }
	    
	    canv->Update();
	  } else {
	    if( !h2d)
	      printf("...Error retrieving %s (a %s) from directory\n",
		     nam, cnam);
	    else
	      printf("..Histogram %s (a %s) is empty\n", nam, cnam);
	  }

	}

      } else if( !strcmp( cnam, "TH1D")) {

	TH1D* h2;
	d->GetObject( nam, h2);

	if( h2) {
	  if( h2->GetEntries()) {

	    if( debug)
	      printf("Look at TH1D %s to set vertical scale...", nam);

	    if( !strcmp( nam, "DataSize") || !strncmp( nam, "PayloadSize", 11) ||
		!strncmp( nam, "SlidingPairSum", 14)) {
	      if( debug) printf("LOGY scale\n");
	      canv->SetLogy(1);
	    } else {
	      if( debug) printf("Lin Y scale\n");
	    }

	    h2->Draw();
	    canv->Update();

	    canv->SetLogy(0);

	  } else {
	    printf("No entries in %s\n", nam);
	  }

	} else {
	  printf("...Error retrieving %s (a %s) from directory\n",
		 nam, cnam);
	}
      } else {
	printf("Don't know how to render a %s\n", cnam);
      }
    }
  }


  // Now the directories
  if( debug)
    printf( "Pass 2: Directories\n");
  next = d->GetListOfKeys();
  TKey *key2;
  while ((key2=(TKey*)next())) {

    const char *cnam = key2->GetClassName();
    const char *nam = key2->GetName();

    // Directory... recurse
    if( !strcmp( cnam, "TDirectoryFile")) {
      if( debug)
	printf("---> Entering directory %s\n", nam);
      TDirectory* sd = d->GetDirectory( nam);
      ProcessDir( sd, canv, myps);
    }
  }

  --level;
}
