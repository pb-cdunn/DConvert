#include "OVBWriter.h"
#include "Overlap.pb.h"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <fcntl.h>
#include <unistd.h>

#include <iostream>

int main(int argc, char* argv[])
{
  if(argc != 3) {
    std::cout << "Usage: write_to_ovb --style <obt or ovl>" << std::endl;
    exit(1);
  }
  
  std::string ovb_style, ovb_name;

  int arg = 1;
  while(arg < argc) {
    if(strcmp(argv[arg], "--style") == 0) {
      ovb_style = std::string(argv[++arg]);
    }
    arg++;
  }
  
  OVBWriter* writer_ptr;
  if(ovb_style == "obt") {
    writer_ptr = new OBTWriter("-");
  } else if(ovb_style == "ovl") {
    writer_ptr = new OVLWriter("-");
  } else {
    std::cerr << "OVB style (-S) must be obt or ovl." << std::endl;
    exit(1);
  }

  proto::Overlap overlap;
  
  auto raw_input = new google::protobuf::io::IstreamInputStream(&std::cin);
  auto coded_input = new google::protobuf::io::CodedInputStream(raw_input);
  
  int counter = 0; 
  int64_t length_counter = 0;
  int64_t written_counter = 0;

  int buffer_size = 1024;
  void* buffer = (void*)malloc(buffer_size);

  uint32_t record_size = 0;

  while(coded_input->ReadVarint32(&record_size)) {

    if(record_size > buffer_size) {
      buffer_size = record_size * 1.2;
      buffer = (void*) realloc(buffer, buffer_size);
    }

    coded_input->ReadRaw(buffer, record_size);
    overlap.ParseFromArray(buffer, record_size);
    delete coded_input;
    coded_input = new google::protobuf::io::CodedInputStream(raw_input);
    
    if(overlap.id_1() < overlap.id_2()) {
      writer_ptr->write_overlap(overlap);
      ++written_counter;
    }
    ++counter;
    length_counter += record_size;
  }
  
  std::cerr << "Received " << counter << " records of total length " << length_counter << std::endl;
  std::cerr << "Wrote " << written_counter << " of them." << std::endl;

  free(buffer);
  delete coded_input;
  delete raw_input;

  return 0;
}
