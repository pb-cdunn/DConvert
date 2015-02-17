#include "gtest/gtest.h"
#include "Trimmer_private.h"
#include "Overlap.pb.h"

#include <algorithm>

// Test functions that determine termination direction
TEST(TestTrimmer, TerminationDirectionTest)
{
  proto::Overlap ovl;

  // \>>>>>>>>>/          2
  // ***************      1
  ovl.set_length_1(100);
  ovl.set_length_2(250);
  ovl.set_start_1(0);
  ovl.set_end_1(50);
  ovl.set_start_2(50);
  ovl.set_end_2(100);
  ovl.set_forward(true);

  EXPECT_TRUE(terminates_from_left(ovl));
  EXPECT_FALSE(terminates_from_right(ovl));
  
  //    \>>>>>>>>/        2
  // ***************      1
  ovl.set_length_1(100);
  ovl.set_length_2(250);
  ovl.set_start_1(25);
  ovl.set_end_1(75);
  ovl.set_start_2(50);
  ovl.set_end_2(100);
  ovl.set_forward(true);

  EXPECT_TRUE(terminates_from_left(ovl));
  EXPECT_TRUE(terminates_from_right(ovl));
  
  //      \>>>>>>>>/      2
  // ***************      1
  ovl.set_length_1(100);
  ovl.set_length_2(250);
  ovl.set_start_1(50);
  ovl.set_end_1(100);
  ovl.set_start_2(50);
  ovl.set_end_2(100);
  ovl.set_forward(true);

  EXPECT_FALSE(terminates_from_left(ovl));
  EXPECT_TRUE(terminates_from_right(ovl));
  
  //       >>>>>>>>/      2
  // ***************      1
  ovl.set_length_1(100);
  ovl.set_length_2(250);
  ovl.set_start_1(50);
  ovl.set_end_1(100);
  ovl.set_start_2(0);
  ovl.set_end_2(50);
  ovl.set_forward(true);

  EXPECT_FALSE(terminates_from_left(ovl));
  EXPECT_FALSE(terminates_from_right(ovl));
  
  //    >>>>>>>>          2
  // ***************      1
  ovl.set_length_1(100);
  ovl.set_length_2(50);
  ovl.set_start_1(50);
  ovl.set_end_1(100);
  ovl.set_start_2(0);
  ovl.set_end_2(50);
  ovl.set_forward(true);

  EXPECT_FALSE(terminates_from_left(ovl));
  EXPECT_FALSE(terminates_from_right(ovl));
  
  //     <<<<<</          2
  // ***************      1
  ovl.set_length_1(100);
  ovl.set_length_2(200);
  ovl.set_start_1(25);
  ovl.set_end_1(75);
  ovl.set_start_2(150);
  ovl.set_end_2(200);
  ovl.set_forward(false);

  EXPECT_TRUE(terminates_from_left(ovl));
  EXPECT_FALSE(terminates_from_right(ovl));
}

