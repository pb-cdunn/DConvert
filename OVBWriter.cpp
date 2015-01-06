#include "AS_global.H"
#include "OVBWriter.h"

#include <strings.h>
#include <iostream>
#include <zlib.h>


#include "kseq.h"

KSEQ_INIT(gzFile, gzread)

void create_name_to_iid(std::string fastq_name, std::map<std::string, uint32_t>* name_to_iid)
{
  name_to_iid->clear();

  gzFile fastq_file = gzopen(fastq_name.c_str(), "r");
  kseq_t* seq;
  uint32_t counter = 0;
  int l;
  seq = kseq_init(fastq_file);

  int ignore1, ignore2, zmw, start, end;
  char name[250];
  char formatted_full_name[250];
  char* end_of_name;
  while((l = kseq_read(seq)) >= 0) {

    end_of_name = index(seq->name.s, '/');
    *end_of_name = '\0'; 
    strcpy(name, seq->name.s);
    *end_of_name = '/';

    sscanf(end_of_name + 1, "%d/%d_%d/%d_%d", &zmw,
           &ignore1, &ignore2, &start, &end);
    sprintf(formatted_full_name, "%s/%d/%d_%d", name, zmw, start, end); 

    std::string string_name = std::string(formatted_full_name);
    (*name_to_iid)[string_name] = counter + 1;
    counter++;
  }
  kseq_destroy(seq);
  gzclose(fastq_file);
}

OVBWriter::OVBWriter(std::string ovb_name, std::string fastq_name)
{
  char* c_ovb_name = (char*)malloc(ovb_name.size() * sizeof(char));
  strcpy(c_ovb_name, ovb_name.c_str());
  output_file = AS_OVS_createBinaryOverlapFile(c_ovb_name, FALSE);
  free(c_ovb_name);

  create_name_to_iid(fastq_name, &name_to_iid);
}

OVBWriter::~OVBWriter()
{
  AS_OVS_closeBinaryOverlapFile(output_file);
}

int OVBWriter::write_overlap(const Overlap_T& overlap)
{
  
  OVSoverlap ovl;

  ovl.a_iid = name_to_iid.at(overlap.name_a);
  ovl.b_iid = name_to_iid.at(overlap.name_b);

  ovl.dat.dat[0] = 0;
  ovl.dat.dat[1] = 0;
  ovl.dat.dat[2] = 0;

  ovl.dat.obt.fwd = overlap.forward;
  ovl.dat.obt.a_beg = overlap.start_a;
  ovl.dat.obt.a_end = overlap.end_a;
  ovl.dat.obt.b_beg = overlap.start_b;
  ovl.dat.obt.b_end_hi = overlap.end_b >> 9;
  ovl.dat.obt.b_end_hi = overlap.end_b & 0x1ff;
  ovl.dat.obt.type = AS_OVS_TYPE_OBT;
  
  AS_OVS_writeOverlap(output_file, &ovl);

  return 0;
}
