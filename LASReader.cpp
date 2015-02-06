#include "LASReader.h"

#include "dalign/DB.h"
#include "dalign/align.h"

#include <cstring>
#include <stdlib.h>

LASReader::LASReader(std::string las_name)
{
  // Open the LAS file.
  char* c_las_name = (char*)malloc(las_name.size() * sizeof(char));
  strcpy(c_las_name, las_name.c_str());
  input = dalign::Fopen(c_las_name, "r");
  free(c_las_name);
  
  // Do some boilerplate reading through initial fields of the LAS file.
  fread(&num_overlaps,sizeof(dalign::int64),1,input);
  fread(&tspace,sizeof(int),1,input);

  if (tspace <= TRACE_XOVR) { small  = 1;
    tbytes = sizeof(dalign::uint8);
  } else {
    small  = 0;
    tbytes = sizeof(dalign::uint16);
  }
  
  tmax = 5000;
  trace = (uint16_t *) dalign::Malloc(sizeof(dalign::uint16)*tmax,"Allocating trace vector");
  ovl = &_ovl;
  
  ovl_counter = 0;

}

int LASReader::next_overlap(Overlap_T* overlap)
{
  if(ovl_counter >= num_overlaps) return 0;
  
  dalign::Read_Overlap(input, ovl);

  if (ovl->path.tlen > tmax) {
    tmax = ((int) 1.2*ovl->path.tlen) + 100;
    trace = (uint16_t *) dalign::Realloc(trace,sizeof(dalign::uint16)*tmax,"Allocating trace vector");
  }

  ovl->path.trace = (void *) trace;
  dalign::Read_Trace(input, ovl, tbytes);

  overlap->id_a = ovl->aread + 1;
  overlap->id_b = ovl->bread + 1;
  overlap->start_a = ovl->path.abpos;
  overlap->start_b = ovl->path.bbpos;
  overlap->end_a = ovl->path.aepos;
  overlap->end_b = ovl->path.bepos;
  overlap->length_a = ovl->alen;
  overlap->length_b = ovl->blen;
  
  overlap->forward = !COMP(ovl->flags);
  
  overlap->diffs = ovl->path.diffs;

  if(COMP(ovl->flags)) {
    int orig_start_b = overlap->start_b;
    int orig_end_b = overlap->end_b;
    overlap->start_b = overlap->length_b - orig_end_b;
    overlap->end_b = overlap->length_b - orig_start_b;
  }

  ovl_counter++;
  return 1;
}

LASReader::~LASReader()
{
  fclose(input);
  free(trace);
}
