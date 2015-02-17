#include "AS_global.H"
#include "OVBWriter.h"

#include <cstdlib>
#include <cstring>

OVBWriter::OVBWriter(std::string ovb_name)
{
  char* c_ovb_name = (char*)malloc(ovb_name.size() * sizeof(char));
  strcpy(c_ovb_name, ovb_name.c_str());
  m_output_file = AS_OVS_createBinaryOverlapFile(c_ovb_name, FALSE);
  free(c_ovb_name);
}

OVBWriter::~OVBWriter()
{
  AS_OVS_closeBinaryOverlapFile(m_output_file);
}

void trim_back_overlap_ends(proto::Overlap* overlap)
{
  bool touches_left  = overlap->start_1() == 0 || overlap->start_2() == 0;
  bool touches_right = overlap->end_1() == overlap->length_1() ||
                       overlap->end_2() == overlap->length_2();
  int clip_size = 25;
  if(touches_left) {
    overlap->set_start_1(overlap->start_1() + clip_size);
    overlap->set_start_2(overlap->start_2() + clip_size);
  }

  if(touches_right) {
    overlap->set_end_1(overlap->end_1() - clip_size);
    overlap->set_end_2(overlap->end_2() - clip_size);
  }
}

// DALIGNER seems to report multiple, overlapping alignments in the same
// orientation. Return true if lhs and rhs are such an overlap pair.
bool is_same_overlap_pair(const proto::Overlap& lhs, const proto::Overlap& rhs)
{
  return lhs.id_1() == rhs.id_1() &&
         lhs.id_2() == rhs.id_2() &&
         lhs.forward() == rhs.forward();
}

bool is_better_overlap(const proto::Overlap& lhs, const proto::Overlap& rhs)
{
  if(lhs.end_1() - lhs.start_1() > rhs.end_1() - rhs.start_1()) return true;
  if(lhs.end_2() - lhs.start_2() > rhs.end_2() - rhs.start_2()) return true;
  if(lhs.diffs() < rhs.diffs()) return true;

  return false;
}

// Is the overlap a "full" overlap, using the celera definition of full. That
// means that both the left and right edges of the overlap end at the end of
// read.
bool is_full_overlap(const proto::Overlap& overlap)
{
  bool full_on_left = (overlap.start_1() == 0) ||
                      (overlap.forward() && overlap.start_2() == 0) ||
                      (!overlap.forward() && overlap.end_2() == overlap.length_2());

  bool full_on_right = (overlap.end_1() == overlap.length_1()) ||
                       (overlap.forward() && overlap.end_2() == overlap.length_2()) ||
                       (!overlap.forward() && overlap.start_2() == 0);
  return full_on_left && full_on_right;
}

// Create a celera "OBT" overlap from a proto::Overlap 
void create_obt_overlap(OVSoverlap* celera_ovl, const proto::Overlap& overlap)
{
  celera_ovl->a_iid = overlap.id_1();
  celera_ovl->b_iid = overlap.id_2();

  celera_ovl->dat.dat[0] = 0;
  celera_ovl->dat.dat[1] = 0;
  celera_ovl->dat.dat[2] = 0;

  celera_ovl->dat.obt.fwd = overlap.forward();
  celera_ovl->dat.obt.a_beg = overlap.start_1();
  celera_ovl->dat.obt.a_end = overlap.end_1();
  if(overlap.forward()) {
    celera_ovl->dat.obt.b_beg = overlap.start_2();
    celera_ovl->dat.obt.b_end_hi = overlap.end_2() >> 9;
    celera_ovl->dat.obt.b_end_lo = overlap.end_2() & 0x1ff;
  } else {
    celera_ovl->dat.obt.b_beg = overlap.end_2();
    celera_ovl->dat.obt.b_end_hi = overlap.start_2() >> 9;
    celera_ovl->dat.obt.b_end_lo = overlap.start_2() & 0x1ff;
  }
  
  celera_ovl->dat.obt.erate = static_cast<double>(overlap.diffs())/(overlap.end_1() - overlap.start_1())
                                                  * 10000;
  celera_ovl->dat.obt.type = AS_OVS_TYPE_OBT;
}

