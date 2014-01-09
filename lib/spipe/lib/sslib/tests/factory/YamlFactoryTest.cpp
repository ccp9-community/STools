/*
 * YamlFactoryTest.cpp
 *
 *  Created on: Oct 3, 2012
 *      Author: Martin Uhrin
 */

// INCLUDES //////////////////////////////////
#include "sslibtest.h"

#include <iostream>

#include <boost/exception/diagnostic_information.hpp>

#include <spl/build_cell/BuildCellFwd.h>
#include <spl/common/AtomSpeciesDatabase.h>
#include <spl/factory/FactoryError.h>
#include <spl/factory/SsLibFactoryYaml.h>
#include <spl/factory/SsLibYamlKeywords.h>
#include <spl/factory/SsLibYamlSchema.h>

using namespace ::spl;

namespace ssbc = ::spl::build_cell;
namespace ssc  = ::spl::common;
namespace splf = ::spl::factory;
namespace ssu = ::spl::utility;

struct StructureGenerator
{
  ::boost::optional<factory::builder::Builder> builder;
};

SCHEMER_MAP(StructureGeneratorSchema, StructureGenerator)
{
  element("builder", &StructureGenerator::builder);
}

BOOST_AUTO_TEST_CASE(StructureGeneratorTest)
{
  //// Settings ////////////////
  const char simpleStructure[] = "RandomStructure.sslib";
  const ::std::string randStrNode = "randomStructure";

  ssc::AtomSpeciesDatabase speciesDb;

  splf::Factory factory(speciesDb);

  const YAML::Node loadedNode = YAML::LoadFile(simpleStructure);

  StructureGeneratorSchema schema;
  StructureGenerator generator;
  schemer::ParseLog log;
  schema.nodeToValue(loadedNode, &generator, &log);
  log.printErrors();

  // TODO: Re-add the following
//  try
//  {
//    ssbc::IStructureGeneratorPtr strGen = factory.createStructureGenerator(builderMap);
//  }
//  catch(const splf::FactoryError & e)
//  {
//    ::std::cout << ::boost::diagnostic_information(e) << ::std::endl;
//  }
}
