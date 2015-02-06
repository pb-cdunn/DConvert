#ifndef _LASREADER_H_
#define _LASREADER_H_

#include <string>
#include <vector>
#include <stdint.h>

#include "Overlap.h"
#include "dalign/DB.h"
#include "dalign/align.h"

class LASReader {
  public:
    LASReader(std::string las_name);
    ~LASReader();

    int next_overlap(Overlap_T* overlap);

  private:
    int64_t num_overlaps; 
    int64_t ovl_counter;
    int tspace, tbytes, small, tmax;
    uint16_t* trace;
    dalign::Overlap _ovl, *ovl;
    FILE* input;

};
#endif
