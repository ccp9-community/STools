/*
 * Factory.cpp
 *
 *  Created on: Aug 18, 2011
 *      Author: Martin Uhrin
 */

// INCLUDES //////////////////////////////////
#include "factory/PipeFactory.h"

#include <spl/factory/SsLibElements.h>
#include <spl/math/Random.h>
#include <spl/potential/OptimisationSettings.h>
#include <spl/utility/UtilityFwd.h>

// From SPipe
#include <blocks/KeepTopN.h>

// Local includes
#include "factory/MapEntries.h"

// NAMESPACES ////////////////////////////////

namespace stools {
namespace factory {

// Alias for accessing keywords namespace
namespace sp = ::spipe;
namespace spb = sp::blocks;
namespace spf = sp::factory;

namespace ssm = ::spl::math;
namespace ssf = ::spl::factory;
namespace ssp = ::spl::potential;

const PipeFactory::BlockHandle PipeFactory::NULL_HANDLE;

PipeFactory::BlockHandle
PipeFactory::createBuildPipe(const OptionsMap & options) const
{
  const OptionsMap * const buildStructuresOptions = options.find(
      spf::BUILD_STRUCTURES);
  if(!buildStructuresOptions)
    return NULL_HANDLE;

  BlockHandle startBlock;
  if(!myBlockFactory.createBuildStructuresBlock(&startBlock,
      *buildStructuresOptions))
    return NULL_HANDLE;

  // Keep track of the last block so we can connect everything up
  BlockHandle block, lastBlock = startBlock;

  const OptionsMap * const outputOptions = options.find(spf::WRITE_STRUCTURES);
  if(outputOptions)
  {
    if(myBlockFactory.createWriteStructuresBlock(&block, *outputOptions))
      lastBlock = lastBlock->connect(block);
    else
      return NULL_HANDLE;
  }

  return startBlock;
}

PipeFactory::BlockHandle
PipeFactory::createSearchPipe(const OptionsMap & options) const
{
  const OptionsMap * const buildStructuresOptions = options.find(
      spf::BUILD_STRUCTURES);
  const ::std::string * const loadStructures = options.find(
      spf::LOAD_STRUCTURES);
  if(!buildStructuresOptions && !loadStructures)
    return NULL_HANDLE;

  BlockHandle startBlock;
  if(buildStructuresOptions)
  {
    if(!myBlockFactory.createBuildStructuresBlock(&startBlock,
        *buildStructuresOptions))
      return NULL_HANDLE;
  }
  else if(loadStructures)
  {
    if(!myBlockFactory.createLoadStructuresBlock(&startBlock, *loadStructures))
      return NULL_HANDLE;
  }
  else
    return NULL_HANDLE;

  // Keep track of the last block so we can connect everything up
  BlockHandle block, lastBlock = startBlock;

  {
    const OptionsMap * const cutAndPasteOptions = options.find(
        spf::CUT_AND_PASTE);
    if(cutAndPasteOptions
        && myBlockFactory.createCutAndPasteBlock(&block, *cutAndPasteOptions))
      lastBlock = lastBlock->connect(block);
  }

  {
    const OptionsMap * const preGeomOptimiseOptions = options.find(
        spf::PRE_GEOM_OPTIMISE);
    if(preGeomOptimiseOptions)
    {
      if(myBlockFactory.createGeomOptimiseBlock(&block, *preGeomOptimiseOptions))
        lastBlock = lastBlock->connect(block);
      else
        return NULL_HANDLE;
    }
  }

  const OptionsMap * const geomOptimiseOptions = options.find(
      spf::GEOM_OPTIMISE);
  if(geomOptimiseOptions)
  {
    if(myBlockFactory.createGeomOptimiseBlock(&block, *geomOptimiseOptions))
      lastBlock = lastBlock->connect(block);
    else
      return NULL_HANDLE;
  }

  const OptionsMap * const removeDuplicatesOptions = options.find(
      spf::REMOVE_DUPLICATES);
  if(removeDuplicatesOptions)
  {
    if(myBlockFactory.createRemoveDuplicatesBlock(&block,
        *removeDuplicatesOptions))
      lastBlock = lastBlock->connect(block);
    else
      return NULL_HANDLE;
  }

  const OptionsMap * const keepWithinXPercentOptions = options.find(
      spf::KEEP_WITHIN_X_PERCENT);
  if(keepWithinXPercentOptions)
  {
    if(myBlockFactory.createKeepWithinXPercentBlock(&block,
        *keepWithinXPercentOptions))
      lastBlock = lastBlock->connect(block);
    else
      return NULL_HANDLE;
  }

  const OptionsMap * const keepTopNOptions = options.find(spf::KEEP_TOP_N);
  if(keepTopNOptions)
  {
    if(myBlockFactory.createKeepTopNBlock(&block, *keepTopNOptions))
      lastBlock = lastBlock->connect(block);
    else
      return NULL_HANDLE;
  }

  // Find out what the symmetry group is
  const OptionsMap * const findSymmetryOptions = options.find(
      spf::FIND_SYMMETRY_GROUP);
  if(findSymmetryOptions)
  {
    if(myBlockFactory.createFindSymmetryGroupBlock(&block,
        *findSymmetryOptions))
      lastBlock = lastBlock->connect(block);
    else
      return NULL_HANDLE;
  }

  const OptionsMap * const writeStructuresOptions = options.find(
      spf::WRITE_STRUCTURES);
  if(writeStructuresOptions)
  {
    if(myBlockFactory.createWriteStructuresBlock(&block,
        *writeStructuresOptions))
      lastBlock = lastBlock->connect(block);
  }

  // Finally tack on a lowest energy block to make sure that only one structure
  // comes out the end in all eventualities
  lastBlock = lastBlock->connect(BlockHandle(new spb::KeepTopN(1)));

  return startBlock;
}

PipeFactory::BlockHandle
PipeFactory::createSearchPipeExtended(const OptionsMap & options) const
{
  // Create a search pipe
  BlockHandle startBlock = createSearchPipe(options);
  if(!startBlock)
    return NULL_HANDLE;

  // Are we doing a stoichiometry search?
  const OptionsMap * const searchStoichiometries = options.find(
      spf::SEARCH_STOICHIOMETRIES);
  if(searchStoichiometries)
  {
    BlockHandle searchStoichsBlock;
    if(!myBlockFactory.createSearchStoichiometriesBlock(&searchStoichsBlock,
        *searchStoichiometries, startBlock))
      return BlockHandle();
    startBlock = searchStoichsBlock;

#ifdef SSLIB_USE_CGAL
    const OptionsMap * const keepStableCompositions = options.find(
        spf::KEEP_STABLE_COMPOSITIONS);
    if(keepStableCompositions)
    {
      BlockHandle block;
      if(myBlockFactory.createKeepStableCompositionsBlock(&block,
          *keepStableCompositions))
        startBlock->connect(block);
    }
#endif
  }

  // Are we doing a parameter sweep
  const OptionsMap * const paramSweepOptions = options.find(
      spf::SWEEP_POTENTIAL_PARAMS);
  if(paramSweepOptions)
  {
    BlockHandle sweepStartBlock;
    if(!myBlockFactory.createSweepPotentialParamsBlock(&sweepStartBlock,
        *paramSweepOptions, startBlock))
      return NULL_HANDLE;

    return sweepStartBlock;
  }

  const OptionsMap * const runParamsQueueOptions = options.find(
      spf::RUN_POTENTIAL_PARAMS_QUEUE);
  if(runParamsQueueOptions)
  {
    BlockHandle runQueueBlock;
    if(!myBlockFactory.createRunPotentialParamsQueueBlock(&runQueueBlock,
        *runParamsQueueOptions, startBlock))
      return BlockHandle();

    return runQueueBlock;
  }

  return startBlock;
}

} // namespace stools
} // namespace factory