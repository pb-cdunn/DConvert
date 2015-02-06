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

void trim_back_overlap_ends(Overlap_T* overlap)
{
  bool touches_left  = overlap->start_a == 0 || overlap->start_b == 0;
  bool touches_right = overlap->end_a == overlap->length_a ||
                       overlap->end_b == overlap->length_b;
  int clip_size = 25;
  if(touches_left) {
    overlap->start_a += clip_size;
    overlap->start_b += clip_size;
  }

  if(touches_right) {
    overlap->end_a -= clip_size;
    overlap->end_b -= clip_size;
  }
}

// DALIGNER seems to report multiple, overlapping alignments in the same
// orientation. Return true if lhs and rhs are such an overlap pair.
bool is_same_overlap_pair(const Overlap_T& lhs, const Overlap_T& rhs)
{
  return lhs.id_a == rhs.id_a &&
         lhs.id_b == rhs.id_b &&
         lhs.forward == rhs.forward;
}

bool is_better_overlap(const Overlap_T& lhs, const Overlap_T& rhs)
{
  if(lhs.end_a - lhs.start_a > rhs.end_a - rhs.start_a) return true;
  if(lhs.end_b - lhs.start_b > rhs.end_b - rhs.start_b) return true;
  if(lhs.diffs < rhs.diffs) return true;

  return false;
}

// Is the overlap a "full" overlap, using the celera definition of full. That
// means that both the left and right edges of the overlap end at the end of
// read.
bool is_full_overlap(const Overlap_T& overlap)
{
  bool full_on_left = (overlap.start_a == 0) ||
                      (overlap.forward && overlap.start_b == 0) ||
                      (!overlap.forward && overlap.end_b == overlap.length_b);

  bool full_on_right = (overlap.end_a == overlap.length_a) ||
                       (overlap.forward && overlap.end_b == overlap.length_b) ||
                       (!overlap.forward && overlap.start_b == 0);
  return full_on_left && full_on_right;
}

// Create a celera "OBT" overlap from an Overlap_T
void create_obt_overlap(OVSoverlap* celera_ovl, const Overlap_T& overlap)
{
  celera_ovl->a_iid = overlap.id_a;
  celera_ovl->b_iid = overlap.id_b;

  celera_ovl->dat.dat[0] = 0;
  celera_ovl->dat.dat[1] = 0;
  celera_ovl->dat.dat[2] = 0;

  celera_ovl->dat.obt.fwd = overlap.forward;
  celera_ovl->dat.obt.a_beg = overlap.start_a;
  celera_ovl->dat.obt.a_end = overlap.end_a;
  if(overlap.forward) {
    celera_ovl->dat.obt.b_beg = overlap.start_b;
    celera_ovl->dat.obt.b_end_hi = overlap.end_b >> 9;
    celera_ovl->dat.obt.b_end_lo = overlap.end_b & 0x1ff;
  } else {
    celera_ovl->dat.obt.b_beg = overlap.end_b;
    celera_ovl->dat.obt.b_end_hi = overlap.start_b >> 9;
    celera_ovl->dat.obt.b_end_lo = overlap.start_b & 0x1ff;
  }
  
  celera_ovl->dat.obt.erate = static_cast<double>(overlap.diffs)/overlap.length_a * 10000;
  celera_ovl->dat.obt.type = AS_OVS_TYPE_OBT;
}

// Create a celera "OVL" overlap from an Overlap_T
void create_ovl_overlap(OVSoverlap* celera_ovl, const Overlap_T& overlap)
{
  celera_ovl->a_iid = overlap.id_a;
  celera_ovl->b_iid = overlap.id_b;

  celera_ovl->dat.dat[0] = 0;
  celera_ovl->dat.dat[1] = 0;
  celera_ovl->dat.dat[2] = 0;
  
  celera_ovl->dat.ovl.flipped = !overlap.forward;
  
  // Set a_hang.
  if(overlap.start_a > 0) {
    celera_ovl->dat.ovl.a_hang = overlap.start_a;
  } else {
    if(overlap.forward) {
      celera_ovl->dat.ovl.a_hang = -overlap.start_b;
    } else {
      celera_ovl->dat.ovl.a_hang = -1*(overlap.length_b - overlap.end_b);
    }
  }

  // Set b_hang
  if(overlap.end_a < overlap.length_a) {
    celera_ovl->dat.ovl.b_hang = -1*(overlap.length_a - overlap.end_a);
  } else {
    if(overlap.forward) {
      celera_ovl->dat.ovl.b_hang = overlap.length_b - overlap.end_b;
    } else {
      celera_ovl->dat.ovl.b_hang = overlap.start_b;
    }
  }
  
  celera_ovl->dat.ovl.orig_erate = static_cast<double>(overlap.diffs)/overlap.length_a * 10000;
  celera_ovl->dat.ovl.corr_erate = static_cast<double>(overlap.diffs)/overlap.length_a * 10000;
  celera_ovl->dat.ovl.seed_value = 0; // whatever
  celera_ovl->dat.ovl.type = AS_OVS_TYPE_OVL;
}

void write_to_ovb(BinaryOverlapFile* output_file,
                  const Overlap_T& overlap, 
                  void (*ovs_creator_func)(OVSoverlap*, const Overlap_T&))
{
  OVSoverlap celera_ovl; 
  ovs_creator_func(&celera_ovl, overlap);
  AS_OVS_writeOverlap(output_file, &celera_ovl);
}

void unique_write(BinaryOverlapFile* output_file,
                  Overlap_T* cached_overlap, 
                  void (*ovs_creator_func)(OVSoverlap*, const Overlap_T&),
                  const Overlap_T& new_overlap)
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
  if(cached_overlap->id_a != 0)
    write_to_ovb(output_file, *cached_overlap, ovs_creator_func); 

  // And then set the cached overlap as the current one
  *cached_overlap = new_overlap;
}
                  
void OBTWriter::write_overlap(const Overlap_T& overlap)
{
  //Overlap_T overlap_copy(overlap);
  //trim_back_overlap_ends(&overlap_copy);
  unique_write(m_output_file, &m_cached_overlap, create_obt_overlap, overlap);
}

void OVLWriter::write_overlap(const Overlap_T& overlap)
{
  if(is_full_overlap(overlap))
    write_to_ovb(m_output_file, overlap, create_ovl_overlap);
}

// OBTWriter needs to write the last cached read before closing the file.
OBTWriter::~OBTWriter()
{
  if(m_cached_overlap.id_a != 0) {
    write_to_ovb(m_output_file, m_cached_overlap, create_obt_overlap);
  }
}
