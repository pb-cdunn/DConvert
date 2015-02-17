#ifndef DCONVERT_TRIMMER_H_
#define DCONVERT_TRIMMER_H_

#include "Overlap.pb.h"
#include "Read.pb.h"

#include <vector>

proto::Read trim_overlaps(std::vector<proto::Overlap>* overlaps,
                          int agglomeration_distance,
                          int termination_count_threshold,
                          int max_deception_length);

#endif
