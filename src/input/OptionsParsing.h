/*
 * OptionsParsing.h
 *
 *  Created on: Aug 18, 2011
 *      Author: Martin Uhrin
 */

#ifndef OPTIONS_PARSING_H
#define OPTIONS_PARSING_H

// INCLUDES //////////////////////////////////
#include <string>

#include <pipelib/PipeRunner.h>

// NAMESPACES ////////////////////////////////
namespace YAML {
class Node;
}

namespace sstbx {
namespace utility {
class HeterogeneousMap;
}
}

namespace stools {
namespace input {

int parseYaml(YAML::Node & nodeOut, const ::std::string & inputFile);

void seedRandomNumberGenerator(const ::sstbx::utility::HeterogeneousMap & schemaOptions);

}
}

#endif /* OPTIONS_PARSING_H */
