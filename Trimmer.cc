#include "Trimmer.h"

#include "OverlapPrinter.h"

#include <algorithm>
#include <cassert>
#include <iostream>

#define DEBUG true

// These functions are supposed to find and trim reads that are "chimeric" in
// the celera assembler sense of the word. That just means something that's
// going to screw up layout, so a read that won't have nice overlaps to other
// reads on either end.
//
// A few definitions:
//   I draw overlaps like this:
// \<<                                                          11361 144307/0_12628
// \<<<<                                                        9530 160528/0_4008
// \>>>>>>>                                                     4701 130100/0_8145
// \<<<<<<<<<                                                   11323 142617/0_11026
// \>>>>>>>>>                                                   2163 101272/0_8026
// \<<<<<<<<<<<<<<<<<<<<</                                      2802 142451/0_9182
// \<<<<<<<<<<<<<<<<<<<<</                                      3288 24234/0_9813
// \>>>>>>>>>>>>>>>>>>>>>>>>                                    11948 34270/0_7952
//     <<<<<<<<<<<<<<<</                                        4197 94189/0_4959
//     >>>>>>>>>>>>>>>>/                                        4556 120423/0_10954
//      \<</                                                    9529 160528/0_802
//        <<                                                    12067 43639/0_537
//                 >>>>>/                                       12082 44684/0_2839
//                 <<<</                                        12105 45959/0_10027
//                 <<<                                          12749 97031/0_801
//                 >>>>/                                        11719 18014/0_8356
//                   <<</                                       5544 34773/0_2832
//                    <</                                       5475 30142/0_9326
//                     /                                        2010 89822/0_2417
//                        <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<</  876 155317/0_8563
//                         \<<<<<<<<<<<<<<<<<<<<<<<<<<          10843 111586/0_5763
//                           <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<</  953 15947/0_12831
//                               <<<<<<<<<<<<<<<<<<<<<<<<<<<</  5543 34773/0_10145
//                                          >>>>>>>>>>>>>>>>>/  9606 18285/0_12538
//                                            >>>>>>>>>>>>>>>/  6 100662/0_8879
//                                             >>>>>>>>>>>>>>/  559 137168/0_9549
// ************************************************************
//
// The asterisks on the bottom represent the query read. The gt and lt
// characters represent the overlap with the partner read. If a \ or / character
// appears at the end of the overlap, that means that there were more bases in
// the partner read that were not part of the overlap.
//
// A _terminating overlap_ is an an overlap that has a boundary that isn't the
// end of one of its reads. An overlap _terminates from the left_ if it is a
// terminating overlap and the termination is on the right side of the overlap
// wrt the query read. So in the picture above, the overlaps with 4197 and 4556
// terminate from the left. The overlap with 10843 _terminates from the right_.
// The overlap with 9529 terminates from both right and left.
//
// The trimming method has the following steps:
// 
// 1. Identify intervals of the query read that have a lot of terminating
// overlaps. Overlaps that terminate often won't terminate at the exact same
// spot, so we'll group them together.
//
// 2. Trim back reads that terminate within the termination intervals the start
// of the interval. The "start" depends on whether the interval is for for
// terminations from the left or from the right.
//
// 3. Trim back overlaps that have been extended through a termination interval
// to the end of a read. This seems to be a daligner idiosyncracy, but if you
// look at the overlap with 11948 above, you see that it does not terminate but
// instead extends to the end of the read. This happens sometimes and confuses
// trimming.
//
// 4. Identify termination intervals with insufficient overlap coverage in
// them. If the termination intervals does not have enough overlaps that span
// it, we'll call it a _suspect interval_.
//
// 5. Find the largest region of the query read between suspect intervals. Trim
// all the overlaps to this region and output its bounds.


// The direction of a terminating overlap
enum class TerminationDirection { FROMTHELEFT, FROMTHERIGHT };

// A litte pod class for termination intervals. See comments in Trimmer.cc
class TerminationInterval {
public:
  int start;
  int end;
  TerminationDirection direction;
  bool operator==(const TerminationInterval& other) const
  {return start==other.start && end==other.end && direction==other.direction;}
};

