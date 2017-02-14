#include "Linear3DFitter.h"
#include <vector>
#include <cmath>

// larcv
#include "UBWireTool/UBWireTool.h"
#include "DataFormat/ImageMeta.h"

// larlite
#include "LArUtil/LArProperties.h"
#include "LArUtil/Geometry.h"

namespace larlitecv {

  // I have added the variables 'time_comp_factor' and 'wire_comp_factor' to this definition when they were not here previously
  PointInfoList Linear3DFitter::findpath( const std::vector<larcv::Image2D>& img_v, const std::vector<larcv::Image2D>& badch_v, 
    const int start_row, const int goal_row, const std::vector<int>& start_cols, const std::vector<int>& goal_cols  ) {
    
    const larcv::ImageMeta& meta = img_v.front().meta();

    // turn image pixel coordinates into a 3D start and goal point
    std::vector<int> start_wids(start_cols.size());
    std::vector<int> goal_wids(goal_cols.size());    
    for (size_t p=0; p<goal_cols.size(); p++) {
      start_wids[p] = img_v.at(p).meta().pos_x( start_cols[p] );
      goal_wids[p]  = img_v.at(p).meta().pos_x( goal_cols[p] );      
    }
    double start_tri = 0.;
    int start_crosses = 0;
    std::vector< float > poszy_start(2,0.0);
    larcv::UBWireTool::wireIntersection( start_wids, poszy_start, start_tri, start_crosses );

    double goal_tri = 0.;
    int goal_crosses = 0;
    std::vector< float > poszy_goal(2,0.0);
    larcv::UBWireTool::wireIntersection( goal_wids, poszy_goal, goal_tri, goal_crosses );

    const float cm_per_tick = ::larutil::LArProperties::GetME()->DriftVelocity()*0.5; // [cm/usec] * [usec/tick] = [cm/tick]    

    std::vector<float> startpos(3,0);
    startpos[0] = (meta.pos_y( start_row )-3200.0)*cm_per_tick;
    startpos[1] = poszy_start[1];
    startpos[2] = poszy_start[0];

    std::vector<float> goalpos(3,0);
    goalpos[0] = (meta.pos_y( goal_row )-3200.0)*cm_per_tick;
    goalpos[1] = poszy_goal[1];
    goalpos[2] = poszy_goal[0];

    if ( start_crosses==0 || goal_crosses==0 ) {
      throw std::runtime_error("Linear3DFitter::findpath[error] start or goal point not a good 3D space point.");
    }

    // This begins the code for the 3D linear fitter

    // This ends the information that can be encapsulated into the function that finds the path between the starting point and the ending point.
    float step_size = 0.3;
    float min_ADC_value = 10.0;
    float neighborhood_square = 5;
    PointInfoList track = pointsOnTrack(img_v, badch_v, startpos, goalpos, step_size, min_ADC_value, neighborhood_square );

    // Return the combination of 'Output_From_Points_On_Track' and 'Output_From_Last_Point_On_Track'
    return track;
  }


  // Now, call a function 'pointsOnTrack' that will return all of the pixel coordinates on or near the track that should be illuminated based on how much charge the tagger finds within each of the voxels                                                                                                                                                            
  // Input values:                                                                                                                                                             
  // The image ('img_v') (type const std::vector<larcv::Image2D>&)                                                                                                              
  // The bad channel image ('badch_v') (type const std::vector<larcv::Image2D>&)                                                                                              
  // The coordinates of the initial point ('initial_point_coords') (type std::vector<float>)                                                          
  // The coordinates of the final point ('final_point_coords') (type std::vector<float>)                                                                                        
  // The step size ('step_size') (type float)                                                                                                                             
  // The total distance between the trajectory points ('distance_between_trajectory_points') (type float)                                                                 
  // The minimum ADC value ('min_ADC_value') (type float)                                                                                                                 
  // The neighborhood size that you are interested in considering ('neighborhood_square') (type 'int')
  // Now I can start writing the 'pointsOnTrack' function, which will return the number of points with charge on the track, the number of points without charge on the track, and information about neighboring points that contain charge. 

