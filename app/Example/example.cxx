#include <iostream>
#include <cmath>
#include <utility>
#include <ctime>

// config/storage: from LArCV
#include "Base/PSet.h"
#include "Base/LArCVBaseUtilFunc.h"
#include "Base/DataCoordinator.h"

// larlite data
#include "Base/DataFormatConstants.h"
#include "DataFormat/opflash.h" // example of data product

// larcv data
#include "DataFormat/EventImage2D.h"
#include "DataFormat/EventPixel2D.h"
#include "DataFormat/EventROI.h"

// ROOT includes
#include "TRandom3.h"


int main( int nargs, char** argv ) {
  
  std::cout << "[Example LArCV and LArLite Data Access]" << std::endl;
  
  // parse configuration file
  // -------------------------
  // get file-level configuration parameter set
  larcv::PSet cfg = larcv::CreatePSetFromFile( "config.cfg" );
  // get main PSet
  larcv::PSet exconfig = cfg.get<larcv::PSet>("ExampleConfigurationFile");
  // get name of tree that stores Image2D
  std::string larcv_image_producer = exconfig.get<std::string>("InputLArCVImages");
  
  // Configure Data coordinator
  // --------------------------
  larlitecv::DataCoordinator dataco;

  // you can get example data files from: uboonegpvm0X:/uboone/data/users/tmw/tutorials/larlitecv_example/

  // larlite
  dataco.add_inputfile( "larlite_opreco_0000.root", "larlite" );
  // larcv
  dataco.add_inputfile( "supera_data_0000.root", "larcv" );

  // configure the data coordinator
  dataco.configure( "config.cfg", "StorageManager", "IOManager", "ExampleConfigurationFile" );
  
  // initialize
  dataco.initialize();

  // Output
  // ------
  // Note that output files are specified in config.cfg and get setup by the data coordinator during initialize()


  // Start Event Loop
  int nentries = dataco.get_nentries("larcv");
  nentries = 10; // to shorten the loop

  TRandom3 rand(time(NULL));
  
  for (int ientry=0; ientry<nentries; ientry++) {
    std::cout << "[Entry " << ientry << "]" << std::endl;

    dataco.goto_entry(ientry,"larcv");

    // --------------------------------------------------------
    // example operating and creating larcv Image2D objects

    // get input images (from larcv)
    larcv::EventImage2D* event_imgs    = (larcv::EventImage2D*)dataco.get_larcv_data( larcv::kProductImage2D, larcv_image_producer );
    std::cout << "get data: number of images=" << event_imgs->Image2DArray().size() << std::endl;
    const std::vector<larcv::Image2D>& img_v = event_imgs->Image2DArray();
    std::cout << "size of first image: " 
	      << " rows=" << img_v.at(0).meta().rows()
	      << " cols=" << img_v.at(0).meta().cols() 
	      << std::endl;
    std::cout << " a pixel and its value: " << img_v.at(0).pixel(10,10) << std::endl;
    
    // make a container to hold our new images to output
    std::vector< larcv::Image2D > output_container;

    // Output Event Containers
    // ------------------------
    // We pass in objects we want to write to file into these Event containers.  These get added to the ROOT tree and saved to disk when we call save() later.
    // note that "hits", "rando", and "outroi" will name the TTree that our objects get saved to. You can name these trees as desired.
    // they will show up in the root file as:
    //   TTree* image2d_rando_tree;
    //   TTree* pixel2d_hits_tree;
    //   TTree* partroi_outroi_tree;
    
    // make a pixel2D event container to output hits to file
    larcv::EventPixel2D* event_pixel2d  = (larcv::EventPixel2D*)dataco.get_larcv_data( larcv::kProductPixel2D, "hits" );

    // make the output event container
    larcv::EventImage2D* out_event_imgs = (larcv::EventImage2D*)dataco.get_larcv_data( larcv::kProductImage2D, "rando" ); // rando matches output list
    out_event_imgs->Emplace( std::move( output_container ) );

    // make output container for ROI objects
    larcv::EventROI* out_event_roi      = (larcv::EventROI*)dataco.get_larcv_data( larcv::kProductROI, "outroi" ); 
    

    // Do some stuff with a LArCV Image2D object
    // -------------------------------------------

    // this is an example, showing some operations we can do with the image: how to access and asign values to the pixels in an image.

    // we iterate through the images
    for ( auto &img : event_imgs->Image2DArray() ) {
      const larcv::ImageMeta& meta = img.meta(); // get a constant reference to the Image's meta data (which defines the image size)
      larcv::Image2D newimg( meta ); // make a new image using the old meta. this means, the size and coordinate system is the same.
      for (int irow=0; irow<meta.rows(); irow++) {
	for (int icol=0; icol<meta.cols(); icol++) {
	  float newval = img.pixel(irow,icol)*rand.Uniform(); // generate a random value based on the old image value
	  newimg.set_pixel( irow, icol, newval ); // set the value int he new image
	  if ( img.pixel(irow,icol)>0 && newval/img.pixel(irow,icol)>0.5 ) {
	    // if it's above 0.5, we make a Pixel2D object
	    larcv::Pixel2D hit( icol, irow );
	    hit.Intensity( newval );
	    hit.Width( 1.0 );
	    // store the pixel 2D in its output container
	    event_pixel2d->Emplace( (larcv::PlaneID_t)meta.plane(), std::move(hit) );
	  }
	}
      }
      // put the new image into the image output container
      output_container.emplace_back( std::move(newimg) ); // we use emplace and move so that we avoid making a copy
    }

    // Make an ROI object
    // ------------------

    // create a vector to hold our ROIs
    std::vector<larcv::ROI> outroi_v;
    larcv::ROI anewroi( larcv::kROIBNB ); // arbitrary ROI given a BNB label, ROIType_t. (You can find labels in $LARCV_BASEDIR/core/DataFormat/DataFormatTypes.h)
    outroi_v.emplace_back( std::move(anewroi) );
    // put it into the container
    out_event_roi->Emplace( std::move(outroi_v) );
    

    // --------------------------------------------------------
    // example of using larlite data: we're going to loop through some opflashes

    // this is represnetative of how one loops through data in the file
    // just change the data product enum and the producer name
    //   enum's can be found in $LARLITE_BASEDIR/core/Base/DataFormatTypes.h
    //   producer name is found by the name of the tree in the file which goes: [data product]_[producer name]_tree
    // in this example we are accessing: opflash_opflash_tree
    larlite::event_opflash* ev_opflash = (larlite::event_opflash*)dataco.get_larlite_data( larlite::data::kOpFlash, "opflashSat" );
    
    // all the event_x classes are just a child class of vector<x>, so we loop through the items inside just like one would a vector
    for (size_t iflash=0; iflash<ev_opflash->size(); iflash++) {
      const larlite::opflash& aflash = ev_opflash->at(iflash);
      std::cout << " opflash #" << iflash << ": PE=" << aflash.TotalPE() << std::endl;
    }
    
    // go to tree
    dataco.save_entry();
  }
  
  dataco.finalize();

  return 0;
}
