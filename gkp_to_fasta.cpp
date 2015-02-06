#include "Read.h"
#include "GKPReader.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
  
  std::string gkp_store_name, fastq_filename;

  int arg = 1;
  while(arg < argc) {
    if(strcmp(argv[arg], "-G") == 0) {
      gkp_store_name = std::string(argv[++arg]);
    }
    if(strcmp(argv[arg], "-F") == 0) {
      fastq_filename = std::string(argv[++arg]);
    }
    arg++;
  }

  GKPReader gkp_reader(gkp_store_name);
  Read_T read;

  while(gkp_reader.next_read(&read)) {
    std::cout << ">" << read.name << std::endl;
    int i = 0;
    for(i = 0; i < read.length; i += 100) {
      std::cout << read.seq.substr(i, 100) << std::endl;
    }
  }
  
  return 0;
}
