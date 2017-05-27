#include "CosmicTagger.h"

#include <iostream>
#include <exception>
#include <sstream>

// larlite

// larcv
#include "Base/LArCVBaseUtilFunc.h"
#include "DataFormat/EventImage2D.h"
#include "DataFormat/EventPixel2D.h"

// larlitecv
#include "GapChs/EmptyChannelAlgo.h"
#include "UnipolarHack/UnipolarHackAlgo.h"
#include "TaggerCROIAlgo.h"


namespace larlitecv {

  CosmicTagger::CosmicTagger()
    : m_toplevel_pset("toplevel")
    , m_pset("TaggerCROI")
    , m_cfg_file("")
    , m_emptyalgo(NULL)
    , m_unihackalgo(NULL)
    , m_taggercroialgo(NULL)
  {
    // private default constructor
  }
  
  CosmicTagger::CosmicTagger( std::string tagger_cfg )
    : m_toplevel_pset("toplevel")
    , m_pset("TaggerCROI")
    , m_cfg_file(tagger_cfg)
    , m_emptyalgo(NULL)
    , m_unihackalgo(NULL)
    , m_taggercroialgo(NULL)
  {
    setConfigFile( tagger_cfg );

    // Standard method using the config file

    // input
    m_dataco_input.set_filelist( m_pset.get<std::string>("LArCVInputFilelist"), "larcv" );
    m_dataco_input.set_filelist( m_pset.get<std::string>("LArLiteInputFilelist"), "larlite" );
    m_dataco_input.configure( m_cfg_file, "StorageManager", "IOManager", "TaggerCROI" );
    m_dataco_input.initialize();

    // output
    m_dataco_output.configure( m_cfg_file, "StorageManagerOut", "IOManagerOut", "TaggerCROI" );
    m_dataco_output.initialize();

    // num entries
    m_nentries = m_dataco_input.get_nentries("larcv");
    std::cout << "NUMBER OF ENTRIES: LARCV=" << m_nentries << " LARLITE=" << m_dataco_input.get_nentries("larlite") << std::endl;    
  }

  CosmicTagger::CosmicTagger( std::string tagger_cfg, std::string larcv_list, std::string larlite_list )
    : m_toplevel_pset("toplevel")
    , m_pset("TaggerCROI")
    , m_cfg_file(tagger_cfg)
    , m_emptyalgo(NULL)
    , m_unihackalgo(NULL)
    , m_taggercroialgo(NULL)      
  {
    setConfigFile( tagger_cfg );

    // Standard method using the config file

    // input
    m_dataco_input.set_filelist( larcv_list, "larcv" );
    m_dataco_input.set_filelist( larlite_list, "larlite" );
    m_dataco_input.configure( m_cfg_file, "StorageManager", "IOManager", "TaggerCROI" );
    m_dataco_input.initialize();

    // output
    m_dataco_output.configure( m_cfg_file, "StorageManagerOut", "IOManagerOut", "TaggerCROI" );
    m_dataco_output.initialize();

    // num entries
    m_nentries = m_dataco_input.get_nentries("larcv");
    std::cout << "NUMBER OF ENTRIES: LARCV=" << m_nentries << " LARLITE=" << m_dataco_input.get_nentries("larlite") << std::endl;    
  }

  CosmicTagger::CosmicTagger( std::string tagger_cfg, const std::vector<std::string>& larcv_filepaths, const std::vector<std::string>& larlite_filepaths )
    : m_toplevel_pset("toplevel")
    , m_pset("TaggerCROI")
    , m_cfg_file(tagger_cfg)
    , m_emptyalgo(NULL)
    , m_unihackalgo(NULL)
    , m_taggercroialgo(NULL)      
  {
    setConfigFile( tagger_cfg );

    // Standard method using the config file

    // input
    m_dataco_input.configure( m_cfg_file, "StorageManager", "IOManager", "TaggerCROI" );
    for ( auto const& fpath : larcv_filepaths ) {
      m_dataco_input.add_inputfile( fpath, "larcv" );
    }
    for ( auto const& fpath : larlite_filepaths ) {
      m_dataco_input.add_inputfile( fpath, "larlite" );
    }
    m_dataco_input.initialize();

    // output
    m_dataco_output.configure( m_cfg_file, "StorageManagerOut", "IOManagerOut", "TaggerCROI" );
    m_dataco_output.initialize();

    // num entries
    m_nentries = m_dataco_input.get_nentries("larcv");
    std::cout << "NUMBER OF ENTRIES: LARCV=" << m_nentries << " LARLITE=" << m_dataco_input.get_nentries("larlite") << std::endl;    
  }
  
