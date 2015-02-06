#ifndef _OVERLAP_H_
#define _OVERLAP_H_

#include <map>
#include <string>
#include <vector>

#include <cstdlib>

typedef struct {
  // The names of the two reads that overlap
  int id_a;
  int id_b;
  
  // The start positions of the overlap in each read
  int start_a;
  int start_b;

  // The end positions of the overlap in each read
  int end_a;
  int end_b;
  
  // The lengths of the entire read
  int length_a;
  int length_b;

  // Whether the overlap is in the same orientation in each read
  bool forward;
  
  // The overlap edit distance
  int diffs;

} Overlap_T;

#endif
