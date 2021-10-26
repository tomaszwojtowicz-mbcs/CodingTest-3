/***************************************************************************
* © 2019 Cadence Design Systems, Inc.  All rights reserved.                *
* This work may not be copied, modified, re-published, uploaded, executed, *
* or distributed in any way, in any medium, whether in whole or in part,   *
* without prior written permission from Cadence Design Systems, Inc.       *
****************************************************************************/


#include <iostream>
#include <limits>
#include <vector>
#include <stack>
#include "test.h"

// Represents a closed interval [min, max]
// * Enforces min < max
//
// * Note: No it doesn't - the constructor, as it's implemented enforces min <= max. Will leave it as it is to support degenerate intervals
class Interval {
public:
    typedef long int Integer;

    Interval(Integer min, Integer max) : _min(min), _max(max) {
        if (_max < _min) {
            std::swap(_min, _max);
        }
    }

    Integer Min() const { return _min; }
    Integer Max() const { return _max; }

    // Added a setter as the alternative to modyfying an Interval would be inserting & removing elements during merging
    // * I find this aproach both cleaner and faster (than shifting elements in vector)
    void SetMax(Integer max) {
        // Note: assuming that degenerate intervals (e.g [1, 1]) are permitted, hence >=
        if (max >= _min) [[likely]] {
            _max = max;
        }
        else {
            throw std::invalid_argument("Attempting to set max that is less than current min");
        }
    }

    // Overloading < operator will allow the use of std::sort on Interval
    bool operator < (const Interval  &other) const {
        return (_min < other.Min());
    }

private:
    Integer _min;
    Integer _max;
};

// Overloading == operator will allow for easy (in terms of syntax, at least) comparing vector<Interval>
bool operator == (const Interval& lhs, const Interval& rhs) {
    return (lhs.Min() == rhs.Min() && lhs.Max() == rhs.Max());
}

// Merges overlapping Intervals and returns them in a vector
std::vector<Interval> MergeIntervals(std::vector<Interval> &intervals) {
    // Construct output vector from 1st element of intervals
    std::vector<Interval> output (1, intervals.front());

    for (size_t i = 1; i < intervals.size(); ++i) {
        Interval& lastInterval = output.back();

        // Check if Intervals are overlaping: e.g the min of a given Interval is <= max of the preceeding Interval +1
        // * The +1 is there to account for Intervals being a closed integral intervals
        // * so [-1, 1] and [2, 5] do overlap
        if (lastInterval.Max() + 1 < intervals[i].Min()) {
            output.push_back(intervals[i]);
        }
        else if (lastInterval.Max() < intervals[i].Max()) {
            lastInterval.SetMax(intervals[i].Max());
        }
    }

    return output;
}

// Returns true if every element of Interval under test is contained in the union of Intervals in the vector
bool IsIntervalInUnionOfOthers(const Interval &interval, const std::vector<Interval> &intervals) {
    if (intervals.empty()) {
        return false;
    }

    // Need a local copy due to intervals being const reference
    // * The prefered aproach would be taking a non-const reference, which seems acceptable in this particular exercise
    std::vector<Interval> intervalsCopy(intervals);

    // Sort the list of intervals by the min, in ascending order
    // * as having an ordered elements simplifies merging of Intervals
    std::sort(intervalsCopy.begin(), intervalsCopy.end());

    intervalsCopy = MergeIntervals(intervalsCopy);

    // Copy-constructing another vector to hold all the Interval collection as well as Interval under test
    std::vector<Interval> allIntervals(intervalsCopy);

    // Insert the Interval under test at the correct position in the vector to preserve its ordering
    allIntervals.insert(lower_bound(allIntervals.begin(), allIntervals.end(), interval), interval);

    // Merge all Intervals, then check if if the resulting vector differs from intervalsCopy
    // * If it does, there must have been elements of Interval under test that were not present in the collection of Intervals
    return (MergeIntervals(allIntervals) == intervalsCopy);
}