  CosmicTagger::~CosmicTagger() {
    if ( m_emptyalgo!=NULL )
      delete m_emptyalgo;
    if ( m_unihackalgo!=NULL )
      delete m_unihackalgo;
    if ( m_taggercroialgo!=NULL )
      delete m_taggercroialgo;    
  }

  void CosmicTagger::setConfigFile( std::string cfgfile ) {
    m_cfg_file = cfgfile;
    m_toplevel_pset = larcv::CreatePSetFromFile( cfgfile );
    try {
      m_pset = m_toplevel_pset.get<larcv::PSet>("TaggerCROI");
    }
    catch (...) {
      throw std::runtime_error("Top Level PSet must be named \"TaggerCROI\"");
    }

    m_tagger_cfg = TaggerCROIAlgoConfig::makeConfigFromFile( m_cfg_file );
    setRunParameters();
  }

  std::string CosmicTagger::printState() {
    std::stringstream ss;
    ss << "CosmicTagger State: ";
    ss << " Config'd ";
    if ( m_state.configured )
      ss << "[true]";
    else
      ss << "[false]";
    
    ss << " Input ";
    if ( m_state.input_ready )
      ss << "[true]";
    else
      ss << "[false]";
    
    ss << " ThruMu ";
    if ( m_state.thrumu_run )
      ss << "[true]";
    else
      ss << "[false]";

    ss << " StopMu ";
    if ( m_state.stopmu_run )
      ss << "[true]";
    else
      ss << "[false]";

    ss << " Untagged ";
    if ( m_state.untagged_run )
      ss << "[true]";
    else
      ss << "[false]";

    ss << " CROI ";
    if ( m_state.croi_run )
      ss << "[true]";
    else
      ss << "[false]";
	
    return ss.str();
  }
  
  void CosmicTagger::setRunParameters() {
    m_larcv_image_producer    = m_pset.get<std::string>("LArCVImageProducer");
    m_larcv_chstatus_producer = m_pset.get<std::string>("LArCVChStatusProducer");  
    m_DeJebWires              = m_pset.get<bool>("DeJebWires");
    m_jebwiresfactor          = m_pset.get<float>("JebWiresFactor");
    m_emptych_thresh          = m_pset.get< std::vector<float> >("EmptyChannelThreshold");
    m_chstatus_datatype       = m_pset.get<std::string>("ChStatusDataType");
    m_opflash_producers       = m_pset.get< std::vector<std::string> >( "OpflashProducers" );
    m_RunThruMu               = m_pset.get<bool>("RunThruMu");
    m_RunStopMu               = m_pset.get<bool>("RunStopMu");
    m_RunCROI                 = m_pset.get<bool>("RunCROI");
    m_save_thrumu_space       = m_pset.get<bool>("SaveThruMuSpace", false);
    m_save_stopmu_space       = m_pset.get<bool>("SaveStopMuSpace", false);
    m_save_croi_space         = m_pset.get<bool>("SaveCROISpace",   false);
    m_save_mc                 = m_pset.get<bool>("SaveMC",false);
    m_skip_empty_events       = m_pset.get<bool>("SkipEmptyEvents",false);
    m_apply_unipolar_hack     = m_pset.get<bool>("ApplyUnipolarHack",false);
    configure_algos();
    m_state.configured = true;      
  }
  
