#ifndef _LASREADER_H_
#define _LASREADER_H_

#include <string>
#include <vector>
#include <stdint.h>

#include "Overlap.pb.h"
#include "dalign/DB.h"
#include "dalign/align.h"

class LASReader {
  public:
    LASReader(std::string las_name, std::string db_name);
    ~LASReader();

    int next_overlap(proto::Overlap* overlap);

  private:
    int64_t num_overlaps; 
    int64_t ovl_counter;
    int tspace, tbytes, small, tmax;
    uint16_t* trace;
    dalign::Overlap _ovl, *ovl;
    FILE* input;
    dalign::HITS_DB _db;
    dalign::HITS_DB* db; 

};
#endif
