/*
 * ConvexHull.h
 *
 *  Created on: Jul 9, 2013
 *      Author: Martin Uhrin
 */

#ifndef CONVEX_HULL_H
#define CONVEX_HULL_H

// INCLUDES ////////////
#include "SSLib.h"

#ifdef SSLIB_USE_CGAL

#include <map>
#include <vector>
#include <string>
#include <utility>

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>

#include <CGAL/Origin.h>
#include <CGAL/Convex_hull_d.h>
#include <CGAL/Fraction_traits.h>
//#if CGAL_USE_GMP && CGAL_USE_MPFR
//#  include <CGAL/Gmpfr.h>
//#else
#  include <CGAL/CORE_Expr.h>
//#endif

#include "OptionalTypes.h"
#include "analysis/AbsConvexHullGenerator.h"
#include "common/AtomsFormula.h"
#include "analysis/CgalCustomKernel.h"
#include "common/Structure.h"
#include "utility/HeterogeneousMapKey.h"

// DEFINITION ///////////////////////

namespace sstbx
{

// FORWARD DECLARATIONS ///////

namespace analysis
{

class HullEntry;

class ConvexHull : public AbsConvexHullGenerator, ::boost::noncopyable
{
public:
//#ifdef CGAL_USE_GMP
//  typedef CGAL::Quotient<CGAL::Gmpfr> RT;
//#else
  typedef CORE::Expr NT; // Number type
  typedef CGAL::Quotient< NT> RT; // Ring type
//#endif
  //typedef CGAL::Cartesian_d<RT> HullTraits;
  typedef CgalCustomKernel< RT> HullTraits;
public:

  typedef CGAL::Convex_hull_d< HullTraits> Hull;
  typedef HullTraits::Point_d PointD;
  typedef HullTraits::Vector_d VectorD;
  typedef ::std::pair< common::AtomsFormula, PointD> Endpoint;
  typedef ::std::vector< common::AtomsFormula> EndpointLabels;
  typedef ::std::vector< Endpoint> Endpoints;
  typedef PointD::Id PointId;

  typedef Endpoints::const_iterator EndpointsConstIterator;

  static const NT NT_ZERO;
  static const HullTraits::FT FT_ZERO;
  static const HullTraits::RT RT_ZERO;

  class HullEntry
  {
  public:
    HullEntry(const common::AtomsFormula & composition,
        const HullTraits::FT value, const PointId id, const bool isEndpoint);

    const common::AtomsFormula &
    getComposition() const;
    const HullTraits::FT
    getValue() const;
    const PointId &
    getId() const;
    const ::boost::optional< PointD> &
    getPoint() const;
    bool
    isEndpoint() const;
  private:
    void
    setPoint(const PointD & point);

    common::AtomsFormula myComposition;
    HullTraits::FT myValue;
    PointId myId;
    ::boost::optional<PointD> myPoint;
    bool myIsEndpoint;

    friend class ConvexHull;
  };

  typedef ::std::vector< HullEntry>::const_iterator EntriesConstIterator;

  template< typename InputIterator>
    static EndpointLabels
    generateEndpoints(InputIterator first, InputIterator last);

  ConvexHull(const EndpointLabels & endpoints);
  ConvexHull(const EndpointLabels & endpoints,
      utility::Key< double> & convexProperty);
  template< typename InputIterator>
    ConvexHull(InputIterator first, InputIterator last);
  virtual
  ~ConvexHull()
  {
  }

  PointId
  addStructure(const common::Structure & structure);
  template< typename InputIterator>
    ::std::vector< PointId>
    addStructures(InputIterator first, InputIterator last);

  /**
   * Get the number of dimensions, including the convex property dimension.
   */
  int
  dims() const;

  const Hull *
  getHull() const;

  PointD
  composition(const PointD & point) const;

  EndpointsConstIterator
  endpointsBegin() const;
  EndpointsConstIterator
  endpointsEnd() const;

  EntriesConstIterator
  entriesBegin() const;
  EntriesConstIterator
  entriesEnd() const;

  ::boost::optional< bool>
  isStable(const PointD & point) const;

  ::boost::optional<bool>
  isStable(const PointId id) const;

  OptionalDouble distanceToHull(const common::Structure & structure) const;
  OptionalDouble distanceToHull(const PointId id) const;

private:

  typedef ::std::map< common::AtomsFormula, HullTraits::FT> ChemicalPotentials;
  typedef ::std::vector< HullEntry> HullEntries;

  PointId
  generateEntry(const common::Structure & structure);
  void
  updateChemicalPotential(const common::AtomsFormula & endpointFormula,
      const HullTraits::FT value);
  PointD
  generateHullPoint(const HullEntry & entry) const;
  PointD

  /**
   * Precondition: composition is composed of multiples of one or more hull endpoints.
   */
  generateHullPoint(const common::AtomsFormula & composition, const HullTraits::FT & convexValue) const;
  void
  generateHull() const;
  bool
  canGenerate() const;
  void
  initEndpoints(const EndpointLabels & labels);
  ::boost::optional<HullTraits::RT>
  distanceToHull(const PointD & p) const;
  bool isEndpoint(const PointD & p) const;

  bool isTopFacet(Hull::Facet_const_iterator facet) const;

  void printPoint(const PointD & p) const;

  // IMPORTANT: Make sure myEndpoints the first member because this is relied on by one of
  // the constructors
  Endpoints myEndpoints;
  const utility::Key< double> myConvexProperty;
  ChemicalPotentials myChemicalPotentials;
  const int myHullDims;
  mutable HullEntries myEntries;
  mutable ::boost::scoped_ptr< Hull> myHull;
};

template< typename InputIterator>
  ConvexHull::EndpointLabels
  ConvexHull::generateEndpoints(InputIterator first, InputIterator last)
  {
    ::std::set< ::std::string> speciesSet;
    ::std::vector< ::std::string> species;
    EndpointLabels endpoints;
    for(InputIterator it = first; it != last; ++it)
    {
      it->getAtomSpecies(species);
      ::std::set< ::std::string> localSet(species.begin(), species.end());
      if(localSet.size() == 1)
        speciesSet.insert(*localSet.begin());
      endpoints.clear();
    }

    BOOST_FOREACH(const ::std::string & s, speciesSet)
      endpoints.push_back(common::AtomsFormula(s));
    return endpoints;
  }

template< typename InputIterator>
  ConvexHull::ConvexHull(InputIterator first, InputIterator last) :
      myConvexProperty(common::structure_properties::general::ENTHALPY), myHullDims(
          last - first)
  {
    initEndpoints(generateEndpoints(first, last));
    addStructures(first, last);
  }

template< typename InputIterator>
  ::std::vector< ConvexHull::PointId>
  ConvexHull::addStructures(InputIterator first, InputIterator last)
  {
    ::std::vector< PointId> ids;
    for(InputIterator it = first; it != last; ++it)
      ids.push_back(addStructure(*it));
    return ids;
  }

}
}

#endif // SSLIB_USE_CGAL
#endif /* CONVEX_HULL_H */
