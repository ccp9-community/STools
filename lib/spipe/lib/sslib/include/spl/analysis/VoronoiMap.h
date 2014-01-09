/*
 * VoronoiMap.h
 *
 *  Created on: Aug 17, 2011
 *      Author: Martin Uhrin
 */

#ifndef VORONOI_MAP_H
#define VORONOI_MAP_H

// INCLUDES ////////////
#include "spl/SSLib.h"

#ifdef SPL_WITH_CGAL

#include <map>
#include <set>

#include <CGAL/config.h>
#include <CGAL/basic.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Voronoi_diagram_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_traits_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_policies_2.h>




// FORWARD DECLARATIONS ///////


// DEFINITION ///////////////////////

namespace spl {
namespace analysis {

//class VoronoiMap
//{
//public:
//
//
//  void addData(const ::arma::vec2 & point, const ::std::string & tag);
//
//
//private:
//
//  // typedefs for defining the adaptor
//  typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
//  typedef CGAL::Delaunay_triangulation_2<Kernel> DelaunayTriangulation;
//  typedef CGAL::Delaunay_triangulation_adaptation_traits_2<DelaunayTriangulation> AdaptationTraits;
//  typedef CGAL::Delaunay_triangulation_caching_degeneracy_removal_policy_2<DelaunayTriangulation> RemovalPolicy;
//  typedef CGAL::Voronoi_diagram_2<DelaunayTriangulation, AdaptationTraits, RemovalPolicy>
//    VoronoiDiagram;
//  typedef AdaptationTraits::Site_2 Site2;
//
//  typedef ::std::set< ::std::string> TagSet;
//  typedef ::std::map< Site2, TagSet > PointTagMap;
//
//
//  PointTagMap     myPointTags;
//
//  VoronoiDiagram  myVoronoi;
//
//
//};

}
}


#endif /* SPL_WITH_CGAL */
#endif /* VORONOI_MAP_H */
