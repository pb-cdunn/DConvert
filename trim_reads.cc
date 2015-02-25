#include "Trimmer.h"

#include "Overlap.pb.h"
#include "Read.pb.h"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <fcntl.h>
#include <unistd.h>

#include <iostream>

void write_trimmed_read(const proto::Read& read,
                        google::protobuf::io::CodedOutputStream* output)
{
  std::cerr << " Read id: " << read.id() << " (" << read.trimmed_start();
  std::cerr << "-" << read.trimmed_end() << ") " << read.untrimmed_length();
  if(read.trimmed_end() - read.trimmed_start() != read.untrimmed_length()) std::cerr << " *";
  std::cerr << std::endl;
  output->WriteVarint32(read.ByteSize());
  read.SerializeToCodedStream(output);
}

int main(int argc, char* argv[])
{

  if(argc < 3) {
    std::cout << "Usage: trim_reads --overlaps <overlaps>" << std::endl;
    exit(1);
  }
  
  std::string overlap_file_name;
  int agglomeration_distance = 100;
  int termination_count_threshold = 3;
  int max_deception_length = 50;
  int min_spanned_coverage = 1;

  int arg = 1;
  while(arg < argc) {
    if(strcmp(argv[arg], "--overlaps") == 0) {
      overlap_file_name = std::string(argv[++arg]);
    }
    if(strcmp(argv[arg], "--agglomeration_distance") == 0) {
      agglomeration_distance = atoi(argv[++arg]);
    }
    if(strcmp(argv[arg], "--termination_count_threshold") == 0) {
      termination_count_threshold = atoi(argv[++arg]);
    }
    if(strcmp(argv[arg], "--max_deception_length") == 0) {
      max_deception_length = atoi(argv[++arg]);
    }
    if(strcmp(argv[arg], "--min_spanned_coverage") == 0) {
      min_spanned_coverage = atoi(argv[++arg]);
    }
    arg++;
  }
  
  auto raw_read_output = new google::protobuf::io::OstreamOutputStream(&std::cout);
  auto coded_read_output = new google::protobuf::io::CodedOutputStream(raw_read_output);

  google::protobuf::io::ZeroCopyInputStream* raw_input = nullptr;
  int input_fd = -1;
  if(overlap_file_name == "-" || overlap_file_name.size() == 0) {
    raw_input = new google::protobuf::io::IstreamInputStream(&std::cin);
  } else {
    input_fd = open(overlap_file_name.c_str(), O_RDONLY);
    raw_input = new google::protobuf::io::FileInputStream(input_fd);
  }
  auto coded_input = new google::protobuf::io::CodedInputStream(raw_input);

  proto::Overlap overlap;
  std::vector<proto::Overlap> overlaps;
  proto::Read trimmed_read;
  
  int buffer_size = 1024;
  void* buffer = (void*)malloc(buffer_size);
  uint32_t record_size = 0;

  int current_id1 = -1;

  while(coded_input->ReadVarint32(&record_size)) {

    if(record_size > buffer_size) {
      buffer_size = record_size * 1.2;
      buffer = (void*) realloc(buffer, buffer_size);
    }

    coded_input->ReadRaw(buffer, record_size);
    overlap.ParseFromArray(buffer, record_size);
    
    if(overlap.id_1() != current_id1) {
      if(overlaps.size() > 0) {
        trimmed_read = trim_overlaps(&overlaps, agglomeration_distance, termination_count_threshold,
                                     max_deception_length, min_spanned_coverage);
        write_trimmed_read(trimmed_read, coded_read_output);
      }
      overlaps.clear();
      current_id1 = overlap.id_1();
    }
    overlaps.push_back(overlap);
  }

  if(overlaps.size() > 0) {
    trimmed_read = trim_overlaps(&overlaps, agglomeration_distance, termination_count_threshold,
                                max_deception_length, min_spanned_coverage);
    write_trimmed_read(trimmed_read, coded_read_output);
  }
  
  delete coded_read_output;
  delete raw_read_output;

  delete coded_input;
  delete raw_input;
  if(input_fd != -1) close(input_fd);

  return 0;
}
