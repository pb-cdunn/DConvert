#include "OVBWriter.h"
#include "Overlap.h"
#include "LASReader.h"

#include <cstring>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
  
  std::string las_name, db_name, fastq_name, ovb_name;

  int arg = 1;
  while(arg < argc) {
    if(strcmp(argv[arg], "-L") == 0) {
      las_name = std::string(argv[++arg]);
    }
    if(strcmp(argv[arg], "-D") == 0) {
      db_name = std::string(argv[++arg]);
    }
    if(strcmp(argv[arg], "-F") == 0) {
      fastq_name = std::string(argv[++arg]);
    }
    if(strcmp(argv[arg], "-O") == 0) {
      ovb_name = std::string(argv[++arg]);
    }
    arg++;
  }
  
  LASReader las_reader(las_name, db_name);
  OVBWriter ovb_writer(ovb_name, fastq_name); 
  Overlap_T overlap;

  while(las_reader.next_overlap(&overlap)) {
    ovb_writer.write_overlap(overlap);
  }
  
  return 0;
}
