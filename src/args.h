#pragma once

namespace ohl::args {

extern bool debug;
extern bool dump_hotfixes;

/**
 * @brief Parses all command line args.
 */
void parse(void);

}  // namespace ohl::args
