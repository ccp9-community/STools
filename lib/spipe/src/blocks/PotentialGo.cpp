/*
 * PotentialGo.cpp
 *
 *  Created on: Aug 18, 2011
 *      Author: Martin Uhrin
 */

// INCLUDES //////////////////////////////////
#include "PotentialGo.h"

#include <iostream>
#include <locale>
#include <set>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tokenizer.hpp>

// From SSTbx
#include <build_cell/AtomsDescription.h>
#include <common/Structure.h>
#include <potential/PotentialData.h>
#include <potential/IGeomOptimiser.h>
#include <potential/IPotential.h>

#include "common/PipeFunctions.h"
#include "common/StructureData.h"
#include "common/SharedData.h"
#include "common/UtilityFunctions.h"

// NAMESPACES ////////////////////////////////


namespace spipe {
namespace blocks {

namespace ssbc = ::sstbx::build_cell;
namespace ssc = ::sstbx::common;
namespace ssp = ::sstbx::potential;
namespace structure_properties = ssc::structure_properties;

PotentialGo::PotentialGo(
  sstbx::potential::IGeomOptimiserPtr optimiser,
  const bool writeOutput):
SpBlock("Potential geometry optimisation"),
myOptimiser(optimiser),
myWriteOutput(writeOutput),
myOptimisationParams()
{}

PotentialGo::PotentialGo(
	sstbx::potential::IGeomOptimiserPtr optimiser,
  const ::sstbx::potential::OptimisationSettings & optimisationParams,
  const bool writeOutput):
SpBlock("Potential geometry optimisation"),
myOptimiser(optimiser),
myWriteOutput(writeOutput),
myOptimisationParams(optimisationParams)
{}

void PotentialGo::pipelineInitialising()
{
  if(myWriteOutput)
    myTableSupport.setFilename(common::getOutputFileStem(getRunner()->memory()) + ".geomopt");
  myTableSupport.registerRunner(*getRunner());
}

void PotentialGo::in(spipe::common::StructureData & data)
{
  ssc::Structure * const structure = data.getStructure();
  const ssp::OptimisationOutcome outcome = myOptimiser->optimise(*structure, myOptimisationParams);
	if(outcome.isSuccess())
  {
    // Update our data table with the structure data
    updateTable(*structure);

	  out(data);
  }
  else
  {
    ::std::cerr << "Optimisation failed: " << outcome.getMessage() << ::std::endl;
    // The structure failed to geometry optimise properly so drop it
    getRunner()->dropData(data);
  }
}

ssp::IGeomOptimiser & PotentialGo::getOptimiser()
{
  return *myOptimiser;
}

::spipe::utility::DataTableSupport & PotentialGo::getTableSupport()
{
  return myTableSupport;
}

void PotentialGo::updateTable(const ssc::Structure & structure)
{
  utility::DataTable & table = myTableSupport.getTable();
  const ::std::string & strName = structure.getName();

  const double * const internalEnergy = structure.getProperty(structure_properties::general::ENERGY_INTERNAL);
  if(internalEnergy)
  {
    table.insert(strName, "energy", common::getString(*internalEnergy));
    table.insert(strName, "energy/atom", common::getString(*internalEnergy / structure.getNumAtoms()));
  }
}

}
}

