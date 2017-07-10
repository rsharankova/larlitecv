// (All of the functions for flash matching from 'ContainedROI/TaggerFlashMatchAlgo.h' are included in "GeneralFlashMatchAlgo.h".
#include "GeneralFlashMatchAlgo.h"

namespace larlitecv {

  GeneralFlashMatchAlgo::GeneralFlashMatchAlgo()
    : m_pmtweights("geoinfo.root")
  {}

  GeneralFlashMatchAlgo::GeneralFlashMatchAlgo( GeneralFlashMatchAlgoConfig& config )
    : m_config(config), m_pmtweights("geoinfo.root") {
    
    setVerbosity( m_config.verbosity );
    m_flash_matcher.Configure( m_config.m_flashmatch_config );
    
    const larutil::Geometry* geo = ::larutil::Geometry::GetME();
    for ( size_t opch=0; opch<32; opch++) {
      m_opch_from_opdet.insert( std::make_pair<int,int>( geo->OpDetFromOpChannel(opch), opch ) );
    }
  }
  
  // A function that will find the flash indices that correspond to flashes that do not correspond to anode/cathode piercing endpoints.
  // Inputs: opflash_v: all of the opflash objects from the event.
  //         anode_flash_idx_v: the indices of the flashes that correspond to boundary points at the anode.
  //         cathode_flash_idx_v: the indices of the flashes that correspond to boundary points at the cathode.
  std::vector < int > GeneralFlashMatchAlgo::non_anodecathode_flash_idx( const std::vector < larlite::opflash* >& opflash_v,
									 std::vector < int > anode_flash_idx_v, std::vector < int > cathode_flash_idx_v ) {
    // Declare a vector for the flashes that are neither anode or cathode crossing.
    std::vector < int > non_ac_flash_idx;
    
    // Loop through the opflash information from the event in order to separate the anode/cathode crossing flashes from the others.
    for (size_t non_ac_iter = 0; non_ac_iter < opflash_v.size(); non_ac_iter++) {
      
      bool belongs_to_ac_flash = false;
      
      // Loop through the anode flashes.
      for (size_t anode_flsh_i = 0; anode_flsh_i < anode_flash_idx_v.size(); anode_flsh_i++) {

	// Compare the two flash indices.
	if ((int)non_ac_iter == anode_flash_idx_v[anode_flsh_i]) belongs_to_ac_flash = true;

      }

      // Loop through the cathode flashes.
      for (size_t cathode_flsh_i = 0; cathode_flsh_i < cathode_flash_idx_v.size(); cathode_flsh_i++) {

        // Compare the two flash indices.
	if ((int)non_ac_iter == cathode_flash_idx_v[cathode_flsh_i]) belongs_to_ac_flash = true;

      }

      // If it was not the same index value as any of the anode/cathode flash indices, then you can append it onto the 'non_ac_flash_idx' vector.
      if (!belongs_to_ac_flash) non_ac_flash_idx.push_back(non_ac_iter);

    }

    // Return the vector of non anode-piercing/cathode-piercing track endpoints.
    return non_ac_flash_idx;

  }


  // A function that will generate a list of opflashes that correspond to the list of indices that are put in.
  // Inputs: full_opflash_v: This is the full list of opflashes from which we are trying to filter
  //   opflashes of the same denomination,
  // i.e. all anode-piercing flashes, all cathode-piercing flashes, all non-side-piercing flashes, etc.
  //         idx_v: This is the list of indices of the flashes that you are interested in.
  // You want to place the flashes located in the 'full_opflash_v' vector at each index in this list
  // in the output list, which contains the opflashes that we are interested in.
  std::vector <larlite::opflash*> GeneralFlashMatchAlgo::generate_single_denomination_flash_list( std::vector< larlite::opflash > full_opflash_v, std::vector < int > idx_v) {

    // Declare a list of flashes that will be the output, which is the list of the certain denomination of flashes.
    std::vector < larlite::opflash* > single_denomination_flash_list;
    single_denomination_flash_list.clear();
    
    for (size_t iflash = 0; iflash < idx_v.size(); iflash++) {
      larlite::opflash* opflash = &(full_opflash_v.at(idx_v.at(iflash)));
      single_denomination_flash_list.push_back( opflash );
    }

    // Return this list of the opflash information of one denomination.
    return single_denomination_flash_list;

  }