  void CosmicTagger::configure_algos() {
    if ( m_emptyalgo!=NULL )
      delete m_emptyalgo;
    if ( m_unihackalgo!=NULL )
      delete m_unihackalgo;
    if ( m_taggercroialgo )
      delete m_taggercroialgo;
    m_emptyalgo   = new larlitecv::EmptyChannelAlgo;
    m_unihackalgo = new larlitecv::UnipolarHackAlgo;
    m_taggercroialgo = new larlitecv::TaggerCROIAlgo( m_tagger_cfg );
  }

  

  void CosmicTagger::runOneEvent( int ientry ) {
    setEntry( ientry );
    bool ok = processInputImages();
    if ( !ok )
      return;
  /*
    // -------------------------------------------------------------------------------------------//
    // RUN ALGOS
    std::cout << "[ RUN ALGOS ]" << std::endl;

    if ( m_RunThruMu ) {
      larlitecv::ThruMuPayload thrumu_data = tagger_algo.runThruMu( m_input_data );
      if ( m_save_thrumu_space )
        thrumu_data.saveSpace();

      if ( RunStopMu ) {
        larlitecv::StopMuPayload stopmu_data = tagger_algo.runStopMu( input_data, thrumu_data );
        // we skip stopmu. we mimic instead.
        // larlitecv::StopMuPayload stopmu_data;
        // // make empty tagged pixel images
        // for ( auto const& img : input_data.img_v ) {
        //   larcv::Image2D stopmu_fake( img.meta() );
        //   stopmu_fake.paint(0);
        //   stopmu_data.stopmu_v.emplace_back( std::move(stopmu_fake) );
        // }

        if ( save_stopmu_space )
          stopmu_data.saveSpace();

        if ( RunCROI ) {
          larlitecv::CROIPayload croi_data = tagger_algo.runCROISelection( input_data, thrumu_data, stopmu_data );
          if ( save_croi_space )
            croi_data.saveSpace();
          WriteCROIPayload( croi_data, input_data, tagger_cfg, dataco_out );
        }
        WriteStopMuPayload( stopmu_data, input_data, tagger_cfg, dataco_out );
      }//end of if stopmu

      WriteThruMuPayload( thrumu_data, input_data, tagger_cfg, dataco_out );
    }

    // -------------------------------------------------------------------------------------------//
    // SAVE DATA

    WriteInputPayload( input_data, tagger_cfg, dataco_out );

    // SAVE MC DATA, IF SPECIFIED, TRANSFER FROM INPUT TO OUTPUT
    if ( save_mc && pset.get<larcv::PSet>("MCWriteConfig").get<bool>("WriteSegment") ) {
      std::cout << "WRITE SEGMENTATION IMAGE INFORMATION" << std::endl;
      larcv::PSet mcwritecfg = pset.get<larcv::PSet>("MCWriteConfig");
      larcv::EventImage2D* ev_segment  = (larcv::EventImage2D*)dataco.get_larcv_data(     larcv::kProductImage2D, mcwritecfg.get<std::string>("SegmentProducer") );
      larcv::EventImage2D* out_segment = (larcv::EventImage2D*)dataco_out.get_larcv_data( larcv::kProductImage2D, mcwritecfg.get<std::string>("SegmentProducer") );
      for ( auto const& img : ev_segment->Image2DArray() ) out_segment->Append( img );
    }

    if ( save_mc && pset.get<larcv::PSet>("MCWriteConfig").get<bool>("WriteTrackShower") ) {
      std::cout << "WRITE MC TRACK SHOWER INFORMATION" << std::endl;
      larcv::PSet mcwritecfg = pset.get<larcv::PSet>("MCWriteConfig");
      larlite::event_mctrack* event_mctrack = (larlite::event_mctrack*)dataco.get_larlite_data(     larlite::data::kMCTrack, mcwritecfg.get<std::string>("MCTrackShowerProducer") );
      larlite::event_mctrack* evout_mctrack = (larlite::event_mctrack*)dataco_out.get_larlite_data( larlite::data::kMCTrack, mcwritecfg.get<std::string>("MCTrackShowerProducer") );
      for ( auto const& track : *event_mctrack ) evout_mctrack->push_back( track  );

      larlite::event_mcshower* event_mcshower = (larlite::event_mcshower*)dataco.get_larlite_data(     larlite::data::kMCShower, mcwritecfg.get<std::string>("MCTrackShowerProducer") );
      larlite::event_mcshower* evout_mcshower = (larlite::event_mcshower*)dataco_out.get_larlite_data( larlite::data::kMCShower, mcwritecfg.get<std::string>("MCTrackShowerProducer") );
      for ( auto const& shower : *event_mcshower ) evout_mcshower->push_back( shower  );
    }

    if ( save_mc && pset.get<larcv::PSet>("MCWriteConfig").get<bool>("WriteMCROI") ) {
      std::cout << "WRITE MC TRACK SHOWER INFORMATION" << std::endl;
      larcv::PSet mcwritecfg = pset.get<larcv::PSet>("MCWriteConfig");
      larcv::EventROI* event_roi = (larcv::EventROI*)dataco.get_larcv_data(     larcv::kProductROI, mcwritecfg.get<std::string>("MCROIProducer") );
      larcv::EventROI* evout_roi = (larcv::EventROI*)dataco_out.get_larcv_data( larcv::kProductROI, mcwritecfg.get<std::string>("MCROIProducer") );
      evout_roi->Set( event_roi->ROIArray() );
    }
    
    dataco_out.save_entry();

    ann::ANNAlgo::cleanup();
  */
  }
 