// Create a celera "OVL" overlap from a proto::Overlap
void create_ovl_overlap(OVSoverlap* celera_ovl, const proto::Overlap& overlap)
{
  celera_ovl->a_iid = overlap.id_1();
  celera_ovl->b_iid = overlap.id_2();

  celera_ovl->dat.dat[0] = 0;
  celera_ovl->dat.dat[1] = 0;
  celera_ovl->dat.dat[2] = 0;
  
  celera_ovl->dat.ovl.flipped = !overlap.forward();
  
  // Set a_hang.
  if(overlap.start_1() > 0) {
    celera_ovl->dat.ovl.a_hang = overlap.start_1();
  } else {
    if(overlap.forward()) {
      celera_ovl->dat.ovl.a_hang = -overlap.start_2();
    } else {
      celera_ovl->dat.ovl.a_hang = -1*(overlap.length_2() - overlap.end_2());
    }
  }

  // Set b_hang
  if(overlap.end_1() < overlap.length_1()) {
    celera_ovl->dat.ovl.b_hang = -1*(overlap.length_1() - overlap.end_1());
  } else {
    if(overlap.forward()) {
      celera_ovl->dat.ovl.b_hang = overlap.length_2() - overlap.end_2();
    } else {
      celera_ovl->dat.ovl.b_hang = overlap.start_2();
    }
  }
  
  celera_ovl->dat.ovl.orig_erate = static_cast<double>(overlap.diffs())/(overlap.end_1() - overlap.start_1()) * 10000;
  celera_ovl->dat.ovl.corr_erate = static_cast<double>(overlap.diffs())/(overlap.end_1() - overlap.start_1()) * 10000;
  celera_ovl->dat.ovl.seed_value = 0; // whatever
  celera_ovl->dat.ovl.type = AS_OVS_TYPE_OVL;
}

void write_to_ovb(BinaryOverlapFile* output_file,
                  const proto::Overlap& overlap, 
                  void (*ovs_creator_func)(OVSoverlap*, const proto::Overlap&))
{
  OVSoverlap celera_ovl; 
  ovs_creator_func(&celera_ovl, overlap);
  AS_OVS_writeOverlap(output_file, &celera_ovl);
}

void unique_write(BinaryOverlapFile* output_file,
                  proto::Overlap* cached_overlap, 
                  void (*ovs_creator_func)(OVSoverlap*, const proto::Overlap&),
                  const proto::Overlap& new_overlap)
{
  // If this overlap is the same as the cached one, we can't write anything,
  // just see if this overlap should replace the currently cached one
  if(is_same_overlap_pair(*cached_overlap, new_overlap)) {
    if(is_better_overlap(new_overlap, *cached_overlap)) {
      *cached_overlap = new_overlap;
    }
    return;
  } 
  
  // If this is a new overlap, then we can write the cached one as long as the
  // cached overlap is a real overlap
  if(cached_overlap->has_id_1())
    write_to_ovb(output_file, *cached_overlap, ovs_creator_func); 

  // And then set the cached overlap as the current one
  *cached_overlap = new_overlap;
}
                  
void OBTWriter::write_overlap(const proto::Overlap& overlap)
{
  //proto::Overlap overlap_copy(overlap);
  //trim_back_overlap_ends(&overlap_copy);
  unique_write(m_output_file, &m_cached_overlap, create_obt_overlap, overlap);
}

void OVLWriter::write_overlap(const proto::Overlap& overlap)
{
  if(is_full_overlap(overlap))
    write_to_ovb(m_output_file, overlap, create_ovl_overlap);
}

// OBTWriter needs to write the last cached read before closing the file.
OBTWriter::~OBTWriter()
{
  if(m_cached_overlap.has_id_1()) {
    write_to_ovb(m_output_file, m_cached_overlap, create_obt_overlap);
  }
}