  // A function that will generate tracks from the 'BMTrackCluster3D' objects that are formed from each pass of the tagger.
  // Inputs: trackcluster3d_v: A vector of 'BMTrackCluster3D' objects that were formed from a pass of the tagger.
  std::vector < larlite::track >  GeneralFlashMatchAlgo::generate_tracks_between_passes(const std::vector< larlitecv::BMTrackCluster3D > trackcluster3d_v) {

    // We will use the 'makeTrack()' functionality of these 'BMTrackCluster3D' objects to turn them into a vector of 'larlite::track' objects.
    std::vector < larlite::track > output_tracks;

    // Loop through the list of 'BMTrackCluster3D' objects and turn them into a vector 'larlite::track' objects.
    for (auto const& trackcluster3d : trackcluster3d_v) {

      output_tracks.push_back(trackcluster3d.makeTrack());

    }

    // Return this list of tracks.
    return output_tracks;

  }

  // A function that will generate a qcluster from a larlite track object.
  // Inputs: track: A larlite track object that needs to be converted to a cluster of charge.
  flashana::QCluster_t GeneralFlashMatchAlgo::GenerateQCluster( const larlite::track& track ) {

    flashana::QCluster_t qcluster;

    for ( int i=0; i<(int)track.NumberTrajectoryPoints()-1; i++ ) {

      const TVector3& pt     = track.LocationAtPoint(i);
      const TVector3& nextpt = track.LocationAtPoint(i+1);
      const TVector3& ptdir  = track.DirectionAtPoint(i);
      float dist = 0.;
      for ( int v=0; v<3; v++ ) {
        float dx = nextpt[v]-pt[v];
        dist += dx*dx;
      }
      dist = sqrt(dist);

      int nsteps = dist/m_config.qcluster_stepsize;
      if ( fabs( nsteps*m_config.qcluster_stepsize - dist )>0.01 ) {
        nsteps+=1;
      }

      float step = dist/float(nsteps);

      for (int istep=0; istep<nsteps; istep++) {
        float pos[3] = {0};
        for (int v=0; v<3; v++)
          pos[v] = pt[v] + step*ptdir[v];
        flashana::QPoint_t qpt( pos[0], pos[1], pos[2], step*m_config.MeV_per_cm );
        qcluster.emplace_back( std::move(qpt) );
      }
    }

    return qcluster;
  }

    // A function that will make flash hypotheses from an entire vector of larlite tracks.
  std::vector<flashana::QCluster_t> GeneralFlashMatchAlgo::GenerateQCluster_vector( const std::vector<larlite::track>& track_v ) {
    
    std::vector<flashana::QCluster_t> qcluster_vector;
    for ( auto& input_track : track_v ) {
      flashana::QCluster_t qcluster = GenerateQCluster( input_track );
      qcluster_vector.emplace_back( std::move(qcluster) );
    }
    return qcluster_vector;
  }


  // A function that will find the center and range of a flash.
  // Inputs: flash: This is hte larlite flash object that is being considered.
  //         zmean: This is the mean z position of the flash.
  //         zrange: This is the range in the z variable that we are considering.
  void GeneralFlashMatchAlgo::GetFlashCenterAndRange( const larlite::opflash& flash, float& zmean, std::vector<float>& zrange ) {

    zmean = 0.;
    zrange.resize(2,0.0);
    float qtot = 0.0;
    for (int ipmt=0; ipmt<32; ipmt++) {
      zmean += flash.PE(ipmt)*m_pmtweights.pmtpos[ipmt][2];
      qtot  += flash.PE( ipmt );
    }

    if (qtot>0)
      zmean /= qtot;

    float mindist = 1000;
    float maxdist = 0;

    for (int ipmt=0; ipmt<32; ipmt++) {
      if (flash.PE(ipmt)>m_config.pmtflash_thresh) {
        float dist = m_pmtweights.pmtpos[ipmt][2]-zmean;
        if (dist<0 || mindist>dist)
          mindist = dist;
        else if ( dist>0 && maxdist<dist )
          maxdist = dist;
      }
    }

    if (qtot<=0 )  {
      // no information, so set wide range                                                                                                                                                                  
      zrange[0] = 0;
      zrange[1] = 3455;
    }
    else {
      zrange[0] = (zmean+mindist-10.0)/0.3; // conversion from cm -> wire                                                                                                                                   
      zrange[1] = (zmean+maxdist+10.0)/0.3; // conversion from cm -> wire                                                                                                                                   
    }

    // bounds check                                                                                                                                                                                         
    if ( zrange[0]<0 ) zrange[0] = 0;
    if ( zrange[1]>=3455 ) zrange[1] = 3455;

  }