// Returns whether this overlap terminates from the left
bool terminates_from_left(const proto::Overlap& overlap)
{
  if(overlap.forward()) {
    //    >>>>>/
    // ************
    return overlap.length_2() > overlap.end_2() &&
           overlap.end_1() < overlap.length_1();
  } else {
    //    <<<<</
    // ************
    return overlap.start_2() > 0 &&
           overlap.end_1() < overlap.length_1();
  }
}

// Returns whether this overlap terminates from the left
bool terminates_from_right(const proto::Overlap& overlap)
{
  if(overlap.forward()) {
    //    \>>>>>
    // ************
    return overlap.start_2() > 0 &&
           overlap.start_1() > 0;
  } else {
    //    \<<<<<
    // ************
    return overlap.length_2() > overlap.end_2() &&
            overlap.start_1() > 0;
  }
}

// Whether the overlap extends to the ends of the partner read in the given
// direction 
bool extends_to_end(const proto::Overlap& overlap, TerminationDirection direction)
{
  if(direction == TerminationDirection::FROMTHERIGHT) {
    if(overlap.start_1() == 0) return true;
    if(overlap.forward()) {
      return overlap.start_2() == 0;
    } else {
      return overlap.end_2() == overlap.length_2();
    }
  } else if(direction == TerminationDirection::FROMTHELEFT) {
    if(overlap.end_1() == overlap.length_1()) return true;
    if(overlap.forward()) {
      return overlap.end_2() == overlap.length_2();
    } else {
      return overlap.start_2() == 0;
    }
  } else {
    assert(!"Oh no.");
  }
}

// Create a vector of intervals that contain sufficiently many terminating
// overlaps
std::vector<TerminationInterval> create_termination_intervals(
      const std::map<int, int>& termination_positions, // Positions of the query read where
                                                       // the overlaps terminate
      TerminationDirection direction, // The direction of all the terminations
      int agglomeration_distance,
      int termination_count_threshold)
{
  std::vector<TerminationInterval> passing_intervals;
  int current_start = 0;
  int current_end = 0;
  int current_count = 0;
  for(auto termination_position : termination_positions) {
    // See if we're too far past the current termination interval and we need
    // to start a new one
    if(termination_position.first > current_end + agglomeration_distance) {

      // Did we see enough terminations in the interval? Then record it.
      if(current_count > termination_count_threshold) {
        passing_intervals.push_back(
            TerminationInterval{current_start, current_end, direction});
      }
      
      current_start = termination_position.first;
      current_end = current_start + 1;
      current_count = termination_position.second;

    // If not, just extend the interval end an increment the count
    } else {
      current_end = termination_position.first + 1;
      current_count += termination_position.second;
    }
  }
  
  if(current_count > termination_count_threshold) {
    passing_intervals.push_back(
        TerminationInterval{current_start, current_end, direction});
  }

  return passing_intervals;
}

// Create a sorted vector of positions at which overlaps terminate from
// a given direction
std::map<int, int> identify_terminating_overlaps(
      const std::vector<proto::Overlap>& overlaps,
      TerminationDirection direction)
{
  std::map<int, int> termination_positions;
  
  for(auto& overlap : overlaps) {
    //std::cerr << "Checking ovl " << overlap.start_1() << " " << overlap.end_1() << std::endl;
    switch(direction) {
      case TerminationDirection::FROMTHELEFT : 
        if(terminates_from_left(overlap)) {
          //std::cerr << "Pushing back left " << overlap.end_1() << std::endl;
          termination_positions[overlap.end_1()]++;
        }
        break;
      case TerminationDirection::FROMTHERIGHT : 
        if(terminates_from_right(overlap)) {
          //std::cerr << "Pushing back right " << overlap.start_1() << std::endl;
          termination_positions[overlap.start_1()]++;
        }
        break;
      default :
        assert(!"Oh no.");
    }
  }

  //std::sort(termination_positions.begin(), termination_positions.end());
  return termination_positions;
}


