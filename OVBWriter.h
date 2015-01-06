#ifndef _OVBWRITER_H_
#define _OVBWRITER_H_

#include "AS_OVS_overlapFile.H"

#include "Overlap.h"

#include <map>
#include <string>
#include <stdint.h>

class OVBWriter {
  public:
    OVBWriter(std::string ovb_name, std::string fastq_name);
    ~OVBWriter();

    int write_overlap(const Overlap_T& overlap);

  private:
    BinaryOverlapFile* output_file;
    std::map<std::string, uint32_t> name_to_iid;
};

#endif