  // A function that will generate an unfitted flash hypothesis for the track.
  // Inputs: qcluster: This is the qcluster that will be used to generate the flash hypothesis.
  flashana::Flash_t GeneralFlashMatchAlgo::GenerateUnfittedFlashHypothesis( const flashana::QCluster_t& qcluster ) {
    const flashana::QLLMatch* matchalgo = (flashana::QLLMatch*)m_flash_matcher.GetAlgo( flashana::kFlashMatch );
    flashana::Flash_t unfitted_hypothesis = matchalgo->GetEstimate( qcluster );
    return unfitted_hypothesis;
  }
  
  // A function that will make an opflash from a data flash.
  // Input: Flash: This is an object of type 'Flash_t' (a data flash).
  larlite::opflash GeneralFlashMatchAlgo::MakeOpFlashFromFlash(const flashana::Flash_t& Flash) {

    const larutil::Geometry* geo = ::larutil::Geometry::GetME();

    std::vector<double> PEperOpDet( Flash.pe_v.size(), 0.0);
    for (unsigned int femch = 0; femch < Flash.pe_v.size(); femch++) {
      unsigned int opdet = geo->OpDetFromOpChannel(femch);
      PEperOpDet[femch] = Flash.pe_v[opdet];
    }

    larlite::opflash flash( Flash.time, 0, 0, 0, PEperOpDet, false, false, 1.0, Flash.y, Flash.y_err, Flash.z, Flash.z_err );

    return flash;
  }

  // A function that will calculate the 2D gaussian value for an opflash object.
  // Inputs: 'flash' - The input 'opflash' object to be fit to a Gaussian 2D value.
  larlite::opflash GeneralFlashMatchAlgo::GetGaus2DPrediction( const larlite::opflash& flash ) {
    const larutil::Geometry* geo = ::larutil::Geometry::GetME();
    float tot_weight = 0;

    // Get Charge Weighted Mean
    float mean[2] = {0.0};
    std::vector<double> xyz(3,0.0);
    for (int iopdet=0; iopdet<32; iopdet++ ) {
      larutil::Geometry::GetME()->GetOpDetPosition( iopdet, xyz );
      mean[0] += xyz[1]*flash.PE(iopdet);
      mean[1] += xyz[2]*flash.PE(iopdet);
      tot_weight += flash.PE(iopdet);
    }
    for (int i=0; i<2; i++)
      mean[i] /= tot_weight;

    /// Get Charge-weighted covariance Matrix
    float cov[2][2] = {0};
    for (int iopdet=0; iopdet<32; iopdet++) {
      larutil::Geometry::GetME()->GetOpDetPosition( iopdet, xyz );
      float dx[2];
      dx[0] = xyz[1] - mean[0];
      dx[1] = xyz[2] - mean[1];
      for (int i=0; i<2; i++) {
        for (int j=0; j<2; j++ ) {
          cov[i][j] += dx[i]*dx[j]*flash.PE(iopdet)/tot_weight;
        }
      }
    }

    // Get Inverse Covariance Matrix
    float det = cov[0][0]*cov[1][1] - cov[1][0]*cov[0][1];
    float invcov[2][2] = { { cov[1][1]/det, -cov[0][1]/det},
                           {-cov[1][0]/det,  cov[0][0]/det } };
    float normfactor = 1.0/sqrt(2*3.141159*det);


    // build guas ll flash hypothesis using cov matrix
    std::vector< double > PEperOpDet(32,0);
    float normw = 0.;
    for ( int iopdet=0; iopdet<32; iopdet++) {
      larutil::Geometry::GetME()->GetOpDetPosition( iopdet, xyz );
      float mahadist = 0.;
      float yz[2] = { (float)(xyz[1]), (float)(xyz[2]) };
      for (int i=0; i<2; i++ ) {
        for (int j=0; j<2; j++) {
          mahadist += (yz[i]-mean[i])*invcov[i][j]*(yz[j]-mean[j]);
        }
      }
      PEperOpDet[iopdet] = normfactor*exp(-0.5*mahadist);
      normw += PEperOpDet[iopdet];
    }

    // normalize back to totalweight                                            
    if ( normw>0 ) {
      for (int iopdet=0; iopdet<32; iopdet++) {
        PEperOpDet[iopdet] *= tot_weight/normw;
      }
    }

    // make flash                                                               
    larlite::opflash gaus2d_flash( flash.Time(), flash.TimeWidth(), flash.AbsTi\
me(), flash.Frame(), PEperOpDet );
    return gaus2d_flash;
  }


