#ifndef __ASTAR3D_ALGO_CONFIG__
#define __ASTAR3D_ALGO_CONFIG__

/*
  class: AStar3DAlgoConfig

  This class is responsible for holding the parameters that
  guide the behavior of the AStar3DAlgo class.


 */

// stlib
#include <vector>

// larcv
#include "Base/PSet.h"

namespace larlitecv {

  // ALGO CONFIGURATION
  class AStar3DAlgoConfig {
  public:
    AStar3DAlgoConfig() {
      astar_start_padding = 0;
      astar_end_padding = 0;
      restrict_path = false;
      path_restriction_radius = 10.0;
    };
    virtual ~AStar3DAlgoConfig() {};

    std::vector<float> astar_threshold; // pixel intensity threshold per plane
    std::vector<int>   astar_neighborhood; // size of 3D cube we evaluate neighbor nodes from
    int astar_start_padding; // region around start node where we will accept any node to form path
    int astar_end_padding;   // region around end node where we will accept any node to form path
    int lattice_padding; // once start and end points in 3D space are given, we pad around the axis-aligned box defined by the start and end point
    bool accept_badch_nodes; // allow path to travel through bad nodes
    int min_nplanes_w_hitpixel; // minimum number of planes which must have charge
    bool restrict_path; // restrict the path to be within some distance from the straigh line path
    float path_restriction_radius; // max distance path can deviate from straight line
    int verbosity; // verbosity of the algorithm: 0=most silent, >0 progressively more verbose

    static AStar3DAlgoConfig MakeFromPSet( const larcv::PSet& pset ); //< make a config instance whose values have been set from a larcv PSet

  };

}

#endif