/*
 * ConvexHull.cpp
 *
 *  Created on: Nov 10, 2011
 *      Author: Martin Uhrin
 */

#include "analysis/ConvexHull.h"

#ifdef SSLIB_USE_CGAL

#include <boost/foreach.hpp>

#include "common/StructureProperties.h"

//#define DEBUG_CONVEX_HULL_GENERATOR

namespace sstbx {
namespace analysis {

const int ConvexHull::CONVEX_PROPERTY_DIMENSION = 0;

ConvexHull::ConvexHull(const EndpointLabels & labels):
    myConvexProperty(common::structure_properties::general::ENTHALPY),
    myHullDims(labels.size())
{
  SSLIB_ASSERT_MSG(labels.size() >= 2, "Need at least two endpoints to make convex hull.");
  initEndpoints(labels);
}

ConvexHull::ConvexHull(const EndpointLabels & labels, utility::Key<double> & convexProperty):
    myConvexProperty(convexProperty), myHullDims(labels.size())
{
  SSLIB_ASSERT_MSG(labels.size() >= 2, "Need at least two endpoints to make convex hull.");
  initEndpoints(labels);
}

ConvexHull::PointId ConvexHull::addStructure(const common::Structure & structure)
{
  return generateEntry(structure);
}

int ConvexHull::dims() const
{
  return myHullDims;
}

const ConvexHull::Hull * ConvexHull::getHull() const
{
  if(!myHull.get() && canGenerate())
    generateHull();

  return myHull.get();
}

ConvexHull::VectorD ConvexHull::composition(const VectorD & vec) const
{
  return VectorD(myHullDims - 1, vec.cartesian_begin() + 1, vec.cartesian_end());
}

ConvexHull::PointD ConvexHull::composition(const PointD & point) const
{
  return PointD(myHullDims - 1, point.cartesian_begin() + 1, point.cartesian_end());
}

ConvexHull::EndpointsConstIterator ConvexHull::endpointsBegin() const
{
  return myEndpoints.begin();
}

ConvexHull::EndpointsConstIterator ConvexHull::endpointsEnd() const
{
  return myEndpoints.end();
}

ConvexHull::HullEntry::HullEntry(const common::Structure::Composition & composition, const HullTraits::FT value):
    myComposition(composition), myValue(value)
{
  myIsEndpoint = true;
  bool foundNonZero = false;
  BOOST_FOREACH(common::Structure::Composition::const_reference x, myComposition)
  {
    if(!foundNonZero && x.second != 0)
      foundNonZero = true;
    else if(foundNonZero && x.second != 0)
    {
      myIsEndpoint = false;
      break;
    }
  }
}

const common::Structure::Composition & ConvexHull::HullEntry::getComposition() const
{
  return myComposition;
}

const ConvexHull::HullTraits::FT ConvexHull::HullEntry::getValue() const
{
  return myValue;
}

bool ConvexHull::HullEntry::isEndpoint() const
{
  return myIsEndpoint;
}

ConvexHull::PointId
ConvexHull::generateEntry(const common::Structure & structure)
{
  PointId id = -1;

  if(structure.getNumAtoms() == 0)
    return id;

  // Check if the structure has a value for the property that will form the 'depth' of the hull
  const double * const value = structure.getProperty(myConvexProperty);
  if(!value)
    return id;

  id = myEntries.size();
  const HullEntries::iterator it = myEntries.insert(myEntries.end(), HullEntry(structure.getComposition(), *value));
  if(it->isEndpoint())
  {
    ::std::string nonZeroElement;
    BOOST_FOREACH(common::Structure::Composition::const_reference x, it->getComposition())
    {
      if(x.second != 0)
      {
        nonZeroElement = x.first;
        break;
      }
    }
    updateChemicalPotential(nonZeroElement, *value / static_cast<double>(structure.getNumAtoms()));
  }

  return id;
}

void ConvexHull::updateChemicalPotential(const ::std::string & endpointSpecies, const HullTraits::FT value)
{
  ChemicalPotentials::iterator it = myChemicalPotentials.find(endpointSpecies);
  if(it == myChemicalPotentials.end())
    myChemicalPotentials[endpointSpecies] = value;
  else
  {
    if(it->second > value)
    {
      it->second = value;
      myHull.reset();   // Hull needs to be re-generated
    }
  }
}

ConvexHull::PointD ConvexHull::generateHullPoint(const HullEntry & entry) const
{
  // Need chemical potentials for all endpoints
  SSLIB_ASSERT(canGenerate());

  ::std::vector<HullTraits::FT> hullCoords(myHullDims);

  const common::Structure::Composition & composition = entry.getComposition();
  common::Structure::Composition::const_iterator it;

  int totalAtoms = 0;
  HullTraits::FT totalMuNAtoms = 0.0;
  BOOST_FOREACH(Endpoints::const_reference endpoint, myEndpoints)
  {
    it = composition.find(endpoint.first);
    if(it != composition.end())
    {
      totalAtoms += it->second;
      totalMuNAtoms += myChemicalPotentials.find(it->first)->second * HullTraits::FT(it->second);
    }
  }

  // Create a vector that is the weighted sum of the vectors of composition
  // simplex
  VectorD v(myHullDims - 1);
  for(int i = 0; i < myEndpoints.size(); ++i)
  {
    it = composition.find(myEndpoints[i].first);
    if(it != composition.end())
    {
      VectorD scaled = myEndpoints[i].second;
      scaled *= HullTraits::FT(it->second, totalAtoms);
#ifdef DEBUG_CONVEX_HULL_GENERATOR
      ::std::cout << "Adding endpoint: " << scaled << " weight: " << HullTraits::FT(it->second, totalAtoms) << ::std::endl;
#endif
      v += scaled;
    }
  }
  for(int i = 0; i < myHullDims - 1; ++i)
    hullCoords[i + 1] = v[i];


  // The first hull coordinate is always the 'convex property', usually the energy
  hullCoords[0] = (entry.getValue() - totalMuNAtoms) / totalAtoms;

  PointD point(myHullDims, hullCoords.begin(), hullCoords.end());

#ifdef DEBUG_CONVEX_HULL_GENERATOR
  ::std::cout << "Hull entry: " << point << ::std::endl;
#endif

  return point;
}

void ConvexHull::generateHull() const
{
  SSLIB_ASSERT(canGenerate());

  myHull.reset(new Hull(myHullDims));
  BOOST_FOREACH(HullEntries::const_reference entry, myEntries)
  {
    myHull->insert(generateHullPoint(entry));
  }
}

bool ConvexHull::canGenerate() const
{
  return myChemicalPotentials.size() == myEndpoints.size();
}

void ConvexHull::initEndpoints(const EndpointLabels & labels)
{
  for(int i = 0; i < labels.size(); ++i)
  {
    myEndpoints.push_back(::std::make_pair(labels[i], VectorD(myHullDims - 1)));
  }

  ::std::vector<RT> vec(myEndpoints.size() - 1, 0);

  // Always start the hull with the first points at (0,0) and (1,0)
  vec[0] = 1.0;
  myEndpoints[1].second = VectorD(vec.size(), vec.begin(), vec.end());

#ifdef DEBUG_CONVEX_HULL_GENERATOR
  ::std::cout << "Convex hull building simplex\n";
  ::std::cout << "0: " << mySimplexCoordinates[0] << ::std::endl;
  ::std::cout << "1: " << mySimplexCoordinates[1] << ::std::endl;
#endif

  VectorD pSum = myEndpoints[1].second;
  vec.assign(vec.size(), 0.0);
  for(int i = 2; i < myHullDims; ++i)
  {
    // Put the new point at the centre of the previous points
    // and 'raise' the last coordinate into the new dimension
    // such as to make the new point unit distance from all the other points
    vec[i - 1] = 1.0;
    for(int j = 0; j < i - 1; ++j)
    {
      vec[j] = pSum[j] / i;
      vec[i - 1] -= vec[j] * vec[j];
    }
    vec[i - 1] = CGAL::sqrt(vec[i - 1]);
    myEndpoints[i].second = VectorD(vec.size(), vec.begin(), vec.end());

#ifdef DEBUG_CONVEX_HULL_GENERATOR
  ::std::cout << i << ": " << CGAL::to_double(mySimplexCoordinates[i][0]) << " " << CGAL::to_double(mySimplexCoordinates[i][1]) << ::std::endl;
#endif

    pSum += myEndpoints[i].second;
  }
#ifdef DEBUG_CONVEX_HULL_GENERATOR
  ::std::cout << "Convex hull finished building simplex\n";
#endif
}

}
}

#endif // SSLIB_USE_CGAL