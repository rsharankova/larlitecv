#ifndef __ASTAR_3D_ALGO__
#define __ASTAR_3D_ALGO__

/** 

AStar algorithm assuming 3D!! grid points. 

Uses Image2D to hold image.

 **/

#include <iostream>
#include <queue>
#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>

#include "DataFormat/Image2D.h"

namespace larlitecv {

  // NODE LATTICE 
  class A3DPixPos_t : public std::array<int,3> {
    public:
      A3DPixPos_t() {
        for (int i=0; i<3; i++) at(i) = -1;
      };
      A3DPixPos_t(int u, int v, int w) {
        this->at(0) = u;
        this->at(1) = v;
        this->at(2) = w;
      };
      virtual ~A3DPixPos_t() {};
  };

  typedef std::pair< A3DPixPos_t, AStar3DNode* > a3dpos_pair_t;

  class Lattice : public std::map< A3DPixPost_t, AStar3DNode* > {
  public:
    Lattice( const std::vector<float>& origin, const std::vector<int>& widths, const std::vector<float>& pixel_lengths ) {
      m_origin = origin;
      m_widths = widths;
      m_cm_per_pixel = pixel_lengths;
    }
    virtual ~Lattice() {};

    A3DPixPos_t getNodePos( const std::vector<float>& pos );
    std::vector<float> getPos( const int u, const int v, const int w);
    std::vector<float> getPos( const A3DPixPos_t& );
    AStar3DNode* getNode( const std::vector<float>& pos );
    AStar3DNode* getNode( const int u, const int v, const int w);
    AStar3DNode* getNode( const A3DPixPos_t& nodeid );
  
    const std::vector<int>& widths() const { return m_widths; }
    const std::vector<float>& origin() const { return m_origin; }
    const std::vector<float>& pixel_len() const { return m_cm_per_pixel; }    

  protected:

    std::vector<float> m_origin;
    std::vector<int> m_widths;
    std::vector<float> m_cm_per_pixel;


  };
  // NODE DEFINTION

  class AStar3DNode  {
  public:
    AStar3DNode() {
      u = v = w = 0;
      fscore=gscore=0.0;
      closed = false;
      dir3d.resize(3,0.0);
      prev = NULL;
    };
    AStar3DNode( int u_, int v_, int w_, std::vector<float> dir3d_ ) { 
      u = u_; 
      v = v_;
      w = w_;
      nodeid[0] = u;
      nodeid[1] = v;
      nodeid[2] = w;
      fscore=0.0;
      gscore=0.0;
      dir3d = dir3d_;
      closed = false;
      prev = NULL;
    };
    AStar3DNode( const AStar3DNode& src ) {
      u = src.u;
      v = src.v;
      w = src.w;
      nodeid = src.nodeid;
      fscore = src.fscore;
      gscore = src.gscore;
      dir3d = src.dir3d;
      closed = src.closed;
      prev = src.prev;
    };
    virtual ~AStar3DNode() {};

    int u; // lattice position corresponding to time or x
    int v; // lattice position corresponding to y
    int w; // latrice position corresponding to z
    A3DPixPos_t nodeid;
    float fscore; // score of path: past path + to goal heuristic
    float gscore; // past path score

    std::vector<float> dir3d;
    bool closed;
    AStar3DNode* prev;

    bool operator <(const AStar3DNode& rhs ) const {
      //std::cout << "astar< " << fscore << " < " << rhs.fscore << std::endl;
      if ( fscore > rhs.fscore ) // lower scores get priority
        return true;
      return false;
    };
    bool operator==(const AStar3DNode& rhs ) const {
      if ( u==rhs.u && v==rhs.v && w==rhs.w ) return true;
      return false;
    };

    bool operator!=(const AStar3DNode& rhs ) const {
      if ( u!=rhs.u || v!=rhs.v || w!=rhs.w ) return true;
      return false;
    };
    
  };

  // NODE CONTAINER

  class AStar3DNodePtrList : public std::vector< AStar3DNode* > {

    public:
      AStar3DNodePtrList() {};
      virtual ~AStar3DNodePtrList() {};

    protected:

    // we want to sort from largest to smallest, because we will pop the nodes off the back of the various sets
    struct dirnodeptr_compare_t {
      bool operator() (const AStar3DNode* lhs, const AStar3DNode* rhs ) const {
        if ( lhs->fscore > rhs->fscore )
          return true;
        return false;
      };
    } m_thecomparator;

    public:
    void sort() {
      std::sort(this->begin(),this->end(), m_thecomparator);
    }

    void addnode( AStar3DNode* elem ) {
      push_back( elem );
      sort();
    }

    std::string str() { 
      std::stringsteam ss;
      ss << "(" << u << "," << v << "," << w << ") f=" << fscore;
      return ss.str();
    }

  };

  // ALGO CONFIGURATION
  class AStar3DAlgoConfig {
  public:
    AStar3DAlgoConfig() {
      astar_start_padding = 0;
      astar_end_padding = 0;
    };
    virtual ~AStar3DAlgoConfig() {};

    std::vector<float> astar_threshold;
    std::vector<int>   astar_neighborhood;
    int astar_start_padding;
    int astar_end_padding;
    int lattice_padding;

  };

  // ALGO 
  class AStar3DAlgo {

    AStar3DAlgo() { verbose=2; };
  public:

    AStar3DAlgo( AStar3DAlgoConfig config ) { _config = config; m_badchimg = NULL; };
    virtual ~AStar3DAlgo() {};
    
    void setVerbose( int v ) { verbose = v; };
    void setBadChImage( const larcv::Image2D& badchimg ) { m_badchimg = &badchimg; };
    std::vector<AStar3DNode> findpath( const larcv::Image2D& img, int start_row, int start_col, int goal_row, int goal_col, float thresh, bool use_bad_chs=false );
    std::vector<AStar3DNode> makeRecoPath( AStar3DNode* start, AStar3DNode* goal, int origin_row, int origin_col, bool& path_completed );

    larcv::Image2D visualizeScores( std::string score_name, const larcv::Image2D& orig_img, 
      const int min_c, const int min_r, const int win_c, const int win_r, 
      const std::map<A3DPixPos_t,AStar3DNode*>  position_lookup);

    const std::vector<larcv::Image2D>& getScoreImages() { return m_visualizedimgs; }

  protected:

    AStar3DNode* GetNode( const int u, const int v, const int w, pos_key_t& lattice );

    AStar3DAlgoConfig _config;
    int verbose;

    void evaluateNeighborNodes( AStar3DNode* current, const AStar3DNode* start, const AStar3DNode* goal,
      AStar3DNodePtrList& openset, AStar3DNodePtrList& closedset, 
      const int neighborhood_size, const int min_c, const int min_r, const int win_c, const int win_r, 
      const larcv::Image2D& img, const larcv::ImageMeta& meta, const bool use_bad_chs, std::map< A3DPixPos_t, AStar3DNode* >& position_lookup );

   void evaluateBadChNeighbors( AStar3DNode* current, const AStar3DNode* start, const AStar3DNode* goal,
      AStar3DNodePtrList& openset, AStar3DNodePtrList& closedset, 
      const int neighborhood_size, const int min_c, const int min_r, const int win_c, const int win_r, 
      const larcv::Image2D& img, const larcv::ImageMeta& meta, const bool use_bad_chs, std::map< A3DPixPos_t, AStar3DNode* >& position_lookup);


    const larcv::Image2D* m_badchimg;
    std::vector< larcv::Image2D > m_visualizedimgs;
  };


}

#endif