  bool CosmicTagger::processInputImages() {

    if ( !m_state.configured ) {
      std::cout << "Invalid state to run processInputImages: " << printState() << std::endl;
      return false;
    }
    
    // set the state: we reset all downstream states
    m_state.input_ready = m_state.thrumu_run = m_state.stopmu_run = m_state.untagged_run = m_state.croi_run = false;

    // -----------------------------------------------------------------------------
    // Load the data
    int run, subrun, event;
    try {
      m_dataco_input.goto_entry( m_entry, "larcv" );

      // set the id's
      m_dataco_input.get_id(run,subrun,event);
      m_dataco_output.set_id(run,subrun,event);
    }
    catch (const std::exception& e) {
      std::cerr << "Error reading the input data: " << e.what() << std::endl;
      return false;
    }
      
    std::cout << "=====================================================" << std::endl;
    std::cout << "--------------------" << std::endl;
    std::cout << " Entry " << m_entry << " : " << run << " " << subrun << " " << event << std::endl;
    std::cout << "--------------------" << std::endl;
    std::cout << "=====================================================" << std::endl;
    
    
    // ------------------------------------------------------------------------------------------//
    // PREPARE THE INPUT

    m_input_data.clear();
    m_input_data.run = run;
    m_input_data.subrun = subrun;
    m_input_data.event  = event;
    m_input_data.entry  = m_entry;

    // get images (from larcv)
    larcv::EventImage2D* event_imgs = NULL;
    try {
      event_imgs    = (larcv::EventImage2D*)m_dataco_input.get_larcv_data( larcv::kProductImage2D, m_larcv_image_producer );
      if ( event_imgs->Image2DArray().size()==0) {
	if ( !m_skip_empty_events )
	  throw std::runtime_error("Number of images=0. LArbys.");
	else {
	  std::cout << "Skipping Empty Events." << std::endl;
	  return false;
	}
      }
      else if ( event_imgs->Image2DArray().size()!=3 ) {
	throw std::runtime_error("Number of Images!=3. Weird.");
      }
    }
    catch (const std::exception &exc) {
      std::cerr << "Error retrieving TPC Images: " << exc.what() << std::endl;
      return false;
    }

    // ------------------------------------------------------------------------------------------//
    // MODIFY IMAGES
    m_input_data.img_v.clear();
    event_imgs->Move( m_input_data.img_v );
    try {
      
      if ( m_DeJebWires ) {
	for ( auto &img : m_input_data.img_v ) {
	  const larcv::ImageMeta& meta = img.meta();
	  for ( int col=0; col<(int)meta.cols(); col++) {
	    for (int row=0; row<(int)meta.rows(); row++) {
	      float val = img.pixel( row, col );
	      if (meta.plane()==0 ) {
		if ( (col>=2016 && col<=2111) || (col>=2176 && 2212) ) {
		  val *= m_jebwiresfactor;
		}
	      }
	      img.set_pixel(row,col,val);
	    }
	  }
	}
      }
    }
    catch (const std::exception& e ) {
      std::cerr << "Error Dejebbing wires: " << e.what() << std::endl;
      return false;
    }

    try {
      if ( m_apply_unipolar_hack ) {
	std::vector<int> applyhack(3,0);
	applyhack[1] = 1;
	std::vector<float> hackthresh(3,-10.0);
	m_input_data.img_v = m_unihackalgo->hackUnipolarArtifact( m_input_data.img_v, applyhack, hackthresh );
      }
      if ( m_input_data.img_v.size()!=3 )
	throw std::runtime_error("Number of Images incorrect.");
    }
    catch ( const std::exception& e ) {
      std::cerr << "Error making unipolar artifact: " << e.what() << std::endl;
      return false;
    }

    // ------------------------------------------------------------------------------------------//
    // LABEL EMPTY CHANNELS

    std::vector< larcv::Image2D > emptyimgs;
    try {
      for ( auto const &img : event_imgs->Image2DArray() ) {
	int p = img.meta().plane();
	larcv::Image2D emptyimg = m_emptyalgo->labelEmptyChannels( m_emptych_thresh.at(p), img );
	emptyimgs.emplace_back( emptyimg );
      }
    }
    catch ( const std::exception& e ) {
      std::cerr << "Error making empty channels: " << e.what() << std::endl;
    }

    // ------------------------------------------------------------------------------------------//
    // LABEL BAD CHANNELS

    m_input_data.badch_v.clear();
    try { 
      if ( m_chstatus_datatype=="LARLITE" ) {
	larlite::event_chstatus* ev_status = (larlite::event_chstatus*)m_dataco_input.get_larlite_data( larlite::data::kChStatus, "chstatus" );
	m_input_data.badch_v = m_emptyalgo->makeBadChImage( 4, 3, 2400, 6048, 3456, 6, 1, *ev_status );
	ev_status->clear(); // clear, we copied the info
    }
      else if ( m_chstatus_datatype=="LARCV" ) {
	larcv::EventChStatus* ev_status = (larcv::EventChStatus*)m_dataco_input.get_larcv_data( larcv::kProductChStatus, m_larcv_chstatus_producer );
	m_input_data.badch_v = m_emptyalgo->makeBadChImage( 4, 3, 2400, 6048, 3456, 6, 1, *ev_status );
	ev_status->clear(); // clear, we copied the info
      }
      else if ( m_chstatus_datatype=="NONE" ) {
	for ( auto const& img : m_input_data.img_v ) {
	  larcv::Image2D badch( img.meta() );
	  badch.paint(0.0);
	  m_input_data.badch_v.emplace_back( std::move(badch) );
	}
      }
      else {
	throw std::runtime_error("ERROR: ChStatusDataType must be either LARCV or LARLITE");
      }
      
      if ( m_input_data.badch_v.size()!=3 ) {
	throw std::runtime_error("Number of Bad Channels not correct.");
      }
    }
    catch ( const std::exception& e ) {
      std::cerr << "Error making badch image: " << e.what() << std::endl;
    }

    // ------------------------------------------------------------------------------------------//
    // MAKE GAP CHANNEL IMAGE

    int maxgap = 200; // this seems bad to have this parameter here and not in a config file
    std::vector< larcv::Image2D> gapchimgs_v;
    try {
      gapchimgs_v = m_emptyalgo->findMissingBadChs( m_input_data.img_v, m_input_data.badch_v, 5, maxgap );
      // combine with badchs
      for ( size_t p=0; p<gapchimgs_v.size(); p++ ) {
	larcv::Image2D& gapchimg = gapchimgs_v[p];
	gapchimg += m_input_data.badch_v[p];
      }
      
      // Set BadCh in input data
      for ( size_t p=0; p<gapchimgs_v.size(); p++ ) {
      m_input_data.gapch_v.emplace_back( std::move(gapchimgs_v.at(p)) );
      }
      if ( m_input_data.gapch_v.size()!=3 )
    	throw std::runtime_error("Number of Gap Channels not correct.");
    }
    catch ( const std::exception& e ) {
      std::cerr << "Error making gapch images: " << e.what() << std::endl;
      return false;
    }

    // -------------------------------------------------------------------------------------------//
    // COLLECT FLASH INFORMATION

    try {
      for ( auto &flashproducer : m_opflash_producers ) {
	larlite::event_opflash* opdata = (larlite::event_opflash*)m_dataco_input.get_larlite_data(larlite::data::kOpFlash, flashproducer );
	std::cout << "search for flash hits from " << flashproducer << ": " << opdata->size() << " flashes" << std::endl;
	m_input_data.opflashes_v.push_back( opdata );
      }
    }
    catch ( const std::exception& e ) {
      std::cerr << "Error retrieving flash information: " << e.what() << std::endl;
      return false;
    }

    // set the state. the data is ready
    m_state.input_ready = true;
    return true;
  }

