#include "Overlap.pb.h"
#include "Read.pb.h"
#include "Trimmer.h"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <fcntl.h>
#include <unistd.h>

#include <iostream>

using OutputStream = google::protobuf::io::CodedOutputStream;
using InputStream = google::protobuf::io::CodedInputStream;

const int AGGLOMERATION_THRESHOLD = 25;
const int TERMINATION_COUNT_THRESHOLD = 3;
const int MAX_DECEPTION_LENGTH = 50;

void write_overlaps(const std::vector<Overlap>& overlaps,
                    OutputStream* out_stream)
{
  for(auto overlap : overlaps) {
    out_stream->WriteVarint32(overlap.ByteSize());
    overlap.SerializeToCodedStream(out_stream);
  }
}

void write_read(const proto::Read& read, OutputStream* out_stream)
{
  out_stream->WriteVarint32(read.ByteSize());
  read.SerializeToCodedStream(out_stream);
}

proto::Read trim_overlaps(std::vector<Overlap>* overlaps)
{

  std::vector<TerminationRegion> termination_regions =
    find_overlap_termination_regions(*overlaps, AGGLOMERATION_THRESHOLD,
                                     TERMINATION_COUNT_THRESHOLD);

  normalize_overlaps(overlaps, termination_regions, MAX_DECEPTION_LENGTH);

}

int main(int argc, char* argv[])
{
  
  if(argc != 2) {
    std::cout << "Usage: filter_chimeras <read_bound_name>" << std::endl;
    exit(1);
  }
  std::string read_bound_name = std::string(argv[1]);

  
  // Create the input and output streams
  auto raw_input = new google::protobuf::io::IstreamInputStream(&std::cin);
  auto coded_input = new InputStream(raw_input);
  auto raw_output = new google::protobuf::io::OstreamOutputStream(&std::cout);
  auto coded_output = new OutputStream(raw_output);

  int read_bound_fd = open(read_bound_name.c_str(), O_WRONLY);
  auto raw_read_bound_output = new google::protobuf::io::FileOutputStream(read_bound_fd);
  auto coded_read_bound_output = new OutputStream(raw_read_bound_output);
  
  // Set up some stuff for reading from the input stream
  int buffer_size = 1024;
  void* buffer = (void*) malloc(buffer_size);
  uint32_t record_size = 0;
  
  // Keep a vector of overlaps for a single read 
  std::vector<proto::Overlap> current_overlaps;
  
  // And now we start reading
  while(coded_input->ReadVarint32(&record_size)) {
    
    // The size of most records is like 25 bytes, but just in case...
    if(record_size > buffer_size) {
      buffer_size = record_size * 1.2;
      buffer = (void*) realloc(buffer, buffer_size);
    }

    // Actually read the data into an overlap
    coded_input->ReadRaw(buffer, record_size);
    overlap.ParseFromArray(buffer, record_size);
    
    // Are we at a new overlap? Then process the current overlaps and write them
    if(current_overlaps.size() >= 0 &&
       current_overlaps[0].id_1() != overlap.id_1()) {
      
      proto::Read trimmed_read = trim_overlaps(&current_overlaps);

      write_overlaps(current_overlaps, coded_output);
      write_read(trimmed_read, coded_read_bound_output);
    }

    current_overlaps.clear();
    current_overlaps.push_back(overlap);
  }
  
  proto::Read trimmed_read = trim_overlaps(&current_overlaps);
  write_overlaps(current_overlaps, coded_output);
  write_read(trimmed_read, coded_read_bound_output);
  
  delete coded_read_bound_output;
  delete raw_read_bound_output;
  close(read_bound_fd);

  delete coded_output;
  delete raw_output;
  delete coded_input;
  delete raw_input;
  free(buffer);

  return 0;
}
