#include "Overlap.pb.h"
#include "LASReader.h"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>


#include <iostream>

int main(int argc, char* argv[])
{
  if(argc != 2) {
    std::cout << "Usage: read_from_las <las_name>" << std::endl;
    exit(1);
  }
  
  std::string las_name = std::string(argv[1]);

  auto las_reader = LASReader(las_name);
  proto::Overlap overlap;
  
  auto raw_output = new google::protobuf::io::OstreamOutputStream(&std::cout);
  auto coded_output = new google::protobuf::io::CodedOutputStream(raw_output);
  
  int counter = 0;
  int length_counter = 0;
  while(las_reader.next_overlap(&overlap)) {
    coded_output->WriteVarint32(overlap.ByteSize());
    overlap.SerializeToCodedStream(coded_output);
    ++counter;
    length_counter += overlap.ByteSize();
  }

  std::cerr << "Read " << counter << " records of total length " << length_counter;
  std::cerr << " from " << las_name << "." << std::endl;

  delete coded_output;
  delete raw_output;
  return 0;
}