  // Note: I will consider a channel to be dead if it has a value greater than 0.0 in 'badch_v', which is the vector of plane images for the event

  PointInfoList Linear3DFitter::pointsOnTrack(const std::vector<larcv::Image2D>& img_v, const std::vector<larcv::Image2D>& badch_v, 
    const std::vector<float>& initial_point_coords, const std::vector<float>& final_point_coords, 
    const float step_size, const float min_ADC_value, const int neighborhood_size) {

    // Declare a variable equal to the 'cm_per_tick', because I need this information to convert between the x-coordinate value and the tick in the image of charge
    const float cm_per_tick = ::larutil::LArProperties::GetME()->DriftVelocity()*0.5; // [cm/usec] * [usec/tick] = [cm/tick]
    const int nplanes = img_v.size();

    // Find the distance between the starting point and the end point 
    float total_distance_between_points = 0;
    for (size_t i=0; i<3; i++) {
      float dx = (final_point_coords[i]-initial_point_coords[i]);
      //if ( i==0 ) dx *= cm_per_tick;
      total_distance_between_points += dx*dx;
    }
    total_distance_between_points = sqrt(total_distance_between_points);

    // Calculate the cosine along each of the axes so that you can find the coordinates of each step that you take along the track   
    // The prescription for the cosine values is the total component of the vector in that direction (the goal point - the starting point) divided by 'distance_between_trajectory_points', the length of the particle's entire trajectory                                                                                                                     
    std::vector<float> dircos(3,0.0);
    for (size_t i=0; i<3; i++) {
      dircos[i] = (final_point_coords[i]-initial_point_coords[i])/total_distance_between_points;
    }

    // From the step size, compute the total number of points that are located on the track.  This number will be almost certainly be a float, because the constraint on the 'step_size' as a fixed parameter does nothing to set the number of points (listed here as 'num_of_steps') to be an integer value.  I will include a test for the endpoint of the track's trajectory, which will not be included in one of the points along the track's trajectory that will undergo a test.
    int num_of_steps = total_distance_between_points/step_size;
    if ( fabs(step_size*num_of_steps-total_distance_between_points)!=0.0 )
      num_of_steps++;

    // recalibrate stepsize so it divides evenly into distance
    std::vector<float> step_size_v(3,0.0);
    for (size_t i=0; i<3; i++)
      step_size_v[i] = dircos[i]*(total_distance_between_points/float(num_of_steps));

    // To optimize the algorithm, I declare all of the new variables here inside Point, and then fill them with each iteration of the track
    // this is passed by moving to container. avoids copies.
    
    // The output class
    PointInfoList track;    

    // The iterator over the 'num_of_steps' can be of type 'size_t', because we are checking points less than the distance of one wire from another.
    for (int wire_iterator = 0; wire_iterator <= num_of_steps; wire_iterator++) {

      PointInfo pt;

      // I will declare a data product that will be used within the 'WireCoordinate' function, just as Taritree did it
      Double_t xyz[3] = {0};
      pt.xyz.resize(3,0);
      for (size_t i=0; i<3; i++) {
        xyz[i] = initial_point_coords[i] + step_size_v[i]*float(wire_iterator);
        pt.xyz[i] = xyz[i];
      }

      // Convert the coordinates of the wire at this value to a wire index value that can be rounded using the rounding function
      // Find the initial wire number based on the starting coordinate of the particle's path                                              
      
      // Declare a vector of the three coordinates within the detector.  I will continue down this route, because I am not sure that all of the starting points come from the same set of channels.  This allows me to do it myself instead of going back into the 'wireIntersection' function.
      pt.wire_id.resize(nplanes,0);
      for (int wire_plane_iterator = 0; wire_plane_iterator < nplanes; wire_plane_iterator++) {
        pt.wire_id[wire_plane_iterator] = round( larutil::Geometry::GetME()->WireCoordinate( xyz , wire_plane_iterator ) );
      }

      // Once you have this information, you can convert the 'wire_id' values to the columns and row in the image based on the compression factor
      // Convert the current x0 value back to the time
      pt.tick  = xyz[0]/cm_per_tick+3200.0;

      // Image coordinates
      pt.cols.resize(nplanes,0);
      try {
        pt.row = img_v.front().meta().row(pt.tick);
        for (int p=0; p<nplanes; p++ )
          pt.cols[p] = img_v.at(p).meta().col( pt.wire_id[p] );
      }
      catch (...) {
        // this position can't be mapped to the image. go to next position.
        continue;
      }

      // is this the same point as before? if so, let's move on.
      if ( track.size()>0 && track.back()==pt )
        continue;

      pt.planehascharge.resize(nplanes,false);
      pt.planehasbadch.resize(nplanes,false);

      // Loop through all of the points on each of the planes to see if they contain the correct amount of charge
      // Now that I have this information, I can loop through each of the planes and see if each of the pixels have above the threshold amount of charge
      pt.planeswithcharge = 0;      
      for (int plane_index = 0; plane_index < nplanes; plane_index++) {

        // Find the pixel value, badch status in the neighborhood of this coordinate value of (row, column)
        pt.planehascharge[plane_index] = doesNeighboringPixelHaveCharge( img_v.at(plane_index), pt.row, pt.cols[plane_index], neighborhood_size, min_ADC_value );
        pt.planehasbadch[plane_index]  = isNeighboringPixelDeadPixel( badch_v.at(plane_index), pt.row, pt.cols[plane_index], neighborhood_size );
        if ( pt.planehascharge[plane_index])
          pt.planeswithcharge++;
      }//end of plane loop 

      // check if good, i.e. the plane either has charge or badch 
      // also sum charge
      pt.goodpoint = true;
      for (int p=0; p<nplanes; p++) {
        if ( !pt.planehascharge[p] && !pt.planehasbadch[p] ) {
          pt.goodpoint = false;
          break;
        }
      }
       
      track.emplace( std::move(pt) ); 

    } // End of the loop over the wires (for some reason I'm getting mismatched parentheses)
 
    return track;

  } // End of the function

