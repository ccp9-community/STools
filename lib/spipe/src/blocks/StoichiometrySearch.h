/*
 * StoichiometrySearch.h
 *
 *
 *  Created on: May 4, 2012
 *      Author: Martin Uhrin
 */

#ifndef STOICHIOMETRY_SEARCH_H
#define STOICHIOMETRY_SEARCH_H

// INCLUDES /////////////////////////////////////////////
#include "StructurePipe.h"

#include <vector>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <pipelib/pipelib.h>

// From SSTbx
#include <build_cell/StructureBuilder.h>
#include <build_cell/RandomUnitCellGenerator.h>
#include <build_cell/BuildCellFwd.h>
#include <common/AtomSpeciesId.h>
#include <io/BoostFilesystem.h>
#include <utility/MultiIdxRange.h>

// Local includes
#include "SpTypes.h"
#include "utility/DataTable.h"
#include "utility/DataTableSupport.h"

// FORWARD DECLARATIONS ////////////////////////////////////
namespace sstbx {
namespace build_cell {
class AddOnStructureBuilder;
}
namespace common {
class AtomSpeciesDatabase;
}
}

namespace spipe {
namespace common {
class DataTableWriter;
}

namespace blocks {

struct SpeciesParameter
{
  SpeciesParameter(
    const ::sstbx::common::AtomSpeciesId::Value _id,
    const size_t _maxNum):
    id(_id),
    maxNum(_maxNum)
  {}

  ::sstbx::common::AtomSpeciesId::Value id;
  size_t                                maxNum;
};

class StoichiometrySearch : public SpStartBlock, public SpFinishedSink,
  ::boost::noncopyable
{
public:
  typedef ::sstbx::build_cell::StructureBuilderPtr StructureBuilderPtr;
  typedef ::std::vector<SpeciesParameter> SpeciesParameters;
  typedef ::sstbx::UniquePtr< ::spipe::SpPipe>::Type SubpipePtr;

  StoichiometrySearch(
    const ::sstbx::common::AtomSpeciesId::Value  species1,
    const ::sstbx::common::AtomSpeciesId::Value  species2,
    const size_t          maxAtoms,
    SubpipePtr            subpipe,
    StructureBuilderPtr   structureBuilder = StructureBuilderPtr()
  );

  StoichiometrySearch(
    const SpeciesParameters & speciesParameters,
    const size_t       maxAtoms,
    const double       atomsRadius,
    SubpipePtr         subpipe,
    StructureBuilderPtr structureBuilder = StructureBuilderPtr()
  );

  // From Block ////////
  virtual void pipelineInitialising();
  virtual void pipelineStarting();
  // End from Block ////

  // From StartBlock ///
  virtual void start();
  // End from StartBlock ///

  // From IDataSink /////////////////////////////
  virtual void finished(SpStructureDataPtr data);
  // End from IDataSink /////////////////////////

private:
  typedef ::spipe::StructureDataType                                        StructureDataTyp;
  typedef ::boost::scoped_ptr< ::spipe::utility::DataTableWriter>           TableWriterPtr;
  typedef ::pipelib::PipeRunner<StructureDataTyp, SharedDataType, SharedDataType> RunnerType;

  // From Block ////////
  virtual void runnerAttached(RunnerSetupType & setup);
  // End from Block ////

  ::sstbx::utility::MultiIdxRange<unsigned int> getStoichRange();

  void releaseBufferedStructures(
    const utility::DataTable::Key &             key
  );

  void updateTable(
    const utility::DataTable::Key &             key,
    const ::sstbx::utility::MultiIdx<unsigned int> & currentIdx,
    const ::sstbx::common::AtomSpeciesDatabase & atomsDb
  );

  StructureBuilderPtr newStructureGenerator() const;

  SubpipePtr mySubpipe;
  SpChildRunnerPtr mySubpipeRunner;

  // Use this to write out our table data
  ::spipe::utility::DataTableSupport    myTableSupport;
  const size_t                          myMaxAtoms;
  ::boost::filesystem::path             myOutputPath;

	/** Buffer to store structure that have finished their path through the sub pipeline. */
	::std::vector<StructureDataTyp *>		  myBuffer;

  SpeciesParameters                     mySpeciesParameters;
  StructureBuilderPtr                   myStructureGenerator;
};

}
}

#endif /* STOICHIOMETRY_SEARCH_H */