// Test the function that tells whether an overlap extends to the end of the
// partner read
TEST(TestTrimmer, ExtendsToEndTest)
{

  proto::Overlap ovl;

  // \>>>>>>>>>           2
  // ***************      1
  ovl.set_length_1(100);
  ovl.set_length_2(150);
  ovl.set_start_1(0);
  ovl.set_end_1(50);
  ovl.set_start_2(100);
  ovl.set_end_2(150);
  ovl.set_forward(true);

  EXPECT_TRUE(extends_to_end(ovl, TerminationDirection::FROMTHELEFT));
  EXPECT_FALSE(extends_to_end(ovl, TerminationDirection::FROMTHERIGHT));

  // \>>>>>>>/            2
  // ***************      1
  ovl.set_length_1(100);
  ovl.set_length_2(150);
  ovl.set_start_1(0);
  ovl.set_end_1(50);
  ovl.set_start_2(50);
  ovl.set_end_2(100);
  ovl.set_forward(true);

  EXPECT_FALSE(extends_to_end(ovl, TerminationDirection::FROMTHELEFT));
  EXPECT_FALSE(extends_to_end(ovl, TerminationDirection::FROMTHERIGHT));

  //    \<<<<<</          2
  // ***************      1
  ovl.set_length_1(100);
  ovl.set_length_2(150);
  ovl.set_start_1(25);
  ovl.set_end_1(75);
  ovl.set_start_2(50);
  ovl.set_end_2(100);
  ovl.set_forward(false);

  EXPECT_FALSE(extends_to_end(ovl, TerminationDirection::FROMTHELEFT));
  EXPECT_FALSE(extends_to_end(ovl, TerminationDirection::FROMTHERIGHT));

  //    <<<<<<<<          2
  // ***************      1
  ovl.set_length_1(100);
  ovl.set_length_2(50);
  ovl.set_start_1(25);
  ovl.set_end_1(75);
  ovl.set_start_2(0);
  ovl.set_end_2(50);
  ovl.set_forward(false);

  EXPECT_TRUE(extends_to_end(ovl, TerminationDirection::FROMTHELEFT));
  EXPECT_TRUE(extends_to_end(ovl, TerminationDirection::FROMTHERIGHT));

  //    >>>>>>>>/         2
  // ***************      1
  ovl.set_length_1(100);
  ovl.set_length_2(150);
  ovl.set_start_1(25);
  ovl.set_end_1(75);
  ovl.set_start_2(0);
  ovl.set_end_2(50);
  ovl.set_forward(true);

  EXPECT_FALSE(extends_to_end(ovl, TerminationDirection::FROMTHELEFT));
  EXPECT_TRUE(extends_to_end(ovl, TerminationDirection::FROMTHERIGHT));

  //    <<<<<<<</         2
  // ***************      1
  ovl.set_length_1(100);
  ovl.set_length_2(150);
  ovl.set_start_1(25);
  ovl.set_end_1(75);
  ovl.set_start_2(100);
  ovl.set_end_2(150);
  ovl.set_forward(false);

  EXPECT_FALSE(extends_to_end(ovl, TerminationDirection::FROMTHELEFT));
  EXPECT_TRUE(extends_to_end(ovl, TerminationDirection::FROMTHERIGHT));
}

TEST(TestTrimmer, TrimOverlapTest)
{
  proto::Overlap ovl;
  //    >>>>>>>>/         2
  // ***************      1
  //         ^    ^
  ovl.set_length_1(100); ovl.set_length_2(200);
  ovl.set_start_1(25); ovl.set_end_1(75);
  ovl.set_start_2(0); ovl.set_end_2(50);
  ovl.set_forward(true);
  TerminationInterval ti{60, 80, TerminationDirection::FROMTHELEFT};
  trim_overlap_to_interval(&ovl, ti, TerminationDirection::FROMTHELEFT);

  EXPECT_EQ(ovl.length_1(), 100); EXPECT_EQ(ovl.length_2(), 200);
  EXPECT_EQ(ovl.start_1(), 25); EXPECT_EQ(ovl.start_2(), 0);
  EXPECT_EQ(ovl.end_1(), 60); EXPECT_EQ(ovl.end_2(), 35);

  //    >>>>>>>>/         2
  // ***************      1
  //  ^           ^
  ovl.set_length_1(100); ovl.set_length_2(200);
  ovl.set_start_1(25); ovl.set_end_1(75);
  ovl.set_start_2(0); ovl.set_end_2(50);
  ovl.set_forward(true);
  ti = TerminationInterval{20, 80, TerminationDirection::FROMTHELEFT};
  trim_overlap_to_interval(&ovl, ti, TerminationDirection::FROMTHELEFT);

  EXPECT_EQ(ovl.length_1(), 100); EXPECT_EQ(ovl.length_2(), 200);
  EXPECT_EQ(ovl.start_1(), 25); EXPECT_EQ(ovl.start_2(), 0);
  EXPECT_EQ(ovl.end_1(), 25); EXPECT_EQ(ovl.end_2(), 0);
  
  //    \<<<<<<</         2
  // ***************      1
  //  ^     ^
  ovl.set_length_1(100); ovl.set_length_2(200);
  ovl.set_start_1(25); ovl.set_end_1(75);
  ovl.set_start_2(10); ovl.set_end_2(60);
  ovl.set_forward(false);
  ti = TerminationInterval{10, 36, TerminationDirection::FROMTHERIGHT};
  trim_overlap_to_interval(&ovl, ti, TerminationDirection::FROMTHERIGHT);

  EXPECT_EQ(ovl.length_1(), 100); EXPECT_EQ(ovl.length_2(), 200);
  EXPECT_EQ(ovl.start_1(), 35); EXPECT_EQ(ovl.start_2(), 10);
  EXPECT_EQ(ovl.end_1(), 75); EXPECT_EQ(ovl.end_2(), 50);
}

