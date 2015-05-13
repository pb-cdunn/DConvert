// vim: set et ts=2 sts=2 sw=2:
#include "OVBWriter.h"
#include "Overlap.pb.h"
#include "IndexMapping.h"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <fcntl.h>
#include <unistd.h>

#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>

int dazz2gkfrg(DConvert::IndexMapping const& im, int dazz_id)
{
    int zmw;
    int const frgid = im.GetGkFragmentIndex(dazz_id, &zmw);
    if (frgid == -1) {
        std::ostringstream msg;
        msg << "Cannot find zmw=" << zmw << " (dazz-id=" << dazz_id << ")\n";
        throw std::runtime_error(msg.str());
    }
    return frgid;
}
int main(int argc, char* argv[])
{
  if(argc < 3) {
    std::cerr << "Usage: write_to_ovb --style <obt or ovl>" << std::endl;
    exit(2);
  }
  
  std::string ovb_style;
  std::string fn_map_gkp, fn_map_dazz, fn_overlaps;

  int arg = 1;
  while(arg < argc) {
    if(strcmp(argv[arg], "--style") == 0) {
      ovb_style = std::string(argv[++arg]);
    }
    if(strcmp(argv[arg], "--map-dazz") == 0) {
      fn_map_dazz = std::string(argv[++arg]);
    }
    if(strcmp(argv[arg], "--map-gkp") == 0) {
      fn_map_gkp = std::string(argv[++arg]);
    }
    if(strcmp(argv[arg], "--overlaps") == 0) {
      fn_overlaps = std::string(argv[++arg]);
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
    exit(2);
  }

  DConvert::IndexMapping im;
  im.Populate(fn_map_dazz, fn_map_gkp);

  proto::Overlap overlap;
  
  std::istream* ovin = &std::cin;
  std::ifstream fovin;
  if (!fn_overlaps.empty() && fn_overlaps != "-") {
    fovin.open(fn_overlaps.c_str());
    ovin = &fovin;
  }
  auto raw_input = new google::protobuf::io::IstreamInputStream(ovin);
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
    
    int o1 = dazz2gkfrg(im, overlap.id_1());
    int o2 = dazz2gkfrg(im, overlap.id_2());
    overlap.set_id_1(o1);
    overlap.set_id_2(o2);

    if(overlap.id_1() < overlap.id_2()) {
      writer_ptr->write_overlap(overlap);
      ++written_counter;
    }
    ++counter;
    length_counter += record_size;
  }
  
  std::cerr << "Received " << counter << " records of total length " << length_counter << std::endl;
  std::cerr << "Wrote " << written_counter << " of them." << std::endl;

  delete writer_ptr;
  free(buffer);
  delete coded_input;
  delete raw_input;

  return 0;
}
