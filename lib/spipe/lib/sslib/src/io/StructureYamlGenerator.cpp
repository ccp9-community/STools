/*
 * StructureYamlGenerator.cpp
 *
 *  Created on: Aug 18, 2011
 *      Author: Martin Uhrin
 */

// INCLUDES //////////////////////////////////
#include "io/StructureYamlGenerator.h"

#include <sstream>

#include <boost/foreach.hpp>

#include <armadillo>

#include "common/Atom.h"
#include "common/AtomSpeciesDatabase.h"
#include "common/Structure.h"
#include "common/UnitCell.h"
#include "factory/SsLibYamlKeywords.h"
#include "io/IoFunctions.h"
#include "utility/IndexingEnums.h"
#include "yaml/Transcode.h"

// DEFINES /////////////////////////////////


// NAMESPACES ////////////////////////////////


namespace sstbx {
namespace io {

namespace kw = factory::sslib_yaml_keywords;
namespace structure_properties = common::structure_properties;

StructureYamlGenerator::StructureYamlGenerator(const common::AtomSpeciesDatabase & speciesDb):
mySpeciesDb(speciesDb)
{
  AtomsFormat format;
  const YAML::Node null;
  format.push_back(FormatEntry(kw::STRUCTURE__ATOMS__SPEC, null));
  format.push_back(FormatEntry(kw::STRUCTURE__ATOMS__POS, null));
  myAtomInfoParser.setFormat(format);
}

StructureYamlGenerator::StructureYamlGenerator(
  const common::AtomSpeciesDatabase & speciesDb,
  const AtomYamlFormatParser::AtomsFormat & format):
mySpeciesDb(speciesDb),
myAtomInfoParser(format)
{}

YAML::Node
StructureYamlGenerator::generateNode(const ::sstbx::common::Structure & structure) const
{
  using namespace utility::cell_params_enum;

  YAML::Node root;

  // Name
  if(!structure.getName().empty())
    root[kw::STRUCTURE__NAME] = structure.getName();
  
  // Unit cell
  const common::UnitCell * const cell = structure.getUnitCell();
  if(cell)
    root[kw::STRUCTURE__CELL] = *cell;

  // Atoms
  for(size_t i = 0 ; i < structure.getNumAtoms(); ++i)
    root[kw::STRUCTURE__ATOMS].push_back(generateNode(structure.getAtom(i)));

  // Properties
  BOOST_FOREACH(const StructureProperty & property, structure_properties::VISIBLE_PROPERTIES)
    addProperty(root[kw::STRUCTURE__PROPERTIES], structure, property);
  

  return root;
}

common::types::StructurePtr
StructureYamlGenerator::generateStructure(const YAML::Node & node) const
{
  typedef AtomYamlFormatParser::AtomInfo::iterator AtomInfoIterator;

  common::types::StructurePtr structure(new common::Structure());

  bool valid = true;

  if(node[kw::STRUCTURE__NAME])
    structure->setName(node[kw::STRUCTURE__NAME].as< ::std::string>());
  
  if(node[kw::STRUCTURE__CELL])
  {
    common::types::UnitCellPtr cell(new common::UnitCell());
    *cell = node[kw::STRUCTURE__CELL].as<common::UnitCell>();
    structure->setUnitCell(cell);
  }

  if(node[kw::STRUCTURE__ATOMS] && node[kw::STRUCTURE__ATOMS].IsSequence())
  {
    AtomYamlFormatParser::AtomInfo atomInfo;
    AtomInfoIterator it;
    common::AtomSpeciesId::Value species;
    ::arma::vec3 pos;
    BOOST_FOREACH(const YAML::Node & atomNode, node[kw::STRUCTURE__ATOMS])
    {
      if(myAtomInfoParser.parse(atomInfo, atomNode))
      {
        const AtomInfoIterator end = atomInfo.end();

        it = atomInfo.find(kw::STRUCTURE__ATOMS__SPEC);
        if(it != end)
          species = mySpeciesDb.getIdFromSymbol(it->second.as< ::std::string>());

        if(species != common::AtomSpeciesId::DUMMY)
        {
          common::Atom & atom = structure->newAtom(species);
          it = atomInfo.find(kw::STRUCTURE__ATOMS__POS);
          if(it != end)
          {
            pos = it->second.as< ::arma::vec3>();
            atom.setPosition(pos);
          }
        }

        atomInfo.clear(); // Clear so we can use next time around
      }
      else
      {
        // TODO: Emit error
      }
    }
  }

  if(node[kw::STRUCTURE__PROPERTIES])
    praseProperties(*structure, node[kw::STRUCTURE__PROPERTIES]);

  return structure;
}

YAML::Node StructureYamlGenerator::generateNode(
  const ::sstbx::common::Atom & atom) const
{
  using namespace utility::cart_coords_enum;

  AtomYamlFormatParser::AtomInfo atomInfo;

  BOOST_FOREACH(const FormatEntry & entry, myAtomInfoParser.getFormat())
  {
    if(entry.first == kw::STRUCTURE__ATOMS__SPEC)
    {
      const ::std::string * const species = mySpeciesDb.getSymbol(atom.getSpecies());
      if(species)
        atomInfo[kw::STRUCTURE__ATOMS__SPEC] = *species;
    }
    else if(entry.first == kw::STRUCTURE__ATOMS__POS)
      atomInfo[kw::STRUCTURE__ATOMS__POS] = atom.getPosition();
    else if(entry.first == kw::STRUCTURE__ATOMS__RADIUS)
      atomInfo[kw::STRUCTURE__ATOMS__RADIUS] = atom.getRadius();
  }

  return myAtomInfoParser.generateNode(atomInfo);
}

bool StructureYamlGenerator::addProperty(
  YAML::Node propertiesNode,
  const common::Structure & structure,
  const StructureProperty & property) const
{
  ::boost::optional< ::std::string> value = structure.getVisibleProperty(property);

  if(!value)
    return false;

  propertiesNode[property.getName()] = *value;

  return true;
}

void StructureYamlGenerator::praseProperties(
  common::Structure & structure,
  const YAML::Node & propertiesNode) const
{
  common::Structure::VisibleProperty * property;
  for(YAML::const_iterator it = propertiesNode.begin(), end = propertiesNode.end();
    it != end; ++it)
  {
    property =
      structure_properties::VISIBLE_PROPERTIES.getProperty(it->first.as< ::std::string>());

    if(property)
    {
      structure.setVisibleProperty(*property, it->second.as< ::std::string>());
    }
  }
}

}
}
