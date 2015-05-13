#include "Read.pb.h"

#include "AS_PER_gkpStore.H"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <unordered_map>

char             fastqUIDmapName[FILENAME_MAX];

int main(int argc, char* argv[])
{
  if(argc < 5) {
    std::cout << "Usage: apply_trimming_to_gkp [--map-gkp <file> --map-dazz <file>] --gkp <gkp_dir> --trimmed_reads <read_proto>" << std::endl;
    exit(2);
  }

  std::string gkp_name, read_proto_name;
  std::string map_gkp_name, map_dazz_name;

  int arg = 1;
  while(arg < argc) {
    if(strcmp(argv[arg], "--gkp") == 0) {
      gkp_name = std::string(argv[++arg]);
    }
    if(strcmp(argv[arg], "--trimmed_reads") == 0) {
      read_proto_name = std::string(argv[++arg]);
    }
    if(strcmp(argv[arg], "--map-dazz") == 0) {
      map_dazz_name = std::string(argv[++arg]);
    }
    if(strcmp(argv[arg], "--map-gkp") == 0) {
      map_gkp_name = std::string(argv[++arg]);
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
  
  // sprintf(fastqUIDmapName, "%s.fastqUIDmap", gkp_name.c_str());
  std::cerr << "name:'" << map_gkp_name << "'\n";
  std::ifstream fni(map_gkp_name.c_str());
  std::unordered_map<int, int> zmw2uid;
  while (fni) {
    int uid, zmw;
    fni >> uid >> zmw;
    // std::cerr << uid << "<-" << zmw << "\n";
    zmw2uid[zmw] = uid;
  }
  std::cerr << "nuniq:" << zmw2uid.size() << "\n";

  std::cerr << "name:'" << map_dazz_name << "'\n";
  std::ifstream fdazz(map_dazz_name.c_str());
  std::unordered_map<int, int> dazz2zmw;
  while (fdazz) {
    int dazz_id, zmw, length;
    fdazz >> dazz_id >> zmw >> length;
    dazz2zmw[dazz_id] = zmw;
  }
  std::cerr << "nuniq:" << dazz2zmw.size() << "\n";

  while(coded_read_input->ReadVarint32(&record_size)) {
    if(record_size > buffer_size) {
      buffer_size = record_size * 1.2;
      buffer = (void*) realloc(buffer, buffer_size);
    }
    coded_read_input->ReadRaw(buffer, record_size);
    std::cerr << "Parsing from protobuf array.\n";
    trimmed_read.ParseFromArray(buffer, record_size);
    std::cerr << "Parsed from protobuf array.\n";

    delete coded_read_input;
    coded_read_input = new google::protobuf::io::CodedInputStream(raw_read_input);
    std::cerr << "coded_read_input:" << (void*)coded_read_input << "\n";

    int const dazz_id = trimmed_read.id();
    int const zmw = dazz2zmw[dazz_id];
    if (zmw2uid.find(zmw) == zmw2uid.end()) {
      std::cerr << "Cannot find zmw=" << zmw << " (dazz-id=" << dazz_id << ")\n";
      continue;  // already removed?
    }
    int const frgid = zmw2uid[zmw] - 1;  // off by 1, for some reason ~cdunn
    gk_store->gkStore_getFragment(frgid, &gk_fragment, GKFRAGMENT_QLT);

    std::cerr << "Setting read " << trimmed_read.id() << "(" << frgid << ") from " << trimmed_read.untrimmed_length() <<
      " to " << trimmed_read.trimmed_start() << " " << trimmed_read.trimmed_end()
      << " for frag w/ len=" << gk_fragment.gkFragment_getSequenceLength()
      << std::endl;
    gk_fragment.gkFragment_setClearRegion(trimmed_read.trimmed_start(),
                                          trimmed_read.trimmed_end(),
                                          AS_READ_CLEAR_OBTCHIMERA);
    gk_store->gkStore_setFragment(&gk_fragment);
  } 
  
  delete coded_read_input;
  delete raw_read_input;
  close(read_fd);
  
  delete gk_store;

  free(buffer);

  return 0;
}
