/*
 * SsLibFactoryYaml.cpp
 *
 *  Created on: Aug 18, 2011
 *      Author: Martin Uhrin
 */

// INCLUDES //////////////////////////////////
#include "spl/factory/SsLibFactoryYaml.h"

#include <memory>

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tokenizer.hpp>

#include "spl/build_cell/AtomsDescription.h"
#include "spl/build_cell/AtomsGroup.h"
#include "spl/build_cell/PointGroups.h"
#include "spl/build_cell/RandomUnitCellGenerator.h"
#include "spl/common/AtomSpeciesDatabase.h"
#include "spl/common/AtomSpeciesId.h"
#include "spl/factory/FactoryError.h"
#include "spl/io/ResReaderWriter.h"
#include "spl/potential/CastepGeomOptimiser.h"
#include "spl/potential/LennardJones.h"
#include "spl/potential/OptimisationSettings.h"
#include "spl/potential/TpsdGeomOptimiser.h"
#include "spl/potential/Types.h"
#include "spl/utility/IndexingEnums.h"
#include "spl/utility/SortedDistanceComparator.h"

// NAMESPACES ////////////////////////////////

namespace spl {
namespace factory {

namespace fs = boost::filesystem;

// Boost Tokenizer stuff
typedef boost::tokenizer< boost::char_separator< char> > Tok;
const boost::char_separator< char> tokSep(" \t");

Factory::Factory() :
    myShapeFactory()
{
}

build_cell::AtomsDescriptionPtr
Factory::createAtomsDescription(const builder::SimpleAtomsDataEntry & options,
    const io::AtomFormatParser< builder::SimpleAtomsData> & parser) const
{
  build_cell::AtomsDescriptionPtr atomsDescription;

  const boost::optional< AtomSpeciesCount> speciesCount = parser.getValue(
      &builder::SimpleAtomsData::species, options);
  if(!speciesCount)
    return atomsDescription;

  if(speciesCount->count.nullSpan() && speciesCount->count.lower() == 0)
    return atomsDescription;

  atomsDescription.reset(
      new build_cell::AtomsDescription(speciesCount->species,
          speciesCount->count));

  atomsDescription->radius = parser.getValue(&builder::SimpleAtomsData::radius,
      options);
  const boost::optional< arma::rowvec3> pos = parser.getValue(
      &builder::SimpleAtomsData::pos, options);
  if(pos)
    atomsDescription->position = pos->t();

  const boost::optional< std::string> label = parser.getValue(
      &builder::SimpleAtomsData::label, options);
  if(label)
    atomsDescription->label = *label;

  return atomsDescription;
}

build_cell::AtomsGroupPtr
Factory::createAtomsGroup(const builder::AtomsGroup & options,
    io::AtomFormatParser< builder::SimpleAtomsData> & parser) const
{
  build_cell::AtomsGroupPtr atomsGenerator(new build_cell::AtomsGroup());

  // Try creating a generator shape
  UniquePtr< build_cell::GeneratorShape>::Type genShape;
  if(options.genShape)
    myShapeFactory.createShape(genShape, *options.genShape);

  atomsGenerator->setNumReplicas(options.num);

  if(options.pos)
    atomsGenerator->setPosition(options.pos->t());
  else
    atomsGenerator->setTransformMode(
        atomsGenerator->getTransformMode()
            | build_cell::AtomsGroup::TransformMode::RAND_POS);

  if(options.rot)
    atomsGenerator->setRotation(options.rot->t());
  else
  {
    atomsGenerator->setTransformMode(
        atomsGenerator->getTransformMode()
            | build_cell::AtomsGroup::TransformMode::RAND_ROT_DIR
            | build_cell::AtomsGroup::TransformMode::RAND_ROT_ANGLE);
  }

  // Check if there is a 'global' radius
  if(options.atomsRadius)
    parser.setDefault(&builder::SimpleAtomsData::radius, options.atomsRadius);

  ///////////
  // Atoms
  BOOST_FOREACH(const builder::SimpleAtomsDataEntry & atomData, options.atoms)
  {
    build_cell::AtomsDescriptionPtr atomsDescription = createAtomsDescription(
        atomData, parser);
    if(atomsDescription.get())
      atomsGenerator->insertAtoms(*atomsDescription);
  }

  // Species distances
  if(options.pairDistances)
  {
    std::vector< std::string> species;
    BOOST_FOREACH(PairDistances::const_reference entry, *options.pairDistances)
    {
      boost::split(species, entry.first, boost::is_any_of("~"));
      if(species.size() == 2)
        atomsGenerator->addSpeciesPairDistance(
            build_cell::SpeciesPair(species[0], species[1]), entry.second);
    }
  }

  return atomsGenerator;
}

Factory::GeomOptimiserPtr
Factory::createGeometryOptimiser(const Optimiser & options) const
{
  GeomOptimiserPtr opt;

  if(options.tpsd)
  {
    potential::IPotentialPtr potential = createPotential(
        options.tpsd->potential);
    if(!potential.get())
      return opt;

    UniquePtr< potential::TpsdGeomOptimiser>::Type tpsd(
        new potential::TpsdGeomOptimiser(potential));

    tpsd->setMaxIterations(options.tpsd->maxIterations);
    tpsd->setEnergyTolerance(options.tpsd->energyTolerance);
    tpsd->setForceTolerance(options.tpsd->forceTolerance);

    opt = tpsd;
  }
  else if(options.castep)
  {
    // Read in the settings
    potential::CastepGeomOptimiseSettings settings;
    settings.keepIntermediateFiles = options.castep->keepIntermediates;
    settings.numRoughSteps = options.castep->numRoughSteps;
    settings.numConsistentRelaxations = options.castep->numSelfConsistent;

    opt.reset(
        new potential::CastepGeomOptimiser(options.castep->runCommand,
            options.castep->seed, settings));
  }

  return opt;
}

potential::OptimisationSettings
Factory::createOptimisationSettings(const OptimiserSettings & options) const
{
  potential::OptimisationSettings settings;

  settings.maxIter = options.maxIterations;
  settings.energyTol = options.energyTolerance;
  settings.forceTol = options.forceTolerance;
  settings.stressTol = options.stressTolerance;

  settings.pressure.reset(::arma::zeros(3, 3));
  settings.pressure->diag().fill(options.pressure);

  return settings;
}

potential::IPotentialPtr
Factory::createPotential(const Potential & options) const
{
  potential::IPotentialPtr pot;

  if(options.lj)
  {
    pot.reset(
        new potential::LennardJones(options.lj->species, options.lj->epsilon,
            options.lj->sigma, options.lj->cutoff, options.lj->beta,
            options.lj->powers(0), options.lj->powers(1),
            options.lj->combiningRule));
  }

  return pot;
}

build_cell::RandomUnitCellPtr
Factory::createRandomCellGenerator(
    const builder::UnitCellBuilder & options) const
{
  using namespace utility::cell_params_enum;

  build_cell::RandomUnitCellPtr cell(new build_cell::RandomUnitCellGenerator());

  if(options.vol)
    cell->setTargetVolume(*options.vol);

  if(options.mul)
    cell->setContentsMultiplier(*options.mul);

  if(options.delta)
    cell->setVolumeDelta(*options.delta);

  if(options.angles)
  {
    if(options.angles->min)
      cell->setMinAngles(*options.angles->min);
    if(options.angles->max)
      cell->setMaxAngles(*options.angles->max);
  }

  if(options.lengths)
  {
    if(options.lengths->min)
      cell->setMinLengths(*options.lengths->min);
    if(options.lengths->max)
      cell->setMaxLengths(*options.lengths->max);
    if(options.lengths->maxRatio)
      cell->setMaxLengthRatio(*options.lengths->maxRatio);
  }

  // Do this after settings the general min/max length/angles as this is a more specific
  // way of specifying the unit cell dimensions
  if(options.abc)
  {
    SSLIB_ASSERT(options.abc->size() == 6);
    for(size_t i = A; i <= GAMMA; ++i)
    {
      cell->setMin(i, (*options.abc)[i].lower());
      cell->setMax(i, (*options.abc)[i].upper());
    }
  }

  return cell;
}

build_cell::StructureBuilderPtr
Factory::createStructureBuilder(const builder::Builder & options) const
{
  build_cell::StructureBuilder::ConstructInfo constructInfo;

  if(options.overlap)
    constructInfo.atomsOverlap = *options.overlap;

  if(options.cluster)
    constructInfo.isCluster = *options.cluster;

  build_cell::StructureBuilderPtr builder(
      new build_cell::StructureBuilder(constructInfo));

  io::AtomFormatParser< builder::SimpleAtomsData> parser;
  if(options.atomsFormat)
    parser.setFormat(*options.atomsFormat);

  parser.setDefault(&builder::SimpleAtomsData::radius, options.atomsRadius);

  // Generators
  if(options.atoms)
  {
    build_cell::AtomsGroupPtr group(new build_cell::AtomsGroup());

    // Try creating a generator shape
    UniquePtr< build_cell::GeneratorShape>::Type genShape;
    if(options.genShape)
      myShapeFactory.createShape(genShape, *options.genShape);

    ///////////
    // Atoms
    BOOST_FOREACH(const builder::SimpleAtomsDataEntry & atomData, *options.atoms)
    {
      build_cell::AtomsDescriptionPtr atomsDescription = createAtomsDescription(
          atomData, parser);
      if(atomsDescription.get())
        group->insertAtoms(*atomsDescription);
    }

    // Species distances
    if(options.pairDistances)
    {
      std::vector< std::string> species;
      BOOST_FOREACH(PairDistances::const_reference entry, *options.pairDistances)
      {
        boost::split(species, entry.first, boost::is_any_of("~"));
        if(species.size() == 2)
          group->addSpeciesPairDistance(
              build_cell::SpeciesPair(species[0], species[1]), entry.second);
      }
    }

    builder->addGenerator(group);
  }

  if(options.groups)
  {
    BOOST_FOREACH(const builder::AtomsGroup & group, *options.groups)
    {
      build_cell::AtomsGroupPtr atomsGenerator = createAtomsGroup(group,
          parser);
      if(atomsGenerator.get())
        builder->addGenerator(atomsGenerator);
    }
  }

  // Unit cell
  if(options.unitCellBuilder)
  {
    build_cell::IUnitCellGeneratorPtr ucGen(
        createRandomCellGenerator(*options.unitCellBuilder));
    if(ucGen.get())
      builder->setUnitCellGenerator(ucGen);
  }

  // Symmetry
  if(options.symmetry)
  {
    if(options.symmetry->pointGroup)
    {
      build_cell::PointGroup group;
      if(build_cell::getPointGroup(group, *options.symmetry->pointGroup))
        builder->setPointGroup(group);
    }
  }

  return builder;
}

utility::IStructureComparatorPtr
Factory::createStructureComparator(const Comparator & options) const
{
  utility::IStructureComparatorPtr comparator;

  if(options.sortedDist)
  {
    utility::SortedDistanceComparator::ConstructionInfo constructInfo;

    constructInfo.tolerance = options.sortedDist->tolerance;
    constructInfo.volumeAgnostic = options.sortedDist->volumeAgnostic;
    constructInfo.usePrimitive = options.sortedDist->usePrimitive;

    comparator.reset(new utility::SortedDistanceComparator(constructInfo));
  }

  return comparator;
}

build_cell::IStructureGeneratorPtr
Factory::createStructureGenerator(
    const builder::StructureGenerator & options) const
{
  build_cell::IStructureGeneratorPtr generator;

  if(options.builder)
    generator = createStructureBuilder(*options.builder);

  return generator;
}

const GenShapeFactory &
Factory::getShapeFactory() const
{
  return myShapeFactory;
}

}
}