class HoleInMiddleTest : public ::testing::Test
{
  // \>>>>>>>>>/
  // \>>>>>>>>>/
  // <<<<<<<<<</
  // \<<<<<<<
  // >>>>>>>>>>>>>>>>>/
  // <<<<<<<<<<<<<<<</
  // >>>>>>>>>>>>>>>>>>/
  // \>>>>>>>>>>>>>>>>>>>>
  //  <<<<<<<<</
  //   >>>>>>
  //      >>>>>/
  //                          \>>>>>>>/
  //                           \<<<<
  // **********************************
public:
  std::vector<proto::Overlap> ovls;

  virtual void SetUp()
  {
    proto::Overlap ovl;
    ovl.set_length_1(1000);
    ovl.set_length_2(800);

    ovl.set_start_1(0); ovl.set_end_1(250); ovl.set_start_2(500); ovl.set_end_2(750); ovl.set_forward(true);
    ovls.push_back(ovl);
    ovl.set_start_1(0); ovl.set_end_1(250); ovl.set_start_2(250); ovl.set_end_2(500); ovl.set_forward(true);
    ovls.push_back(ovl);
    ovl.set_start_1(0); ovl.set_end_1(250); ovl.set_start_2(550); ovl.set_end_2(800); ovl.set_forward(false);
    ovls.push_back(ovl);
    ovl.set_start_1(0); ovl.set_end_1(200); ovl.set_start_2(0); ovl.set_end_2(200); ovl.set_forward(false);
    ovls.push_back(ovl);
    ovl.set_start_1(0); ovl.set_end_1(600); ovl.set_start_2(0); ovl.set_end_2(600); ovl.set_forward(true);
    ovls.push_back(ovl);
    ovl.set_start_1(0); ovl.set_end_1(590); ovl.set_start_2(210); ovl.set_end_2(800); ovl.set_forward(false);
    ovls.push_back(ovl);
    ovl.set_start_1(0); ovl.set_end_1(610); ovl.set_start_2(0); ovl.set_end_2(610); ovl.set_forward(true);
    ovls.push_back(ovl);
    ovl.set_start_1(0); ovl.set_end_1(650); ovl.set_start_2(150); ovl.set_end_2(800); ovl.set_forward(true);
    ovls.push_back(ovl);
    ovl.set_start_1(50); ovl.set_end_1(255); ovl.set_start_2(595); ovl.set_end_2(800); ovl.set_forward(false);
    ovls.push_back(ovl);
    ovl.set_start_1(100); ovl.set_end_1(245); ovl.set_start_2(0); ovl.set_end_2(145); ovl.set_length_2(145); ovl.set_forward(true);
    ovls.push_back(ovl);
    ovl.set_start_1(150); ovl.set_end_1(250); ovl.set_start_2(0); ovl.set_end_2(100); ovl.set_forward(true);
    ovls.push_back(ovl);
    ovl.set_start_1(800); ovl.set_end_1(1000); ovl.set_start_2(200); ovl.set_end_2(400); ovl.set_forward(true);
    ovls.push_back(ovl);
    ovl.set_start_1(810); ovl.set_end_1(950); ovl.set_start_2(0); ovl.set_end_2(140); ovl.set_forward(false);
    ovls.push_back(ovl);
  }
};