  // Declare a function that will search for charge in the vicinity of a pixel (one that is either dead or empty)
  // This function will be a boolean and will require the following information:
  // 'img_v' - This is the vector of images that is being examined for charge
  // 'plane_num' - This is an 'int' which corresponds to the plane that we are looking at 
  // 'central_row'       - This is an 'int' which corresponds to the row number of the central pixel
  // 'central_col'    - This is an 'int' which corresponds to the column number of the central pixel around which we will search for charge
  // 'neighborhood size' - This is an 'int' which specifies the size of the square around the central pixel that we will search for charge
  // 'min_ADC_value' - This is the minimum ADC value that a pixel in the vicinity of the central pixel can have so that it is deemed true that there is charge in the vicinity of the central pixel
  bool Linear3DFitter::doesNeighboringPixelHaveCharge(const larcv::Image2D& img, int central_row, int central_col, int neighborhood_size, float min_ADC_value) {

    // You can define variables for the initial row and the final row based on where in the TPC they are
    int last_row_index = (int)img.meta().rows() - 1;
    int last_col_index = (int)img.meta().cols() - 1;

    // Declare variables for the starting and ending coordinates in the search
    int starting_row_coord = central_row - neighborhood_size;
    int ending_row_coord   = central_row + neighborhood_size;
    int starting_col_coord = central_col - neighborhood_size;
    int ending_col_coord   = central_col + neighborhood_size;

    // If I am close to the boundary of the image, then I'll have to adjust the search for this neighborhood to end on the boundary of the image.
    if (starting_row_coord < 0)
        starting_row_coord = 0;

    if (starting_row_coord > last_row_index)
      starting_row_coord = last_row_index;

    if (ending_row_coord < 0)
      ending_row_coord = 0;

    if (ending_row_coord > last_row_index)
      ending_row_coord = last_row_index;

    if (starting_col_coord < 0)
      starting_col_coord = 0;

    if (starting_col_coord > last_col_index)
      starting_col_coord = last_col_index;

    if (ending_col_coord < 0)
      ending_col_coord = 0;

    if (ending_col_coord > last_col_index)
      ending_col_coord = last_col_index;

    
    // Begin a loop over the rows and the columns of the image that are in the vicinity of 'central_row' and 'central_col' to see if there is any charge in the vicinity
    for (int col_iter = starting_col_coord; col_iter <= ending_col_coord; col_iter++) {

      for (int row_iter = starting_row_coord; row_iter <= ending_row_coord; row_iter++) {

        // See if any pixel in this neighborhood has charge above threshold.  If it does, then return 'true' for 'charge_in_vicinity'.
        if (img.pixel(row_iter, col_iter) > min_ADC_value) {
          // no need to keep checking
          return true;
        }

      }

    }

    return false;
  }