  // A function that will make a data flash (an object of type 'Flash_t') from an 'opflash'.
  // Inputs: flash: This is an object of type 'opflash' that has to be converted into a data flash.
  flashana::Flash_t GeneralFlashMatchAlgo::MakeDataFlash( const larlite::opflash& flash ) {

    // get geometry info                                                                                                                                                                                    
    const larutil::Geometry* geo = ::larutil::Geometry::GetME();

    // provide mean and basic width                                                                                                                                                                         
    float wire_mean;
    std::vector<float> wire_range;
    GetFlashCenterAndRange( flash, wire_mean, wire_range );
    if ( m_verbosity>0 )
      std::cout << "Flash: pe=" << flash.TotalPE() << " wire_mean=" << wire_mean << " wire_range=[" << (int)wire_range[0] << "," << (int)wire_range[1] << "]" << std::endl;

    // also load flash info into flash manager                                                                                                                                                              
    flashana::Flash_t f;
    f.x = f.x_err = 0;
    f.y = flash.YCenter();
    f.z = flash.ZCenter();
    f.y_err = flash.YWidth();
    f.z_err = flash.ZWidth();
    f.pe_v.resize(geo->NOpDets());
    f.pe_err_v.resize(geo->NOpDets());
    for (unsigned int i = 0; i < f.pe_v.size(); i++) {
      unsigned int opdet = geo->OpDetFromOpChannel(i);
      f.pe_v[opdet] = flash.PE(i) / m_config.gain_correction[i];
      f.pe_err_v[opdet] = sqrt(flash.PE(i) / m_config.gain_correction[i]);
    }
    f.time = flash.Time();
    f.idx = 0;

    return f;
  }
  