  bool CosmicTagger::findThruMu() {
    // check pre-reqs
    if ( !m_state.configured || !m_state.input_ready ) {
      std::cout << "Invalid state to run findThruMu: " << printState() << std::endl;
      return false;
    }
    
    // invalidate downstream
    m_state.thrumu_run = m_state.stopmu_run = m_state.untagged_run = m_state.croi_run = false;

    larlitecv::ThruMuPayload thrumu_data;
    try {
      thrumu_data = m_taggercroialgo->runThruMu( m_input_data );
    }
    catch ( const std::exception& e ) {
      std::cerr << "Error Running ThruMu: " << e.what() << std::endl;
      return false;
    }

    std::swap( m_thrumu_data, thrumu_data );
    thrumu_data.clear();

    // set stte
    m_state.thrumu_run = true;
    return true;
  }

  bool CosmicTagger::findStopMu() {

    if ( !m_state.configured || !m_state.input_ready || !m_state.thrumu_run ) {
      std::cout << "Invalid state to run StopMu: " << printState() << std::endl;
      return false;
    }

    // reset downstream
    m_state.stopmu_run = m_state.untagged_run = m_state.croi_run = false;
    
    larlitecv::StopMuPayload stopmu_data;
    try {
      stopmu_data = m_taggercroialgo->runStopMu( m_input_data, m_thrumu_data );
    }
    catch ( const std::exception& e ) {
      std::cerr << "Error finding StopMu: " << e.what() << std::endl;
      return false;
    }
    
    if ( m_save_stopmu_space )
      stopmu_data.saveSpace();

    std::swap( m_stopmu_data, stopmu_data );

    m_state.stopmu_run = true;
    m_state.untagged_run = true; // no untagged stage yet
    
    return true;
  }

  bool CosmicTagger::findCROI() {

    if ( !m_state.configured || !m_state.input_ready || !m_state.thrumu_run || !m_state.stopmu_run ) {
      std::cout << "Invalid state to run Untagged/CROI step: " << printState() << std::endl;
      return false;
    }

    m_croi_data_v.clear();
    
    try {
      larlitecv::CROIPayload croi_data = m_taggercroialgo->runCROISelection( m_input_data, m_thrumu_data, m_stopmu_data );
      m_croi_data_v.emplace_back( std::move(croi_data) );
    }
    catch ( const std::exception& e ) {
      std::cerr << "Error finding untagged and CROI: " << e.what() << std::endl;
      return false;
    }
    
    if ( m_save_croi_space )
      m_croi_data_v.front().saveSpace();

    m_state.croi_run = true;
    
    return true;
  }  

}