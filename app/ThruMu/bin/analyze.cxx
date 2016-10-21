#include <iostream>
#include <cmath>
#include <utility>
#include <algorithm>

// config/storage
#include "Base/PSet.h"
#include "Base/LArCVBaseUtilFunc.h"
#include "Base/DataCoordinator.h"

// larlite data
#include "Base/DataFormatConstants.h"
#include "DataFormat/opflash.h"

// larcv data
#include "DataFormat/EventImage2D.h"
#include "DataFormat/EventPixel2D.h"
#include "DataFormat/EventROI.h"
#include "ANN/ANNAlgo.h"
#include "dbscan/DBSCANAlgo.h"

// larelitecv
#include "ThruMu/BoundaryMuonTaggerAlgo.h"
#include "ThruMu/FlashMuonTaggerAlgo.h"
#include "ThruMu/BoundaryEndPt.h"
#include "ThruMu/EmptyChannelAlgo.h"

// ROOT
#include "TFile.h"
#include "TTree.h"


int main( int nargs, char** argv ) {
  
  std::cout << "[ANALYSIS OF BOUNDARY MUON TAGGER]" << std::endl;
    
  // output file
  TFile* out = new TFile("output_analysis.root", "RECREATE" );
  TTree* bmt = new TTree("bmt","boundary muon tagger metrics");
  int current;
  int mode;
  float enu;
  float dwall;
  double pos[3];
  int nendpts[3][7];
  int total_pixel_count[3];
  int nubb_pixel_count[3];
  int muon_pixel_count[3];
  int total_tag_count[3];
  int nubb_tag_count[3];
  int total_diff;
  float total_frac_remain[3];
  int nubb_diff;
  float nubb_frac_remain[3];
  bmt->Branch( "current", &current, "current/I" );
  bmt->Branch( "mode", &mode, "mode/I" );
  bmt->Branch( "dwall", &dwall, "dwall/F" );
  bmt->Branch( "enu", &enu, "enu/F" );
  bmt->Branch( "pos", &pos, "pos[3]/D" );
  bmt->Branch( "nendpts", &nendpts, "nendpts[3][7]/I" );
  bmt->Branch( "total_pixel_count", &total_pixel_count, "total_pixel_count[3]/I" );
  bmt->Branch( "total_tag_count", &total_tag_count, "total_tag_count[3]/I" );
  bmt->Branch( "nubb_pixel_count", &nubb_pixel_count, "nubb_pixel_count[3]/I" );
  bmt->Branch( "nubb_tag_count", &nubb_tag_count, "nubb_tag_count[3]/I" );
  bmt->Branch( "total_diff", &total_diff, "total_diff/I" );
  bmt->Branch( "nubb_diff", &nubb_diff, "nubb_diff/I" );
  bmt->Branch( "total_frac_remain", &total_frac_remain, "total_frac_remain[3]/F" ); 
  bmt->Branch( "nubb_frac_remain", &nubb_frac_remain, "nubb_frac_remain[3]/F" );


  float threshold = 10;
  
  // Configure Data coordinator
  larlitecv::DataCoordinator dataco;

  // larlite
  dataco.add_inputfile( "data/data_samples/v05/spoon/larlite/larlite_mcinfo_0000.root",  "larlite" );
  dataco.add_inputfile( "data/data_samples/v05/spoon/larlite/larlite_wire_0000.root",    "larlite" );
  dataco.add_inputfile( "data/data_samples/v05/spoon/larlite/larlite_opdigit_0000.root", "larlite" );
  dataco.add_inputfile( "data/data_samples/v05/spoon/larlite/larlite_opreco_0000.root",  "larlite" );

  // larcv
  dataco.add_inputfile( "fullrun_2.root", "larcv" );

  // configure
  dataco.configure( "analyze.cfg", "StorageManager", "IOManager", "Analysis" );
  
  // initialize
  dataco.initialize();


  // Start Event Loop
  int nentries = dataco.get_nentries("larcv");

  std::cout << "NENTRIES IN BMT FILE: " << nentries << std::endl;
  
  for (int ientry=0; ientry<nentries; ientry++) {
    
    dataco.goto_entry(ientry,"larcv");

    // get ROI for interaction information
    larcv::EventROI* event_rois = (larcv::EventROI*)dataco.get_larcv_data( larcv::kProductROI, "tpc" );
    if ( event_rois->ROIArray().size()==0 ) {
      std::cout << "No ROI?" << std::endl;
      continue;
    }

    // gather truth

    // interaction mode
    const larcv::ROI& primary = event_rois->ROIArray().at(0);
    current = primary.NuCurrentType();
    mode    = primary.NuInteractionType();
    enu     = primary.EnergyInit();
    std::cout << "Event " << ientry << std::endl;
    std::cout << "  current=" << current << " mode=" << mode << " enu=" << enu << std::endl;

    // distance from wall
    pos[0] = primary.X();
    pos[1] = primary.Y();
    pos[2] = primary.Y();
    double dwall_v[3] = {0.0};
    dwall_v[0] = std::min( pos[0]-(-10.0), 250.0-pos[0] );
    dwall_v[1] = std::min( pos[1]-(-125.0), 125.0-pos[1] );
    dwall_v[2] = std::min( pos[2]-(-10.0), 1100.0-pos[2] );
    
    dwall = 1e9;
    for (int i=0; i<3; i++) {
      if ( dwall>dwall_v[i] )
	dwall = (float)dwall_v[i];
    }
    std::cout << "  pos=(" << pos[0] << "," << pos[1] << "," << pos[2] << ")" << std::endl;
    std::cout << "  dwall=" << dwall << std::endl;

    // analysis 1
    // (1) count number of pixels above threshold for whole image
    // (2) count number of pixels above threshold inside neutrino BB
    // (3) count number of pixels above threshold inside muon BB
    // (4) subtract number of pixels that have been tagged

    for (int p=0; p<3; p++) {
      total_pixel_count[p] = 0;
      total_tag_count[p] = 0;
      nubb_pixel_count[p] = 0;
      nubb_tag_count[p] = 0;
      muon_pixel_count[p] = 0; // set to -1 for non-CC events
    }

    // count end points
    larcv::EventPixel2D* event_top     = (larcv::EventPixel2D*)dataco.get_larcv_data( larcv::kProductPixel2D, "topspacepts" );
    larcv::EventPixel2D* event_bot     = (larcv::EventPixel2D*)dataco.get_larcv_data( larcv::kProductPixel2D, "botspacepts" );
    larcv::EventPixel2D* event_up      = (larcv::EventPixel2D*)dataco.get_larcv_data( larcv::kProductPixel2D, "upspacepts" );
    larcv::EventPixel2D* event_down    = (larcv::EventPixel2D*)dataco.get_larcv_data( larcv::kProductPixel2D, "downspacepts" );
    larcv::EventPixel2D* event_anode   = (larcv::EventPixel2D*)dataco.get_larcv_data( larcv::kProductPixel2D, "anodepts" );
    larcv::EventPixel2D* event_cathode = (larcv::EventPixel2D*)dataco.get_larcv_data( larcv::kProductPixel2D, "cathodepts" );
    larcv::EventPixel2D* event_imgends = (larcv::EventPixel2D*)dataco.get_larcv_data( larcv::kProductPixel2D, "imgendpts" );
    larcv::EventPixel2D* endpts[7] = { event_top, event_bot, event_up, event_down, event_anode, event_cathode, event_imgends };
    for (int p=0; p<3; p++) {
      for (int b=0; b<7; b++) {
	nendpts[p][b] = endpts[b]->Pixel2DArray( p ).size();
      }
    }

    // count pixels
    larcv::EventImage2D* event_imgs    = (larcv::EventImage2D*)dataco.get_larcv_data( larcv::kProductImage2D, "tpc" );
    larcv::EventImage2D* event_marked  = (larcv::EventImage2D*)dataco.get_larcv_data( larcv::kProductImage2D, "trackcluster" );
    const std::vector< larcv::Image2D >& tpc_imgs    = event_imgs->Image2DArray();
    const std::vector< larcv::Image2D >& thrumu_imgs = event_marked->Image2DArray();

    for (int p=0; p<3; p++) {
      
      const larcv::Image2D& img    = tpc_imgs.at(p);
      const larcv::Image2D& thrumu = thrumu_imgs.at(p);
      const larcv::ImageMeta& meta = img.meta();
      
      const larcv::ImageMeta& nu_bbox = primary.BB(p);

      for (int r=0; r<meta.rows(); r++) {
	for (int c=0; c<meta.cols(); c++) {

	  float orig_val = img.pixel(r,c);
	  float tag_val  = thrumu.pixel(r,c);

	  if ( orig_val>threshold ) {
	    total_pixel_count[p]++;
	    if ( tag_val>10 ) total_tag_count[p]++;
	    float x = meta.pos_x( c );
	    float y = meta.pos_y( r );
	    if ( nu_bbox.min_y() <= y && y <= nu_bbox.max_y() 
		 && nu_bbox.min_x() <= x && x <= nu_bbox.max_x() ) {
	      nubb_pixel_count[p]++;
	      if ( tag_val>10 ) nubb_tag_count[p]++;
	    }
	  }
	}
      }

      total_diff = total_pixel_count[p]-total_tag_count[p];
      total_frac_remain[p] = float(total_diff)/float(total_pixel_count[p]);
      std::cout << "Total pixels: total(" << total_pixel_count[p] << ") - tagged(" << total_tag_count[p] << ") = " << total_diff << ", frac=" << total_frac_remain << std::endl;

      nubb_diff = nubb_pixel_count[p]-nubb_tag_count[p];
      nubb_frac_remain[p] = float(nubb_diff)/float(nubb_pixel_count[p]);
      std::cout << "Nu ROI pixels: nubb(" << nubb_pixel_count[p] << ") - tagged(" << nubb_tag_count[p] << ") = " << nubb_diff << ", frac=" << nubb_frac_remain << std::endl;

    }//end of loop over planes
    bmt->Fill();
  }
  
  out->cd();
  bmt->Write();
  out->Close();

  dataco.close();
  
  return 0;
}