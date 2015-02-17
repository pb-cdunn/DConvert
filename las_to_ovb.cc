#include "OVBWriter.h"
#include "Overlap.pb.h"
#include "LASReader.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
  std::string las_name, ovb_name, ovb_style;

  int arg = 1;
  while(arg < argc) {
    if(strcmp(argv[arg], "-L") == 0) {
      las_name = std::string(argv[++arg]);
    }
    if(strcmp(argv[arg], "-O") == 0) {
      ovb_name = std::string(argv[++arg]);
    }
    if(strcmp(argv[arg], "-S") == 0) {
      ovb_style = std::string(argv[++arg]);
    }
    arg++;
  }
  
  if(las_name.size() == 0 || ovb_name.size() == 0) {
    std::cerr << "Usage: las_to_ovb -L <las_name> -O <ovb_name> ";
    std::cerr << "-S <style: obt or ovl>" << std::endl;
    exit(1);
  }
  
  OVBWriter* writer_ptr;
  if(ovb_style == "obt") {
    writer_ptr = new OBTWriter(ovb_name);
  } else if(ovb_style == "ovl") {
    writer_ptr = new OVLWriter(ovb_name);
  } else {
    std::cerr << "OVB style (-S) must be obt or ovl." << std::endl;
    exit(1);
  }


  LASReader las_reader(las_name);
  proto::Overlap overlap;

  while(las_reader.next_overlap(&overlap)) {
    // We don't want to write each overlap twice, so only write those where the
    // ids are ordered.
    if(overlap.id_1() > overlap.id_2()) continue;
    writer_ptr->write_overlap(overlap);
  }
  
  delete writer_ptr; 
  return 0;
}