// Trim the overlap back to the edge of the given interval in the specified
// direction
void trim_overlap_to_interval(
      proto::Overlap* overlap, int start, int end,
      TerminationDirection direction)
{
  if(direction == TerminationDirection::FROMTHERIGHT) {
    int adjustment_size = end - overlap->start_1() - 1;
    if(adjustment_size < 0) adjustment_size = 0;
    adjustment_size = std::min(adjustment_size, overlap->end_1() - overlap->start_1());
    adjustment_size = std::min(adjustment_size, overlap->end_2() - overlap->start_2());
    assert(adjustment_size >= 0);

    overlap->set_start_1(overlap->start_1() + adjustment_size);

    if(overlap->forward()) {
      overlap->set_start_2(overlap->start_2() + adjustment_size);
    } else {
      overlap->set_end_2(overlap->end_2() - adjustment_size);
    }
  } 
  
  else if(direction == TerminationDirection::FROMTHELEFT) {
    int adjustment_size = overlap->end_1() - start;
    if(adjustment_size < 0) adjustment_size = 0;
    adjustment_size = std::min(adjustment_size, overlap->end_1() - overlap->start_1());
    adjustment_size = std::min(adjustment_size, overlap->end_1() - overlap->start_1());
    adjustment_size = std::min(adjustment_size, overlap->end_2() - overlap->start_2());
    assert(adjustment_size >= 0);
    
    overlap->set_end_1(overlap->end_1() - adjustment_size);

    if(overlap->forward()) {
      overlap->set_end_2(overlap->end_2() - adjustment_size);
    } else {
      overlap->set_start_2(overlap->start_2() + adjustment_size);
    }
  }
}

// Trim terminating overlaps back to the edge of their containing termination
// interval
void trim_terminating_overlaps(
      std::vector<proto::Overlap>* overlaps,
      const std::vector<TerminationInterval>& termination_intervals)
{
  if(termination_intervals.size() == 0 ||
     overlaps->size() == 0)
    return;
  auto direction = termination_intervals[0].direction;
  
  // Get a function pointers that are determined by the direction we're looking
  bool (*terminates_from_correct_direction)(const proto::Overlap&) = nullptr;
  int (proto::Overlap::*business_end)(void) const = nullptr;
  switch(direction) {
    case TerminationDirection::FROMTHELEFT :
      business_end = &proto::Overlap::end_1; 
      terminates_from_correct_direction = &terminates_from_left;
      break;
    case TerminationDirection::FROMTHERIGHT :
      business_end = &proto::Overlap::start_1; 
      terminates_from_correct_direction = &terminates_from_right;
      break;
  }

  // Sort the overlaps by their relevant position in the query read 
  // std::cerr << "Sorting " << overlaps->size() << " overlaps." << std::endl;
  std::vector<proto::Overlap*> ovl_ptrs;
  std::for_each(overlaps->begin(), overlaps->end(),
                [&ovl_ptrs](proto::Overlap& a){ovl_ptrs.push_back(&a);});
  std::sort(ovl_ptrs.begin(), ovl_ptrs.end(),
            [&business_end](const proto::Overlap* a, const proto::Overlap* b)
              {return (a->*business_end)() < (b->*business_end)();});
  //std::sort(overlaps->begin(), overlaps->end(),
  //          [&business_end](const proto::Overlap& a, const proto::Overlap& b)
  //            {return (a.*business_end)() < (b.*business_end)();});
  
  // Now iterate over the overlaps, trimming them when necessary
  int interval_index = 0;
  for(auto ovl_ptr : ovl_ptrs) {
    // Skip ahead to the relevant termination interval
    while(interval_index < termination_intervals.size() - 1 &&
          termination_intervals[interval_index].end < (ovl_ptr->*business_end)()) {
      ++interval_index;
    }
    auto termination_interval = termination_intervals[interval_index];

    // Check if the overlap terminates in the interval. If so, trim it back
    if(terminates_from_correct_direction(*ovl_ptr)) {
      if((ovl_ptr->*business_end)() >= termination_interval.start &&
         (ovl_ptr->*business_end)() <= termination_interval.end)
      {
        trim_overlap_to_interval(ovl_ptr, termination_interval.start,
                                 termination_interval.end, direction);
      }
    }
  }
}

