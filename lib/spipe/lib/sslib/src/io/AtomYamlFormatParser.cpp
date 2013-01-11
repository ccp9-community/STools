/*
 * StructureYamlGenerator.cpp
 *
 *  Created on: Aug 18, 2011
 *      Author: Martin Uhrin
 */

// INCLUDES //////////////////////////////////
#include "io/AtomYamlFormatParser.h"

#include <set>

#include <boost/foreach.hpp>

// DEFINES /////////////////////////////////


// NAMESPACES ////////////////////////////////


namespace sstbx {
namespace io {


AtomYamlFormatParser::AtomYamlFormatParser()
{}


AtomYamlFormatParser::AtomYamlFormatParser(const YAML::Node & formatNode)
{
  updateFormat(formatNode);
}

const AtomYamlFormatParser::AtomsFormat & AtomYamlFormatParser::getFormat() const
{
  return myAtomsFormat;
}

void AtomYamlFormatParser::setFormat(const AtomsFormat & format)
{
  myAtomsFormat = format;
}

bool AtomYamlFormatParser::updateFormat(const YAML::Node & formatNode)
{
  myAtomsFormat.clear();
  if(formatNode.IsSequence())
  {
    BOOST_FOREACH(const YAML::Node & formatEntry, formatNode)
    {
      if(formatEntry.IsScalar())
      {
        myAtomsFormat.push_back(
          FormatEntry(formatEntry.as< ::std::string>(), YAML::Node())
        );
      }
      else if(formatEntry.IsMap())
      {
        const YAML::Node::const_iterator it = formatEntry.begin();
        myAtomsFormat.push_back(
          FormatEntry(it->first.as< ::std::string>(), it->second)
        );
      }
    }
  }
  return true;
}

bool AtomYamlFormatParser::parse(AtomInfo & atomInfo, const YAML::Node & atomNode) const
{
  if(atomNode.IsSequence())
  {
    const size_t numEntries = atomNode.size();
    if(numEntries > myAtomsFormat.size())
      return false;

    for(size_t i = 0; i < numEntries; ++i)
    {
      if(atomNode[i].IsNull())
        atomInfo[myAtomsFormat[i].first] = myAtomsFormat[i].second;
      else
        atomInfo[myAtomsFormat[i].first] = atomNode[i];
    }
    return true;
  }
  else if(atomNode.IsMap())
  {
    // Fill it up with everything in the node
    for(YAML::const_iterator it = atomNode.begin(), end = atomNode.end(); it != end; ++it)
      atomInfo[it->first.as< ::std::string>()] = it->second;
    return true;
  }
  else
    return false;
}

YAML::Node AtomYamlFormatParser::generateNode(const AtomInfo & atomInfo) const
{
  typedef ::std::set< ::std::string> EntriesSet;
  // First check if we can use the compact format or not
  EntriesSet entries;

  bool compactFormat = true;
  EntriesSet::const_iterator end = entries.end();
  BOOST_FOREACH(AtomInfo::const_reference entry, atomInfo)
  {
    if(entries.find(entry.first) == end)
    {
      compactFormat = false;
      break;
    }
  }

  YAML::Node atomNode;
  if(compactFormat)
  {
    const AtomInfo::const_iterator end = atomInfo.end();
    AtomInfo::const_iterator it;
    BOOST_FOREACH(const FormatEntry & entry, myAtomsFormat)
    {
      it = atomInfo.find(entry.first);
      if(it != end)
        atomNode.push_back(it->second);
      else
        atomNode.push_back(YAML::Null);
    }
  }
  else
  {
    BOOST_FOREACH(AtomInfo::const_reference entry, atomInfo)
      atomNode[entry.first] = entry.second;
  }

  return atomNode;
}

}
}
