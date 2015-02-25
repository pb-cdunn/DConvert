#include "OverlapPrinter.h"
#include <algorithm>
#include <set>


// Amount overhang has to exceed to be noted
// CA uses 15
#define TOLERANCE 15

typedef struct {
  int start; //Start and end wrt the base read, not the partner
  int end;
  bool forward;
  bool left_overhang;
  bool right_overhang;
  uint32_t partner_iid;
} OvlForPrint;

bool by_start_then_end(const OvlForPrint& a, const OvlForPrint& b)
{
  if(a.start == b.start) {
    return a.end < b.end;
  } else {
    return a.start < b.start;
  }
}

std::string overlap_debug_string(const std::vector<proto::Overlap>& ovls,
                                 int width)
{

  std::string formatted_ovl;

  // First gather the overlaps that we're going to print
  std::vector<OvlForPrint> ovls_for_printing;
  uint32_t query_iid = 0;

  for(auto ovl : ovls) {
    OvlForPrint ovl_print = {0,0,false,false,false,0};
    
    ovl_print.start = ovl.start_1();
    ovl_print.end = ovl.end_1();
    ovl_print.partner_iid = ovl.id_2();
    query_iid = ovl.id_1();
      
    // Set the overhangs
    if(ovl.forward()) {
      ovl_print.left_overhang = ovl.start_2() > TOLERANCE; 
      ovl_print.right_overhang = (ovl.length_2() - ovl.end_2()) > TOLERANCE; 
    } else {
      ovl_print.right_overhang = ovl.start_2() > TOLERANCE; 
      ovl_print.left_overhang = (ovl.length_2() - ovl.end_2()) > TOLERANCE; 
    }
    ovl_print.forward = ovl.forward();
    ovls_for_printing.push_back(ovl_print);
  }
  
  std::sort(ovls_for_printing.begin(), ovls_for_printing.end(), by_start_then_end);
  
  int read_length = ovls[0].length_1();
  float bases_per_char = read_length / (float)width;

  formatted_ovl.append(width, '*');
  formatted_ovl.append("\n");
  formatted_ovl.append(std::to_string(query_iid));
  formatted_ovl.append(" Length: ");
  formatted_ovl.append(std::to_string(read_length));
  formatted_ovl.append("\n");
  
  std::set<int> observed_partners;

  for(int print_i = 0; print_i < ovls_for_printing.size(); print_i++) {
    OvlForPrint ovl_print = ovls_for_printing[print_i];
    
    //if(observed_partners.find(ovl_print.partner_iid) != observed_partners.end()) continue;
    observed_partners.insert(ovl_print.partner_iid);

    int pre_chars  = (int)(ovl_print.start / bases_per_char);
    int ovl_chars  = (int)((ovl_print.end - ovl_print.start) / bases_per_char);
    int post_chars = (int)((read_length - ovl_print.end) / bases_per_char);

    char ovl_char = ovl_print.forward ? '>' : '<'; 
    
    // Adjust the lengths to accomodate the overhang symbols 
    if(ovl_print.left_overhang) {
      if(pre_chars) {
        pre_chars--;
      } else if(post_chars) {
        post_chars--;
      } else if(ovl_chars) {
        ovl_chars--;
      }
    }

    if(ovl_print.right_overhang) {
      if(post_chars) {
        post_chars--;
      } else if(pre_chars) {
        pre_chars--;
      } else if(ovl_chars) {
        ovl_chars--;
      }
    }
    // Actually write the characters for the overlap 
    int written_chars = 0;
    formatted_ovl.append(pre_chars, ' ');
    written_chars += pre_chars;
   
    if(ovl_print.left_overhang) {
      formatted_ovl.append("\\");
      written_chars++;
    }

    formatted_ovl.append(ovl_chars, ovl_char);
    written_chars += ovl_chars;

    if(ovl_print.right_overhang) {
      formatted_ovl.append("/");
      written_chars++;
    }

    formatted_ovl.append(post_chars, ' ');
    written_chars += post_chars;
    
    formatted_ovl.append(width - written_chars, ' ');
    formatted_ovl.append(" ");
    formatted_ovl.append(std::to_string(ovl_print.partner_iid));
    formatted_ovl.append(" ");
    formatted_ovl.append(std::to_string(ovl_print.start));
    formatted_ovl.append(" ");
    formatted_ovl.append(std::to_string(ovl_print.end));
    formatted_ovl.append("\n");
  }

  return formatted_ovl;
}
