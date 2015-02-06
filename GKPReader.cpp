#include "AS_global.H"
#include "GKPReader.h"

GKPReader::GKPReader(std::string gkp_store_name)
{
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
  read_counter++;

  return 1;
}