void trim_deceptive_overlaps(std::vector<proto::Overlap>* overlaps,
                             const std::vector<TerminationInterval>& termination_intervals,
                             int max_deception_length)
{
  if(termination_intervals.size() == 0 ||
     overlaps->size() == 0)
    return;
  auto direction = termination_intervals[0].direction;

  std::vector<proto::Overlap*> ovl_ptrs;
  std::for_each(overlaps->begin(), overlaps->end(),
                [&ovl_ptrs](proto::Overlap& a){ovl_ptrs.push_back(&a);});

  if(direction == TerminationDirection::FROMTHELEFT) {
    // std::cerr << "Sorting " << overlaps->size() << " overlaps." << std::endl;
    std::sort(ovl_ptrs.begin(), ovl_ptrs.end(),
              [](const proto::Overlap* a, const proto::Overlap* b)
                {return a->end_1() < b->end_1();});
  } else {
    // std::cerr << "Sorting " << overlaps->size() << " overlaps." << std::endl;
    std::sort(ovl_ptrs.begin(), ovl_ptrs.end(),
              [](const proto::Overlap* a, const proto::Overlap* b)
                {return a->start_1() < b->start_1();});
  }

  int interval_index = 0;
  for(auto ovl_ptr : ovl_ptrs) {
    if(!extends_to_end(*ovl_ptr, direction)) continue;

    int business_end = -1; 
    int revised_bound = -1;
    if(direction == TerminationDirection::FROMTHELEFT) {
      business_end = ovl_ptr->end_1();
      revised_bound = ovl_ptr->end_1() - max_deception_length;
    } else {
      business_end = ovl_ptr->start_1();
      revised_bound = ovl_ptr->start_1() + max_deception_length;
    }

    // Skip ahead to the relevant termination interval
    while(interval_index < termination_intervals.size() - 1 &&
          termination_intervals[interval_index].end < business_end) {
      ++interval_index;
    }
    auto termination_interval = termination_intervals[interval_index];
    
    if(std::min(revised_bound, business_end) <= termination_interval.end &&
       std::max(revised_bound, business_end) >= termination_interval.start)
    {
      trim_overlap_to_interval(ovl_ptr, termination_interval.start,
                               termination_interval.end, direction);
    }
  }
}

bool is_spanned(int start, int end, int min_coverage, 
                const std::vector<proto::Overlap>& overlaps)
{
  int coverage_counter_1 = 0;
  int coverage_counter_2 = 0;
  //for(auto& overlap : overlaps) {
  //  if(overlap.start_1() <= start && end <= overlap.end_1())
  //    ++coverage_counter;
  //    if(coverage_counter >= min_coverage) return true;
  //}
  int point_1 = start + 1;
  int point_2 = end - 1;
  //std::cerr << "Checking points " << point_1 << " " << point_2 << std::endl;
  for(auto& overlap : overlaps) {
    if(overlap.start_1() <= point_1 && point_1 <= overlap.end_1())
      ++coverage_counter_1;
    if(overlap.start_1() <= point_2 && point_2 <= overlap.end_1())
      ++coverage_counter_2;
    if(coverage_counter_1 >= min_coverage && coverage_counter_2 >= min_coverage) return true;
  }
  return false;
}

std::vector<std::pair<int, int>> find_spanned_intervals(
      const std::vector<TerminationInterval>& termination_intervals,
      const std::vector<proto::Overlap>& overlaps,
      int min_coverage)
{
  std::vector<std::pair<int, int>> spanned_intervals;  
  if(overlaps.size() == 0 || termination_intervals.size() == 0) return spanned_intervals;

  std::vector<int> boundaries;

  for(auto& termination_interval : termination_intervals) {
    if(termination_interval.direction == TerminationDirection::FROMTHELEFT) {
      boundaries.push_back(termination_interval.start);
    } else {
      boundaries.push_back(termination_interval.end);
    }
  }
  boundaries.push_back(0);
  boundaries.push_back(overlaps[0].length_1());

  std::sort(boundaries.begin(), boundaries.end());
  
  bool last_interval_was_spanned = false;
  int last_interval_start = 0;
  int last_interval_end = 0;

  for(int i=0; i<boundaries.size() - 1; i++) {
    //std::cerr << "Checking interval " << boundaries[i] << " " << boundaries[i+1] << std::endl;
    bool this_interval_is_spanned = is_spanned(boundaries[i], boundaries[i+1], min_coverage, overlaps);
    //std::cerr << "Spanned? " << std::boolalpha << this_interval_is_spanned << std::endl;

    if(!this_interval_is_spanned) {
      if(last_interval_was_spanned) {
        spanned_intervals.push_back(std::make_pair(last_interval_start, last_interval_end));
      }
      last_interval_start = 0;
      last_interval_end = 0;
      last_interval_was_spanned = false;
    } else {
      last_interval_end = boundaries[i+1];
      if(!last_interval_was_spanned) {
        last_interval_start = boundaries[i];
      }
      last_interval_was_spanned = true;
    }
  }
  if(last_interval_was_spanned) {
    spanned_intervals.push_back(std::make_pair(last_interval_start, last_interval_end));
  }
  return spanned_intervals;
}

