#include "AS_global.H"
#include "GKPReader.h"

#include <zlib.h>
#include "kseq.h"


KSEQ_INIT(gzFile, gzread)

void read_fastq_names(std::string fastq_filename,
                      std::vector<std::string>* fastq_names)
{
  gzFile fastq_file = gzopen(fastq_filename.c_str(), "r");
  kseq_t* seq;
  int l;
  seq = kseq_init(fastq_file);
  while((l = kseq_read(seq)) >= 0) {
    fastq_names->push_back((std::string) seq->name.s);
  }
  kseq_destroy(seq);
  gzclose(fastq_file);
}

GKPReader::GKPReader(std::string gkp_store_name, std::string fastq_name)
{
  read_fastq_names(fastq_name, &fastq_names);
  gk_store = new gkStore(gkp_store_name.c_str(), false, false);
  num_reads = gk_store->gkStore_getNumFragments();

  gk_stream = new gkStream(gk_store, 0, num_reads, GKFRAGMENT_SEQ);
  
  read_counter = 0;
}

GKPReader::~GKPReader()
{
  delete gk_stream;
  delete gk_store;
}

int GKPReader::next_read(Read_T* read)
{
  if(read_counter >= num_reads) return 0;
  
  gkFragment gk_fragment;
  
  gk_stream->next(&gk_fragment);  

  read->name = fastq_names[read_counter];
  
  size_t length =  gk_fragment.gkFragment_getClearRegionLength(AS_READ_CLEAR_LATEST);
  char *seq = gk_fragment.gkFragment_getSequence() + gk_fragment.gkFragment_getClearRegionBegin(AS_READ_CLEAR_LATEST);
  seq[length] = '\0';

  read->seq = std::string(seq);
  read->length = length;
  read_counter++;

  return 1;
}
