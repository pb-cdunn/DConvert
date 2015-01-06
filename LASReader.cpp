#include "LASReader.h"

#include "dalign/DB.h"
#include "dalign/align.h"

#include <cstring>
#include <stdlib.h>

void create_id_to_name(std::string db_name, std::map<int, std::string>* id_to_name)
{
  char* c_db_name = (char*)malloc(db_name.size() * sizeof(char));
  strcpy(c_db_name, db_name.c_str());

  HITS_DB    _db, *db = &_db;
  FILE* dbfile;
  Open_DB(c_db_name, db);
  dbfile = Fopen(c_db_name, "r");
  
  int nfiles, last;
  char  prolog[1000], fname[1000];
  if(fscanf(dbfile,DB_NFILE,&nfiles) != 1) SYSTEM_ERROR;
  if(fscanf(dbfile,DB_FDATA,&last,fname,prolog) != 3) SYSTEM_ERROR;

  HITS_READ  *reads;
  int         i, fcount, nblock, ireads, breads;
  int64       size, totlen;

  size = 400*1000000ll;
  reads = db->reads;

  ireads = 0;
  breads = 0;
  totlen = 0;
  nblock = 1;
  fcount = 0;

  for (i = 0; i < db->nreads; i++) {   
    int        len, flags;
    HITS_READ *r;
    
    r     = reads + i;
    len   = r->end - r->beg;
    flags = r->flags;

    if (len >= db->cutoff && (flags & DB_BEST) != 0) {
      ireads += 1;
      breads += 1;
      totlen += len;
      
      if (totlen >= size || ireads >= READMAX) {
        ireads = 0;
        totlen = 0;
        nblock += 1;
      }
      char full_name[1000];
      sprintf(full_name, "%s/%d/%d_%d", prolog, r->origin, r->beg, r->end); 
      (*id_to_name)[breads - 1] = full_name;
      fprintf(stderr, "%d %s\n", breads, full_name);

    } 

    if (i+1 >= last && ++fcount < nfiles) {
      if(fscanf(dbfile,DB_FDATA,&last,fname,prolog) != 3) SYSTEM_ERROR;
    }
  } 
  
  fclose(dbfile);
  Close_DB(db);
  free(c_db_name);
}

LASReader::LASReader(std::string las_name, std::string db_name)
{
  // Create a map from ID to read name.
  create_id_to_name(db_name, &id_to_name);
  
  // Open the LAS file.
  char* c_las_name = (char*)malloc(las_name.size() * sizeof(char));
  strcpy(c_las_name, las_name.c_str());
  input = Fopen(c_las_name, "r");
  free(c_las_name);
  
  // Do some boilerplate reading through initial fields of the LAS file.
  fread(&num_overlaps,sizeof(int64),1,input);
  fread(&tspace,sizeof(int),1,input);

  if (tspace <= TRACE_XOVR) { small  = 1;
    tbytes = sizeof(uint8);
  } else {
    small  = 0;
    tbytes = sizeof(uint16);
  }
  
  tmax = 5000;
  trace = (uint16_t *) Malloc(sizeof(uint16)*tmax,"Allocating trace vector");
  ovl = &_ovl;
  
  ovl_counter = 0;

}

int LASReader::next_overlap(Overlap_T* overlap)
{
  if(ovl_counter >= num_overlaps) return 0;
  
  Read_Overlap(input, ovl);

  if (ovl->path.tlen > tmax) {
    tmax = ((int) 1.2*ovl->path.tlen) + 100;
    trace = (uint16_t *) Realloc(trace,sizeof(uint16)*tmax,"Allocating trace vector");
  }

  ovl->path.trace = (void *) trace;
  Read_Trace(input, ovl, tbytes);

  overlap->name_a = id_to_name.at(ovl->aread);
  overlap->name_b = id_to_name.at(ovl->bread);
  overlap->start_a = ovl->path.abpos;
  overlap->start_b = ovl->path.bbpos;
  overlap->end_a = ovl->path.aepos;
  overlap->end_b = ovl->path.bepos;
  overlap->length_a = ovl->alen;
  overlap->length_b = ovl->blen;
  
  overlap->forward = !COMP(ovl->flags);

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
