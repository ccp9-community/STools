/*
 * WriteStructure.cpp
 *
 *  Created on: Aug 18, 2011
 *      Author: Martin Uhrin
 */

// INCLUDES //////////////////////////////////
#include "blocks/WriteStructure.h"

// From SSTbx
#include <common/Structure.h>
#include <io/StructureReadWriteManager.h>
#include <io/BoostFilesystem.h>

// From local
#include "common/SharedData.h"
#include "common/StructureData.h"
#include "common/UtilityFunctions.h"


// NAMESPACES ////////////////////////////////


namespace spipe {
namespace blocks {

namespace fs = ::boost::filesystem;
namespace ssc = ::sstbx::common;
namespace ssio = ::sstbx::io;
namespace ssu = ::sstbx::utility;

WriteStructure::WriteStructure():
SpBlock("Write structures")
{}

void WriteStructure::in(::spipe::common::StructureData & data)
{
  ssio::StructureReadWriteManager & rwMan = getRunner()->memory().global().getStructureIo();
  ssc::Structure * const structure = data.getStructure();

	// Check if the structure has a name already, otherwise give it one
	if(structure->getName().empty())
	{
		structure->setName(::spipe::common::generateUniqueName());
	}

	// Create the path to store the structure
	fs::path p(structure->getName() + ".res");

  // Prepend the pipe output path
  p = getRunner()->memory().shared().getOutputPath(*getRunner()) / p;
	
  if(!rwMan.writeStructure(*data.getStructure(), p, getRunner()->memory().global().getSpeciesDatabase()))
  {
    // TODO: Produce error
  }

	out(data);
}

}
}

