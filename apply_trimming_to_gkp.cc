#include "Read.pb.h"

#include "AS_PER_gkpStore.H"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <fcntl.h>
#include <unistd.h>

#include <iostream>

int main(int argc, char* argv[])
{
  if(argc != 5) {
    std::cout << "Usage: write_to_ovb --gkp <gkp_dir> --trimmed_reads <read_proto>" << std::endl;
    exit(1);
  }

  std::string gkp_name, read_proto_name;

  int arg = 1;
  while(arg < argc) {
    if(strcmp(argv[arg], "--gkp") == 0) {
      gkp_name = std::string(argv[++arg]);
    }
    if(strcmp(argv[arg], "--trimmed_reads") == 0) {
      read_proto_name = std::string(argv[++arg]);
    }
    arg++;
  }

  int read_fd = open(read_proto_name.c_str(), O_RDONLY);
  auto raw_read_input = new google::protobuf::io::FileInputStream(read_fd);
  auto coded_read_input = new google::protobuf::io::CodedInputStream(raw_read_input);
  uint32_t record_size = 0;
  int buffer_size = 1024;
  void* buffer = (void*)malloc(buffer_size);
  proto::Read trimmed_read;

  auto gk_store = new gkStore(gkp_name.c_str(), false, true);
  gk_store->gkStore_enableClearRange(AS_READ_CLEAR_OBTCHIMERA);
  gk_store->gkStore_metadataCaching(true);
  auto num_reads = gk_store->gkStore_getNumFragments();
  gkFragment gk_fragment;
  
  for(uint32_t i=1; i<=num_reads; i++) {
    
    coded_read_input->ReadVarint32(&record_size);
    if(record_size > buffer_size) {
      buffer_size = record_size * 1.2;
      buffer = (void*) realloc(buffer, buffer_size);
    }
    coded_read_input->ReadRaw(buffer, record_size);
    trimmed_read.ParseFromArray(buffer, record_size);


    gk_store->gkStore_getFragment(i, &gk_fragment, GKFRAGMENT_QLT);

    gk_fragment.gkFragment_setClearRegion(trimmed_read.trimmed_start(),
                                          trimmed_read.trimmed_end(),
                                          AS_READ_CLEAR_OBTCHIMERA);
    std::cerr << "Setting read " << trimmed_read.id() << " to " << trimmed_read.trimmed_start() << " " << trimmed_read.trimmed_end() << std::endl;
    gk_store->gkStore_setFragment(&gk_fragment);
  } 
  
  delete coded_read_input;
  delete raw_read_input;
  close(read_fd);
  
  delete gk_store;

  free(buffer);

  return 0;
}
