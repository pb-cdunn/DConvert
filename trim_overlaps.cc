#include "Overlap.pb.h"
#include "Read.pb.h"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <fcntl.h>
#include <unistd.h>

#include <iostream>

void write_overlap(const proto::Overlap& overlap,
                   google::protobuf::io::CodedOutputStream* output)
{
  output->WriteVarint32(overlap.ByteSize());
  overlap.SerializeToCodedStream(output);
}

/* void trim_overlap(proto::Overlap* overlap,
                  std::pair<int, int> bounds_1,
                  std::pair<int, int> bounds_2)
{
  // First figure out how much to adjust the overlap to the left
  int left_adjustment_1 = 0;
  int left_adjustment_2 = 0;
  int left_adjustment = 0;
  left_adjustment_1 = std::max(0, bounds_1.first - overlap->start_1());
  if(overlap->forward()) {
    left_adjustment_2 = std::max(0, bounds_2.first - overlap->start_2());
  } else {
    left_adjustment_2 = std::max(0, overlap->end_2() - bounds_2.second);
  }
  left_adjustment = std::max(left_adjustment_1, left_adjustment_2);

  overlap->set_start_1(overlap->start_1() + left_adjustment);
  if(overlap->forward()) {
    overlap->set_start_2(overlap->start_2() + left_adjustment);
  } else {
    overlap->set_end_2(overlap->end_2() - left_adjustment);
  }
  
  // Then figure out how to adjust the overlap to the right
  int right_adjustment_1 = 0;
  int right_adjustment_2 = 0;
  int right_adjustment = 0;
  right_adjustment_1 = std::max(0, overlap->end_1() - bounds_1.second);
  if(overlap->forward()) {
    right_adjustment_2 = std::max(0, overlap->end_2() - bounds_2.second);
  } else {
    right_adjustment_2 = std::max(0, bounds_2.first - overlap->start_2());
  }
  right_adjustment = std::max(right_adjustment_1, right_adjustment_2);
  
  overlap->set_end_1(overlap->end_1() - right_adjustment);
  if(overlap->forward()) {
    overlap->set_end_2(overlap->end_2() - right_adjustment);
  } else {
    overlap->set_start_2(overlap->start_2() + right_adjustment);
  } 

  // Finally, set the read lengths for each read in the overlap pair
  overlap->set_length_1(bounds_1.second - bounds_1.first);
  overlap->set_length_2(bounds_2.second - bounds_2.first);
} */

void trim_overlap(proto::Overlap* overlap,
                  std::pair<int, int> bounds_1,
                  std::pair<int, int> bounds_2)
{
  // First figure out how much to adjust the overlap to the left
  int left_adjustment_1 = 0;
  int left_adjustment_2 = 0;
  int left_adjustment = 0;
  left_adjustment_1 = std::max(0, bounds_1.first - overlap->start_1());
  if(overlap->forward()) {
    left_adjustment_2 = std::max(0, bounds_2.first - overlap->start_2());
  } else {
    left_adjustment_2 = std::max(0, overlap->end_2() - bounds_2.second);
  }
  left_adjustment = std::max(left_adjustment_1, left_adjustment_2);

  overlap->set_start_1(overlap->start_1() + left_adjustment);
  if(overlap->forward()) {
    overlap->set_start_2(overlap->start_2() + left_adjustment);
  } else {
    overlap->set_end_2(overlap->end_2() - left_adjustment);
  }
  
  // Then figure out how to adjust the overlap to the right
  int right_adjustment_1 = 0;
  int right_adjustment_2 = 0;
  int right_adjustment = 0;
  right_adjustment_1 = std::max(0, overlap->end_1() - bounds_1.second);
  if(overlap->forward()) {
    right_adjustment_2 = std::max(0, overlap->end_2() - bounds_2.second);
  } else {
    right_adjustment_2 = std::max(0, bounds_2.first - overlap->start_2());
  }
  right_adjustment = std::max(right_adjustment_1, right_adjustment_2);
  
  overlap->set_end_1(overlap->end_1() - right_adjustment);
  if(overlap->forward()) {
    overlap->set_end_2(overlap->end_2() - right_adjustment);
  } else {
    overlap->set_start_2(overlap->start_2() + right_adjustment);
  } 
  
  overlap->set_start_1(overlap->start_1() - bounds_1.first);
  overlap->set_end_1(overlap->end_1() - bounds_1.first);
  overlap->set_start_2(overlap->start_2() - bounds_2.first);
  overlap->set_end_2(overlap->end_2() - bounds_2.first);

  // Finally, set the read lengths for each read in the overlap pair
  overlap->set_length_1(bounds_1.second - bounds_1.first);
  overlap->set_length_2(bounds_2.second - bounds_2.first);
}