TEST_F(HoleInMiddleTest, TestTerminationIdentification)
{
  auto from_the_left = identify_terminating_overlaps(ovls, TerminationDirection::FROMTHELEFT);
  auto from_the_right = identify_terminating_overlaps(ovls, TerminationDirection::FROMTHERIGHT);
  
  EXPECT_EQ(from_the_left.size(), 8);
  EXPECT_EQ(from_the_right.size(), 2);

  std::sort(from_the_left.begin(), from_the_left.end());
  decltype(from_the_left) expected_left = {250, 250, 250, 250,
                                           255, 590, 600, 610};
  EXPECT_EQ(from_the_left, expected_left);

  std::sort(from_the_right.begin(), from_the_right.end());
  decltype(from_the_right) expected_right = {800, 810};
  EXPECT_EQ(from_the_right, expected_right);

}

TEST_F(HoleInMiddleTest, TestTerminationIntervals)
{
  auto from_the_left = identify_terminating_overlaps(ovls, TerminationDirection::FROMTHELEFT);
  auto from_the_right = identify_terminating_overlaps(ovls, TerminationDirection::FROMTHERIGHT);


  auto left_agg25_thresh_1 = create_termination_intervals(from_the_left, TerminationDirection::FROMTHELEFT,
                                                          25, 1);
  decltype(left_agg25_thresh_1) expected_left_agg25_thresh_1 = {
    TerminationInterval{250, 256, TerminationDirection::FROMTHELEFT},
    TerminationInterval{590, 611, TerminationDirection::FROMTHELEFT}};

  EXPECT_EQ(left_agg25_thresh_1, expected_left_agg25_thresh_1);

  auto right_agg25_thresh_1 = create_termination_intervals(from_the_right, TerminationDirection::FROMTHERIGHT,
                                                           25, 1);
  decltype(right_agg25_thresh_1) expected_right_agg25_thresh_1 = {
    TerminationInterval{800, 811, TerminationDirection::FROMTHERIGHT}};
  EXPECT_EQ(right_agg25_thresh_1, expected_right_agg25_thresh_1);

  auto left_agg5_thresh_2 = create_termination_intervals(from_the_left, TerminationDirection::FROMTHELEFT,
                                                         5, 2);
  decltype(left_agg5_thresh_2) expected_left_agg5_thresh_2 = {
    TerminationInterval{250, 256, TerminationDirection::FROMTHELEFT}};
  EXPECT_EQ(left_agg5_thresh_2, expected_left_agg5_thresh_2);
}

TEST_F(HoleInMiddleTest, TestTrimTerminatingOverlaps)
{
  auto from_the_left = identify_terminating_overlaps(ovls, TerminationDirection::FROMTHELEFT);
  auto from_the_right = identify_terminating_overlaps(ovls, TerminationDirection::FROMTHERIGHT);

  auto left_intervals = create_termination_intervals(from_the_left, TerminationDirection::FROMTHELEFT,
                                                     25, 1);
  auto right_intervals = create_termination_intervals(from_the_right, TerminationDirection::FROMTHERIGHT,
                                                      25, 1);

  trim_terminating_overlaps(&ovls, left_intervals);
  trim_terminating_overlaps(&ovls, right_intervals);

  auto ends_at_250 = std::count_if(ovls.begin(), ovls.end(),
                                    [](proto::Overlap o){return o.end_1() == 250;});
  EXPECT_EQ(ends_at_250, 5);

  auto ends_at_590 = std::count_if(ovls.begin(), ovls.end(), [](proto::Overlap o){return o.end_1() == 590;});
  EXPECT_EQ(ends_at_590, 3);
  
  auto starts_at_810 = std::count_if(ovls.begin(), ovls.end(), [](proto::Overlap o){return o.start_1() == 810;});
  EXPECT_EQ(starts_at_810, 2);
}

    