proto::Read trim_to_largest_spanned_interval(const std::vector<proto::Overlap>& overlaps,
                                             const std::vector<std::pair<int, int>>& spanned_intervals)
{
  if(overlaps.size() == 0) {
    return proto::Read();
  }
  
  // Initialize the output read to not trimming anything 
  proto::Read output_read;  
  output_read.set_id((overlaps)[0].id_1());
  output_read.set_trimmed_start(0);
  output_read.set_trimmed_end((overlaps)[0].length_1());
  output_read.set_untrimmed_length((overlaps)[0].length_1());

  if(spanned_intervals.size() == 0) {
    return output_read;
  }
  
  //std::cerr << "Before trimming to largest spanned interval: " << std::endl; 
  //for(auto& overlap : *overlaps) {
  //  std::cerr << overlap.DebugString() << std::endl;
  //}
    
  using Interval = std::pair<int, int>;
  Interval largest_interval = *std::max_element(spanned_intervals.begin(), spanned_intervals.end(),
    [](Interval a, Interval b){return (a.second - a.first) < (b.second - b.first);});
  
  output_read.set_trimmed_start(largest_interval.first); 
  output_read.set_trimmed_end(largest_interval.second); 

  int interval_size = largest_interval.second - largest_interval.first;
  //for(auto& overlap : *overlaps) {
    // This will trim back the starts and ends of each overlap to the
    // boundaries of the spanned interval
    //std::cerr << "TRIMMING: " << std::endl << overlap.DebugString();
    //std::cerr <<  "TO " << largest_interval.first << " " << largest_interval.second << std::endl;
  //  trim_overlap_to_interval(&overlap, largest_interval.second, largest_interval.first,
  //                           TerminationDirection::FROMTHELEFT);
    //std::cerr << "AFTER LEFT " << std::endl << overlap.DebugString();
  //  trim_overlap_to_interval(&overlap, largest_interval.second, largest_interval.first,
  //                           TerminationDirection::FROMTHERIGHT);
    //std::cerr << "AFTER RIGHT " << std::endl << overlap.DebugString();
    
    // Now we need to adjust the length of the 
  //  overlap.set_length_1(interval_size);
//  } 
  //std::cerr << "After trimming to largest spanned interval: " << std::endl; 
  //for(auto& overlap : *overlaps) {
  //  std::cerr << overlap.DebugString() << std::endl;
  //}
  
  //auto erase_itr = std::remove_if(overlaps->begin(), overlaps->end(),
  //    [](proto::Overlap& overlap){return overlap.end_1() == overlap.start_1() ||
  //                                       overlap.end_2() == overlap.start_2();});
  //overlaps->erase(erase_itr, overlaps->end());

  //std::cerr << "After removing: " << std::endl; 
  //for(auto& overlap : *overlaps) {
  //  std::cerr << overlap.DebugString() << std::endl;
  //}
  return output_read;
}