int main(int argc, char* argv[])
{
  if(argc < 5) {
    std::cout << "Usage: trim_overlaps --overlaps <overlaps> --trimmed_reads <trimmed_reads>" << std::endl;
    exit(1);
  }

  std::string overlap_file_name, trimmed_read_file_name;

  int arg = 1;
  while(arg < argc) {
    if(strcmp(argv[arg], "--overlaps") == 0) {
      overlap_file_name = std::string(argv[++arg]);
    }
    if(strcmp(argv[arg], "--trimmed_reads") == 0) {
      trimmed_read_file_name = std::string(argv[++arg]);
    }
    arg++;
  }
 
  if(trimmed_read_file_name == "-" && overlap_file_name == "-") {
    fprintf(stderr, "Cannot read both overlaps and reads from stdin.");
    exit(1);
  }
 
  google::protobuf::io::ZeroCopyInputStream* raw_overlap_input = nullptr;
  google::protobuf::io::ZeroCopyInputStream* raw_read_input = nullptr;

  int overlap_input_fd = -1;
  int read_input_fd = -1;

  if(overlap_file_name == "-") {
    raw_overlap_input = new google::protobuf::io::IstreamInputStream(&std::cin);
  } else {
    overlap_input_fd = open(overlap_file_name.c_str(), O_RDONLY);
    raw_overlap_input = new google::protobuf::io::FileInputStream(overlap_input_fd);
  }
  
  if(trimmed_read_file_name == "-") {
    raw_read_input = new google::protobuf::io::IstreamInputStream(&std::cin);
  } else {
    read_input_fd = open(trimmed_read_file_name.c_str(), O_RDONLY);
    raw_read_input = new google::protobuf::io::FileInputStream(read_input_fd);
  }

  auto coded_overlap_input = new google::protobuf::io::CodedInputStream(raw_overlap_input);
  auto coded_read_input = new google::protobuf::io::CodedInputStream(raw_read_input);

  auto raw_output = new google::protobuf::io::OstreamOutputStream(&std::cout);
  auto coded_output = new google::protobuf::io::CodedOutputStream(raw_output);

  
  std::vector<std::pair<int, int>> trimmed_read_boundaries;
  proto::Read trimmed_read;
  
  int buffer_size = 1024;
  void* buffer = (void*)malloc(buffer_size);
  uint32_t record_size = 0;
  
  while(coded_read_input->ReadVarint32(&record_size)) {
    if(record_size > buffer_size) {
      buffer_size = record_size * 1.2;
      buffer = (void*) realloc(buffer, buffer_size);
    }

    coded_read_input->ReadRaw(buffer, record_size);
    trimmed_read.ParseFromArray(buffer, record_size);
    
    trimmed_read_boundaries.emplace_back(std::make_pair(trimmed_read.trimmed_start(),
                                                        trimmed_read.trimmed_end()));
  } 

  proto::Overlap overlap;

  while(coded_overlap_input->ReadVarint32(&record_size)) {
    if(record_size > buffer_size) {
      buffer_size = record_size * 1.2;
      buffer = (void*) realloc(buffer, buffer_size);
    }
    
    coded_overlap_input->ReadRaw(buffer, record_size);
    overlap.ParseFromArray(buffer, record_size);    
    
    std::cerr << "*******************" << std::endl;
    std::cerr << overlap.DebugString() << std::endl;   
    auto bounds_1 = trimmed_read_boundaries.at(overlap.id_1() - 1);
    auto bounds_2 = trimmed_read_boundaries.at(overlap.id_2() - 1);
    
    std::cerr << bounds_1.first << " " << bounds_1.second << std::endl;
    std::cerr << bounds_2.first << " " << bounds_2.second << std::endl << std::endl;

    trim_overlap(&overlap, bounds_1, bounds_2);
    std::cerr << overlap.DebugString() << std::endl;   
    std::cerr << "*******************" << std::endl;

    if(overlap.end_1() > overlap.start_1() &&
       overlap.end_2() > overlap.start_2()) {
      write_overlap(overlap, coded_output);
    } else {
      std::cerr << "Skipping it." << std::endl;
    }
  }
  
  delete coded_output;
  delete raw_output;

  delete coded_read_input;
  delete raw_read_input;
  if(read_input_fd != -1) close(read_input_fd); 

  delete coded_overlap_input;
  delete raw_overlap_input;
  if(overlap_input_fd != -1) close(overlap_input_fd); 

  return 0;
}
