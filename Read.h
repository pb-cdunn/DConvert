#ifndef _READ_H_
#define _READ_H_

#include <cstdlib>
#include <string>

typedef struct Read_T { 
  std::string name;
  std::string seq;
  size_t      length;
} Read_T;

#endif