int main(int, char **) {
    // Added couple of tests related to degenerate intervals
    {
        TEST("Degenerate interval is in the union of the collection");
        Interval interval{2, 2};
        std::vector<Interval> others{ {0, 2}, {4, 7}};
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval, others), true);
    }

    {
        TEST("Degenerate interval is not in the union of the collection");
        Interval interval{2, 2};
        std::vector<Interval> others{ {0, 1}, {4, 7}};
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval, others), false);
    }

    {
        TEST("Return false on empty collection");
        Interval interval {-5, 9};
        std::vector<Interval> others;
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval, others), false);
    }

    {
        TEST("Interval larger than single others interval, no overlap");
        Interval interval {5, 7};
        std::vector<Interval> others{ {0, 1} };
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval, others), false);
    }

    {
        TEST("Interval smaller than single others interval, no overlap");
        Interval interval {-2000000, -1500000};
        std::vector<Interval> others{ {2000, 3000} };
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval, others), false);
    }

    {
        TEST("Interval is in between two intervals");
        Interval interval{3, 6};
        std::vector<Interval> others{ {0, 2}, {4, 7}};
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval, others), false);
    }

    {
        TEST("Interval endpoints in two intervals");
        Interval interval{2, 4};
        std::vector<Interval> others{ {0, 2}, {4, 7}};
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval, others), false);
    }

    {
        TEST("Interval is in union of itself");
        Interval interval {0, 3};
        std::vector<Interval> others { interval };
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval, others), true);
    }

    {
        TEST("Interval is in union of overlapping intervals");
        Interval interval {0, 3};
        std::vector<Interval> others { {-1, 2}, {2, 5} };
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval, others), true);
    }

    {
        TEST("Interval is in union of disjoint intervals");
        Interval interval {0, 3};
        std::vector<Interval> others { {-5, -2}, {5, 10}, {0,2}, {2,3} };
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval, others), true);
    }

    {
        TEST("Interval is in union of adjacent intervals");
        Interval interval {0, 3};
        std::vector<Interval> others { {-1, 1}, {2, 5} };
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval, others), true);
    }

    {
        TEST("Interval is in union of point intervals");
        Interval interval {5, 9};
        std::vector<Interval> others;
        for (int v : {100, 5, -7, 6, 9, 12, 4, 8, 101, 7}) {
            others.emplace_back( Interval {v, v} );
        }
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval, others), true);
    }

    {
        TEST("Interval is in union of overlapping intervals");
        Interval interval {1, 7};
        std::vector<Interval> others { {5, 10}, {0, 6}, {2, 3} };
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval, others), true);
    }

    {
        TEST("Interval is in large redundant union");
        Interval interval1 {5, 9};
        Interval interval2 {999995, 999995};
        Interval interval3 {-999995, -999995};
        Interval interval4 {-300000, 300000};
        std::vector<Interval> others;
        for (int v = -1000000; v < 1000000; ++v ) {
            others.emplace_back( Interval {-1000000 + v, 1000000 + v} );
        }
        RandomizeVectorOrder(others);
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval1, others), true);
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval2, others), true);
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval3, others), true);
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval4, others), true);
    }

    {
        TEST("Interval is in large union");
        Interval interval1 {5, 9};
        Interval interval2 {999995, 999995};
        Interval interval3 {-999995, -999995};
        Interval interval4 {-300000, 300000};
        std::vector<Interval> others;
        for (int v = -1000000; v < 1000000; ++v ) {
            others.emplace_back( Interval {v, v} );
        }
        RandomizeVectorOrder(others);
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval1, others), true);
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval2, others), true);
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval3, others), true);
        ASSERT_EQ(IsIntervalInUnionOfOthers(interval4, others), true);
    }

    {
        TEST("Interval is wide");
        try {
            Interval interval {std::numeric_limits<Interval::Integer>::min(),
                std::numeric_limits<Interval::Integer>::max()};

            std::vector<Interval> others;
            others.emplace_back(std::numeric_limits<Interval::Integer>::min(), 0);
            others.emplace_back(-1, std::numeric_limits<Interval::Integer>::max());

            ASSERT_EQ(IsIntervalInUnionOfOthers(interval, others), true);
        } catch (...) {
            // I.e. fail if we get an exception
            ASSERT_EQ(false, true);
        }
    }

    return 0;
}