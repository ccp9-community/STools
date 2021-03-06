/*
 * SharedData.h
 *
 *
 *  Created on: Aug 17, 2011
 *      Author: Martin Uhrin
 */

#ifndef SHARED_DATA_H
#define SHARED_DATA_H

// INCLUDES /////////////////////////////////////////////
#include "StructurePipe.h"

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include <armadillo>

// From SSTbx
#include <build_cell/IStructureGenerator.h>
#include <build_cell/BuildCellFwd.h>
#include <io/BoostFilesystem.h>

// Local includes
#include "PipeLibTypes.h"
#include "utility/DataTable.h"
#include "common/CommonData.h"

// FORWARD DECLARATIONS ////////////////////////////////////

namespace spipe {
namespace common {

class SharedData
{
public:

  typedef ::sstbx::build_cell::IStructureGeneratorPtr IStructureGeneratorPtr;

  static const char DIR_SUBSTRING_DELIMITER[];

  SharedData();

  bool appendToOutputDirName(const ::std::string & toAppend);

  /**
  /* Get the output path for the pipeline that owns this shared data relative to
  /* the working directory where the code was executed.
  /**/
  ::boost::filesystem::path getOutputPath(const SpRunner & runner) const;

  /**
  /* Get the output path for the pipeline that owns this shared data relative to
  /* the working directory where the code was executed.
  /**/
  ::boost::filesystem::path getOutputPath(const SpRunnerAccess & runner) const;

  /**
  /* Get the output path for the pipeline that owns this shared data relative to
  /* the parent pipeline (or global data output path if there is no parent).
  /**/
  const ::boost::filesystem::path & getPipeRelativeOutputPath() const;

  const ::std::string & getInstanceName() const;

  ::sstbx::build_cell::IStructureGenerator * getStructureGenerator();
  const ::sstbx::build_cell::IStructureGenerator * getStructureGenerator() const;
  template <class T>
  void setStructureGenerator(SSLIB_UNIQUE_PTR(T) generator);

  ::sstbx::utility::HeterogeneousMap  objectsStore;

private:
  void reset();

  void buildOutputPathRecursive(::boost::filesystem::path & path, const SpRunner & runner) const;
  void buildOutputPathRecursive(::boost::filesystem::path & path, const SpRunnerAccess & runner) const;

  IStructureGeneratorPtr myStructureGenerator;
  ::boost::filesystem::path myOutputDir;
  ::std::string  myInstanceName;

};

template <class T>
void SharedData::setStructureGenerator(SSLIB_UNIQUE_PTR(T) generator)
{
  myStructureGenerator = generator;
}

}
}

#endif /* SHARED_DATA_H */
