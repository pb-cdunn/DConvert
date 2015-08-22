// vim: set et ts=2 sts=2 sw=2:
#include "Overlap.pb.h"
#include "LASReader.h"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <fstream>
#include <iostream>

bool is_big_enough(const proto::Overlap& overlap)
{
  float coverage = (overlap.end_1() - overlap.start_1())/static_cast<float>(overlap.length_1());
  return coverage > 0.02;
}
int main(int argc, char* argv[])
{
  if(argc < 5) {
    std::cout << "Usage: read_from_las --las <las_name> --db <db_name> [--out <overlaps_name>]" << std::endl;
    exit(2);
  }
  std::string las_file_name, db_file_name, out_file_name;

  int arg = 1;
  while(arg < argc) {
    if(strcmp(argv[arg], "--las") == 0) {
      las_file_name = std::string(argv[++arg]);
    }
    if(strcmp(argv[arg], "--db") == 0) {
      db_file_name= std::string(argv[++arg]);
    }
    if(strcmp(argv[arg], "--out") == 0) {
      out_file_name = std::string(argv[++arg]);
    }
    arg++;
  }
  
  auto las_reader = LASReader(las_file_name, db_file_name);
  proto::Overlap overlap;
  
  std::ostream* out = &std::cout;
  std::ofstream out_fstream;
  if (!out_file_name.empty() && out_file_name != "-") {
    out_fstream.open(out_file_name.c_str());
    out = &out_fstream;
  }
  auto raw_output = new google::protobuf::io::OstreamOutputStream(out);
  auto coded_output = new google::protobuf::io::CodedOutputStream(raw_output);
  
  using std::cerr;
  int64_t counter = 0;
  int64_t length_counter = 0;
  while(las_reader.next_overlap(&overlap)) {
    if(!is_big_enough(overlap)) {
        continue;
        cerr << "Skipped overlap!\n";
    }
    if (0) fprintf(stderr, "%04ld %d %d %d %d %d %d\n",
            counter,
            overlap.id_1(), overlap.end_1(), overlap.length_1(),
            overlap.id_2(), overlap.end_2(), overlap.length_2());
    coded_output->WriteVarint32(overlap.ByteSize());
    overlap.SerializeToCodedStream(coded_output);
    ++counter;
    length_counter += overlap.ByteSize();
  }

  std::cerr << "Read " << counter << " records of total length " << length_counter;
  std::cerr << " from " << las_file_name << "." << std::endl;

  delete coded_output;
  delete raw_output;
  return 0;
}
