#ifndef _OVERLAP_H_
#define _OVERLAP_H_

#include <map>
#include <string>
#include <vector>

#include <cstdlib>

typedef struct {
  // The names of the two reads that overlap
  std::string name_a;
  std::string name_b;
  
  //The start positions of the overlap in each read
  int start_a;
  int start_b;

  //The end positions of the overlap in each read
  int end_a;
  int end_b;
  
  //The lengths of the entire read
  int length_a;
  int length_b;

  //Whether the overlap is in the same orientation in each read
  bool forward;
} Overlap_T;

typedef std::map<std::string, std::vector<int> > QnameToOvlsMap_T;

void Overlap_create_map(const std::vector<Overlap_T>& overlaps,
                        QnameToOvlsMap_T* qname_to_ovls);

std::string Overlap_name_tail(const std::string& qname);
#endif