  // Declare a channel that will search for dead pixels in the vicinity of the central pixel.  This will take the same input parameters and use the same logic as the algorithm above, except it will take 'badch_v' as an input instead of 'img_v', and it will see if any of the entries in the vicinity of the central pixel have a value in the array that's greater than 0.0.  The 'float' input argument 'min_ADC_value' is not needed for this function.
  bool Linear3DFitter::isNeighboringPixelDeadPixel(const larcv::Image2D& badch, int central_row, int central_col, int neighborhood_size) {

    // You can define variables for the initial row and the final row based on where in the TPC they are 
    int last_row_index = (int)badch.meta().rows() - 1;
    int last_col_index = (int)badch.meta().cols() - 1;

    // Declare variables for the starting and ending coordinates in the search                                                                                            
    int starting_row_coord = central_row - neighborhood_size;
    int ending_row_coord   = central_row + neighborhood_size;
    int starting_col_coord = central_col - neighborhood_size;
    int ending_col_coord   = central_col + neighborhood_size;

    // If I am close to the boundary of the image, then I'll have to adjust the search for this neighborhood to end on the boundary of the image.                 
    if (starting_row_coord < 0)
      starting_row_coord = 0;

    if (starting_row_coord > last_row_index)
      starting_row_coord = last_row_index;

    if (ending_row_coord < 0)
      ending_row_coord = 0;

    if (ending_row_coord > last_row_index)
      ending_row_coord = last_row_index;

    if (starting_col_coord < 0)
      starting_col_coord = 0;

    if (starting_col_coord > last_col_index)
      starting_col_coord = last_col_index;

    if (ending_col_coord < 0)
      ending_col_coord = 0;

    if (ending_col_coord > last_col_index)
      ending_col_coord = last_col_index;

    // Begin a loop over the rows and the columns of the image that are in the vicinity of 'central_row' and 'central_col' to see if there is any charge in the vicinity     
    for (int col_iter = starting_col_coord; col_iter <= ending_col_coord; col_iter++) {

      for (int row_iter = starting_row_coord; row_iter <= ending_row_coord; row_iter++) {

        // See if any pixel in this neighborhood has charge above threshold.  If it does, then return 'true' for 'charge_in_vicinity'.              
        if (badch.pixel(row_iter, col_iter) > 0.0) {
          // no need to keep checking, return function.
          return true;
        }

      }

    }

    // Return 'charge_in_vicinity'                                                                                                                                              
    return false;

  }


  // ================================================================

#ifndef __CINT__
#ifndef __CLING__
  void PointInfoList::emplace( PointInfo&& pt ) {

    // tally
    if ( pt.planeswithcharge==(int)pt.cols.size() )
      num_pts_w_allcharge++;
    else if ( pt.planeswithcharge==0 && pt.goodpoint )
      num_pts_w_allbadch++;
    else if ( pt.planeswithcharge==0 && !pt.goodpoint )
      num_pts_w_allempty++;

    if ( pt.goodpoint )
      num_pts_good++;

    // then store
    emplace_back( std::move(pt) );
  }

#endif
#endif
  
} // This should match the end of namespace larcv......

