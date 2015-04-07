#include "LASReader.h"

#include "dalign/DB.h"
#include "dalign/align.h"

#include <cstring>
#include <stdlib.h>

LASReader::LASReader(std::string las_name, std::string db_name)
{
  // Open the LAS file.
  char* c_las_name = (char*)malloc(las_name.size() * sizeof(char));
  strcpy(c_las_name, las_name.c_str());
  input = dalign::Fopen(c_las_name, "r");
  free(c_las_name);
  
  db = &_db;  
  char* c_db_name = (char*)malloc(db_name.size() * sizeof(char));
  strcpy(c_db_name, db_name.c_str());
  Open_DB(c_db_name, db);
  free(c_db_name);
  dalign::Trim_DB(db);


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

int LASReader::next_overlap(proto::Overlap* overlap)
{
  if(ovl_counter >= num_overlaps) return 0;
  
  dalign::Read_Overlap(input, ovl);

  if (ovl->path.tlen > tmax) {
    tmax = ((int) 1.2*ovl->path.tlen) + 100;
    trace = (uint16_t *) dalign::Realloc(trace,sizeof(dalign::uint16)*tmax,"Allocating trace vector");
  }

  ovl->path.trace = (void *) trace;
  dalign::Read_Trace(input, ovl, tbytes);

  overlap->set_id_1(ovl->aread + 1);
  overlap->set_id_2(ovl->bread + 1);
  overlap->set_start_1(ovl->path.abpos);
  overlap->set_start_2(ovl->path.bbpos);
  overlap->set_end_1(ovl->path.aepos);
  overlap->set_end_2(ovl->path.bepos);
  overlap->set_length_1(db->reads[ovl->aread].rlen);
  overlap->set_length_2(db->reads[ovl->bread].rlen);
  
  overlap->set_forward(!COMP(ovl->flags));
  
  overlap->set_diffs(ovl->path.diffs);

  if(COMP(ovl->flags)) {
    int orig_start_2 = overlap->start_2();
    int orig_end_2 = overlap->end_2();
    overlap->set_start_2(overlap->length_2() - orig_end_2);
    overlap->set_end_2(overlap->length_2() - orig_start_2);
  }

  ovl_counter++;
  return 1;
}

LASReader::~LASReader()
{
  Close_DB(db);
  fclose(input);
  free(trace);
}
