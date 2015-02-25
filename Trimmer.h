#ifndef DCONVERT_TRIMMER_H_
#define DCONVERT_TRIMMER_H_

#include "Overlap.pb.h"
#include "Read.pb.h"

#include <vector>

// Use overlaps to trim a read to an interval that will not confuse the
// unitigger. 
proto::Read trim_overlaps(std::vector<proto::Overlap>* overlaps,
                          int agglomeration_distance,
                          int termination_count_threshold,
                          int max_deception_length,
                          int min_spanned_coverage);

#endif
