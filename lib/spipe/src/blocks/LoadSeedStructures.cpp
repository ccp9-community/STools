/*
 * RandomStructure.cpp
 *
 *  Created on: Aug 18, 2011
 *      Author: Martin Uhrin
 */

// INCLUDES //////////////////////////////////
#include "blocks/LoadSeedStructures.h"

#include <sstream>
#include <vector>

#include <boost/foreach.hpp>

// From SSTbx
#include <common/AtomSpeciesDatabase.h>
#include <common/Constants.h>
#include <common/Structure.h>
#include <io/IoFunctions.h>
#include <io/ResourceLocator.h>
#include <utility/UtilFunctions.h>

// Local includes
#include "common/PipeFunctions.h"
#include "common/SharedData.h"
#include "common/StructureData.h"
#include "common/UtilityFunctions.h"

// NAMESPACES ////////////////////////////////


namespace spipe {
namespace blocks {

namespace fs = ::boost::filesystem;
namespace ssbc = ::sstbx::build_cell;
namespace ssc = ::sstbx::common;
namespace ssio = ::sstbx::io;
namespace ssu = ::sstbx::utility;

const double LoadSeedStructures::ATOMIC_VOLUME_MULTIPLIER = 2.0;

LoadSeedStructures::LoadSeedStructures(
  const ::std::string & seedStructures,
  const bool tryToScaleVolumes):
SpBlock("Load seed structures"),
mySeedStructuresString(seedStructures),
myTryToScaleVolumes(tryToScaleVolumes)
{}

void LoadSeedStructures::pipelineInitialising()
{
  myStructures.clear();

  // First of all split the string up
  ::std::string entry;
  std::stringstream stream(mySeedStructuresString);
  while(getline(stream, entry))
    processEntry(entry);
}

void LoadSeedStructures::start()
{
	using ::spipe::common::StructureData;

  double oldVolume, newVolume;
  const ssc::UnitCell * unitCell;
  BOOST_FOREACH(const ssc::Structure & str, myStructures)
  {
    StructureData & data = getRunner()->createData();
    // Make a clone of our structure
    ssc::Structure & structure = data.setStructure(str.clone());
    
    // Set up the structure name if needed
    if(structure.getName().empty())
      structure.setName(common::generateStructureName(getRunner()->memory()));

    unitCell = structure.getUnitCell();
    if(myTryToScaleVolumes && unitCell)
    {
      oldVolume = unitCell->getVolume();
      newVolume = ATOMIC_VOLUME_MULTIPLIER * getTotalAtomicVolume(str);
      structure.scale(newVolume / oldVolume);
    }

    // Send it on its way
    out(data);
  }
}

int LoadSeedStructures::processEntry(const ::std::string & entry)
{
  const EntryType type = entryType(entry);

  if(type == FILE_PATH || type == FOLDER_PATH)
  {
    ssio::ResourceLocator loc;
    loc.set(entry);
    return processFileOrFolder(loc);
  }
  else if(type == WILDCARD_PATH)
    return processWildcardEntry(entry);

  return -1;
}

int LoadSeedStructures::processWildcardEntry(const ::std::string & entry)
{
  ::std::vector<fs::path> entryPaths;

  if(!ssio::getWildcardPaths(entry, entryPaths))
    return -1;

  int numOut = 0;

  BOOST_FOREACH(const fs::path & entryPath, entryPaths)
  {
    if(fs::is_regular_file(entryPath))
      numOut += processFileOrFolder(entryPath);
  }

  return numOut;
}

int LoadSeedStructures::processFileOrFolder(const ::sstbx::io::ResourceLocator & loc)
{
  // Try loading the file
  const size_t numLoaded = getRunner()->memory().global().getStructureIo().readStructures(
    myStructures,
    loc,
    getRunner()->memory().global().getSpeciesDatabase()
  );
  if(numLoaded == 0)
    return -1;

  return static_cast<int>(numLoaded);
}

LoadSeedStructures::EntryType
LoadSeedStructures::entryType(const ::std::string & entry) const
{
  if(entry.find('*') != ::std::string::npos)
    return WILDCARD_PATH;

  ssio::ResourceLocator entryLocator;
  if(!entryLocator.set(entry))
    return UNKNOWN;

  if(fs::exists(entryLocator.path()))
  {
    if(fs::is_regular_file(entryLocator.path()))
      return FILE_PATH;
    else if(fs::is_directory(entryLocator.path()))
      return FOLDER_PATH;
  }

  return UNKNOWN;
}

double LoadSeedStructures::getTotalAtomicVolume(
  const ::sstbx::common::Structure & structure) const
{
  typedef ::boost::optional<double> OptionalDouble;

  ::std::vector<ssc::AtomSpeciesId::Value> species;
  structure.getAtomSpecies(species);

  OptionalDouble radius;
  double dRadius, volume = 0.0;
  const ssc::AtomSpeciesDatabase & speciesDb = getRunner()->memory().global().getSpeciesDatabase();
  BOOST_FOREACH(ssc::AtomSpeciesId::Value spec, species)
  {
    radius = speciesDb.getRadius(spec);
    if(radius)
    {
      dRadius = *radius;
      volume += dRadius * dRadius * dRadius;
    }
  }

  volume *= ssc::constants::FOUR_THIRDS * ssc::constants::PI;
  return volume;
}

}
}
