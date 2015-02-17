#ifndef DCONVERT_OVBWRITER_H_
#define DCONVERT_OVBWRITER_H_

#include "AS_OVS_overlapFile.H"
#include "Overlap.pb.h"

#include <string>

class OVBWriter {
public:
  OVBWriter(std::string ovb_name);
  virtual ~OVBWriter();
  virtual void write_overlap(const proto::Overlap& overlap) = 0;

protected:
  BinaryOverlapFile* m_output_file;
  proto::Overlap m_cached_overlap;
};


class OBTWriter : public OVBWriter {
  public:
    OBTWriter(std::string ovb_name) : OVBWriter(ovb_name) {}
    ~OBTWriter();
    void write_overlap(const proto::Overlap& overlap);
};

class OVLWriter : public OVBWriter {
  public:
    OVLWriter(std::string ovb_name) : OVBWriter(ovb_name) {}
    void write_overlap(const proto::Overlap& overlap);
};

#endif