  // A function that will generate a flash hypothesis for a larlite track.  This will follow the logic at the beginning of the 'GeneralFlashMatchAlgo::InTimeFlashComparison' function.
  // Inputs: input_track: This is the input larlite track needed to generate the flash hypothesis.  It will be converted to a qcluster, which will be converted a flash_t object, which will be
  // converted into an opflash object.
  // *** We will not be using the gaus2d object here right now. ****
  //         taggerflashmatch_pset: This is the input parameter set used to instatiate the GeneralFlashMatchAlgo object.
  larlite::opflash GeneralFlashMatchAlgo::make_flash_hypothesis(const larlite::track input_track) {

    // Follow the logic of the 'GeneralFlashMatchAlgo::InTimeFlashComparison' function to generate a flash hypothesis.
    flashana::QCluster_t qcluster = GenerateQCluster(input_track);
    flashana::Flash_t flash_hypo  = GenerateUnfittedFlashHypothesis( qcluster );
    larlite::opflash opflash_hypo = MakeOpFlashFromFlash( flash_hypo );
    larlite::opflash gaus2d_hypo  = GetGaus2DPrediction( opflash_hypo );

    // What to return depends on the values set in the 'Config' class.
    if (!m_config.use_gaus2d)
      return opflash_hypo;

    else
      return gaus2d_hypo;

  }

  
  // A function that will make flash hypotheses from an entire vector of larlite tracks.
  std::vector<larlite::opflash> GeneralFlashMatchAlgo::make_flash_hypothesis_vector( const std::vector<larlite::track>& input_track_v ) {
    std::vector<larlite::opflash> opflash_hypo_v;
    for ( auto& input_track : input_track_v ) {
      larlite::opflash opflash_hypo = make_flash_hypothesis( input_track );
      opflash_hypo_v.emplace_back( std::move(opflash_hypo) );
    }
    return opflash_hypo_v;
  }
  
// A function that will take in reconstructed track and flash info and return a chi2 fit between them, using the 'GeneralFlashMatchAlgo::InTimeFlashComparison' class.
// Inputs: opflash_hypo: This is the flash hypothesis that is used in compare to the data flash.
//         qcluster: The qcluster that corresponds to the opflash hypothesis.
//         data_opflash: This is the 'data_flash' that you are matching to the 'flash_hypothesis' opflash object for the track.
  float GeneralFlashMatchAlgo::generate_chi2_in_track_flash_comparison(const flashana::QCluster_t qcluster, const larlite::opflash data_opflash) {
    
    // Convert 'data_opflash' into type 'Flash_t' (a data flash).
    flashana::Flash_t data_flasht = MakeDataFlash( data_opflash );

    // Convert 'qcluster' into an unfitted flash hypothesis.
    flashana::Flash_t flash_hypo = GenerateUnfittedFlashHypothesis( qcluster );

    // Convert the 'flash_hypo' to a data flash and then a Gaussian 2D object to have that below.
    larlite::opflash opflash_hypo = MakeOpFlashFromFlash( flash_hypo );
    larlite::opflash gaus2d_hypo  = GetGaus2DPrediction( opflash_hypo );
    
    // Use the geometry package at this point in the algorithm.
    const larutil::Geometry* geo = ::larutil::Geometry::GetME();

    // Declare the variables for finding the chi2 value.
    float chi2 = 0.;
    float tot_pe_hypo = 0.;
    float tot_pe_data = 0.;
    std::vector< float > expectation(32,0);

    // This begins the part of the code that finds the chi2 value.
    for (size_t i=0; i<data_flasht.pe_v.size(); i++) {
      float observed = data_flasht.pe_v.at(i);
      float expected = flash_hypo.pe_v.at(i)*m_config.fudge_factor;
      tot_pe_data += observed;
      tot_pe_hypo += expected;
      if ( observed>0 && expected==0 ) {
	if ( !m_config.use_gaus2d )
	  expected = 1.0e-3;
	else
	  expected = gaus2d_hypo.PE( geo->OpDetFromOpChannel(i) )*m_config.fudge_factor;
      }
      expectation[i] = expected;

      float dpe = (expected-observed);
      if ( observed>0 )
	dpe += observed*( log( observed ) - log( expected ) );
      chi2 += 2.0*dpe;
    }
    chi2 /= 32.0;

    std::cout << "  [observed] ";
    for (int ich=0; ich<32; ich++ ) {
      std::cout << std::setw(5) << (int)data_flasht.pe_v.at(  geo->OpDetFromOpChannel(ich) );
    }
    std::cout << " TOT=" << tot_pe_data << " CHI2=" << chi2 << std::endl;

    std::cout << "  [expected] ";
    for ( int ich=0; ich<32; ich++ ) {
      //std::cout << std::setw(5) << (int)(opflash_hypo.pe_v.at(  geo->OpDetFromOpChannel(ich) )*m_config.fudge_factor);
      //std::cout << std::setw(5) << (int)(opflash_hypo.pe_v.at( ich )*m_config.fudge_factor);
      std::cout << std::setw(5) << (int)expectation[ich];
    }
    std::cout << " TOT=" << tot_pe_hypo << " BestCHI2=" << chi2 << std::endl;

    // Return the chi2 value.
    return chi2;
        
  }
 
}