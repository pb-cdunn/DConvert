#ifndef _OVERLAPPRINTER_H_
#define _OVERLAPPRINTER_H_

#include <string>
#include <vector>

#include "Overlap.pb.h"

std::string overlap_debug_string(const std::vector<proto::Overlap>& ovls,
                                 int width);

#endif