proto::Read trim_overlaps(std::vector<proto::Overlap>* overlaps,
                          int agglomeration_distance,
                          int termination_count_threshold,
                          int max_deception_length,
                          int min_coverage)
{

  // This code at one time selected a random subset of overlaps to use for
  // chimera trimming. The idea was that many overlaps were redundant, so we
  // wouldn't lose anything by looking at a subset. However, sometimes there's
  // a single critical overlap that bridges what would otherwise be a break in
  // the assembly, and we can't risk excluding it.
//  std::vector<int> indices(orig_overlaps->size());
//  std::iota(indices.begin(), indices.end(), 0);
//  std::random_shuffle(indices.begin(), indices.end());
//  indices.resize(std::min(indices.size(), 300ul));
//
//  std::vector<proto::Overlap> _overlaps;
//  for(auto index : indices) {
//    _overlaps.push_back((*orig_overlaps)[index]);
//  }
//  std::vector<proto::Overlap>* overlaps = &_overlaps;
  
  if(DEBUG) {
    std::cerr << "Before any trimming" << std::endl;
    std::cerr << overlap_debug_string(*overlaps, 120) << std::endl;
  }

  // First, find the positions of overlap terminations 
  auto terminations_from_left = identify_terminating_overlaps(
      *overlaps, TerminationDirection::FROMTHELEFT);
  auto terminations_from_right = identify_terminating_overlaps(
      *overlaps, TerminationDirection::FROMTHERIGHT);
  
  if(DEBUG) { 
    std::cerr << "Terminations from the left:" << std::endl;
    for(auto term : terminations_from_left) {
      std::cerr << term.first << " ";
    }
    std::cerr << std::endl;
    std::cerr << "Terminations from the right:" << std::endl;
    for(auto term : terminations_from_right) {
      std::cerr << term.first << " ";
    }
    std::cerr << std::endl;
  }

  // Second, combine those termination positions into intervals
  auto left_termination_intervals = create_termination_intervals(
      terminations_from_left, TerminationDirection::FROMTHELEFT,
      agglomeration_distance, termination_count_threshold);
  auto right_termination_intervals = create_termination_intervals(
      terminations_from_right, TerminationDirection::FROMTHERIGHT,
      agglomeration_distance, termination_count_threshold);
  
  if(DEBUG) { 
    std::cerr << "Left termination intervals" << std::endl;
    for(auto ti : left_termination_intervals) {
      std::cerr << "(" << ti.start << ", " << ti.end << ") ";
    }
    std::cerr << std::endl; 
    std::cerr << "Right termination intervals" << std::endl;
    for(auto ti : right_termination_intervals) {
      std::cerr << "(" << ti.start << ", " << ti.end << ") ";
    }
    std::cerr << std::endl; 
  }
  // Third, trim overlaps that end in a termination interval back to the start
  // of the interval
  trim_terminating_overlaps(overlaps, left_termination_intervals);
  trim_terminating_overlaps(overlaps, right_termination_intervals);
  
  if(DEBUG) { 
    std::cerr << "After trimming terminating" << std::endl;
    std::cerr << overlap_debug_string(*overlaps, 120) << std::endl;
  }
  // Fourth, trim overlaps that extend just past a termination region to
  // the end of the partner read 
  trim_deceptive_overlaps(overlaps, left_termination_intervals,
                          max_deception_length);
  trim_deceptive_overlaps(overlaps, right_termination_intervals,
                          max_deception_length);
  
  if(DEBUG) {  
    std::cerr << "After trimming deceptive" << std::endl;
    std::cerr << overlap_debug_string(*overlaps, 120) << std::endl;
  }
  // Fifth, find termination intervals with insufficient coverage
  auto all_termination_intervals = left_termination_intervals;
  all_termination_intervals.insert(all_termination_intervals.end(),
      right_termination_intervals.begin(), right_termination_intervals.end());
  
  if(DEBUG) { 
    std::cerr << "All termination intervals" << std::endl;
    for(auto ti : all_termination_intervals) {
      std::cerr << "(" << ti.start << ", " << ti.end << ") ";
    }
    std::cerr << std::endl; 
  }

  auto spanned_intervals = find_spanned_intervals(
      all_termination_intervals, *overlaps, min_coverage);
  
  if(DEBUG) {
    std::cerr << "Spanned intervals" << std::endl;
    for(auto ti : spanned_intervals) {
      std::cerr << "(" << ti.first << ", " << ti.second << ") ";
    }
    std::cerr << std::endl; 
  }

  // Finally, trim the overlaps to the the good boundaries and return a read
  // object that contains the trimming information
  proto::Read trimmed_read = trim_to_largest_spanned_interval(*overlaps, spanned_intervals);
  return trimmed_read;
}
