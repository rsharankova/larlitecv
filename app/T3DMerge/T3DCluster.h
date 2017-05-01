#ifndef __T3DCluster_H__
#define __T3DCluster_H__

/* =================================================================
 * T3DCluster: track-3d cluster
 * 
 * This is the representation of a track that we use to do our
 * 3D cluster routines on.
 *
 * We define a track to be an ordered list of 3D points.
 * We will be doing a lot of nearest line, nearest point calculations
 * using a kDtree. To save time, we also define a bounding box in order
 * to do broad, first pass collision/overlap testing.
 * 
 * we will make use of a lot of the larlite::geotools and
 * larcv::ANN code base.
 * =================================================================*/

#include <vector>

// larlitecv/BasicTool
#include "GeoAlgo/GeoAABox.h"

namespace larlitecv {

  typedef std::vector<float> Point_t;
  
  class T3DCluster {

  public:
    class Builder; // we use the builder pattern to make sure this class is built ok
    
  protected:
    T3DCluster() {};
    T3DCluster( const std::vector<Point_t>& path, const geoalgo::AABox& bbox );
    virtual ~T3DCluster() {};

    std::vector< Point_t > m_path;
    geoalgo::AABox m_bbox;

  };

  class T3DCluster::Builder {

  protected:
    
    std::vector<Point_t> path;
    geoalgo::AABox bbox;
    
  public:
    
    Builder() {};

    Builder& setPath( const std::vector<Point_t>& path );
    Builder& addPoint( const Point_t& pt );
    Builder& updateBBox();

    T3DCluster build();

    
  };

}

#endif
