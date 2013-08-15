/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/RangeMap.cc,v $
 * $Author: mah5 $
 * $Date: 2007/10/19 03:46:23 $
 * $Revision: 1.4 $
 * $Name:  $
 *
 * File:   try.cc
 * Author: Fred Kuhns
 * Email:  fredk@cse.wustl.edu
 * Organization: Washington University in St. Louis
 * 
 * Created:  07/13/07 14:59:04 CDT
 * 
 * Description:
 */
#include <iostream>
#include <exception>
#include "./RangeMap.h"

using namespace std;

ostream &operator<<(ostream &os, const Range &other)
{
  os << "[" << other.start_ << ", " << other.end_ << ")";
  return os;
}

Range& Range::merge(const Range &other)
{
  if (this == &other) return *this;

  if (empty()) {
    *this = other;
  } else if (can_merge(other)) {
    start_ = std::min(start_, other.start_);
    end_   = std::max(end_, other.end_);
  } else {
    throw std::runtime_error("Range: attempting to merge disjoint (isolated) ranges.");
  }
  return *this;
}

Range Range::slice(int cnt)
{
  Range tmp;
  if (cnt >= len()) {
    tmp = *this;
    clear();
  } else {
    tmp.start_ = start_;
    tmp.end_   = start_ + cnt;
    start_    += cnt;
  }
  return tmp;
}

// Enforce Strick Weak Ordering on Range class (only define <)
bool operator<(const Range &lhs, const Range &rhs)
{
  // XXX std::cout << "lhs.end_ " << lhs.end_ << " rhs.start_ " << rhs.start_ << std::endl;
  return lhs.end_ <= rhs.start_;
}

// ----------------------------- RangeSet --------------------------------------
ostream &operator<<(ostream &os, const RangeSet &rs)
{
  RangeSet::set_type::const_iterator iter;
  for (iter = rs.begin(); iter != rs.end(); ++iter)
    os << *iter << " ";
  return os;
}

#if 0
void RangeSet::add(const Range &other)
{
  RangeSet::set_type::iterator iter;
  Range combined = other;

  if (other.end_ > end_)
    throw std::runtime_error("RangeSet: Error adding range ... extends beyond set");

  // XXX std::cout << "Adding range: " << other << "\n";
  // look for any existing Ranges which can be merged with combine.
  while ((iter = set_.find(combined)) != set_.end()) {
    combined.merge(*iter);
    set_.erase(iter);
  }
  // now put the merged Range back into the RangeSet.
  if (! set_.insert(combined).second)
    throw std::runtime_error("Error adding combined range to range set");
}
#endif
