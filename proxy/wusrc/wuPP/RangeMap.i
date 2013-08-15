/*
 * $Source: /project/arlcvs/cvsroot/wu_arl/wusrc/wuPP/RangeMap.i,v $
 * $Author: fredk $
 * $Date: 2008/08/24 23:15:43 $
 * $Revision: 1.2 $
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
#include <stdexcept>
#include <wuPP/error.h>

using namespace std;

// ---------------------------- RangeMap Class ------------------------------
template <class T>
ostream &operator<<(ostream &os, const RangeMap<T> &rm)
{
  typename RangeMap<T>::map_type::const_iterator iter = rm.map().begin();
  for (; iter != rm.map().end(); ++iter)
    os << "\t" <<  iter->first << " : " << iter->second << "\n";
  return os;
}

template <class T>
void RangeMap<T>::add(T &other)
{
  int cnt = std::min(other.len(), space());
  //std::cout << "RangeMap: Adding range (" << next_ << ") " << other << "\n";
  Range tmp(next_, cnt);
  //std::cout << "\t" << tmp << "\n";

  map_.insert(make_pair(tmp, other.slice(cnt)));
  next_ += cnt;

  return;
}

template <class T>
int RangeMap<T>::convert(int val) const
{
  typename RangeMap<T>::map_type::const_iterator iter = find(val);
  if (iter == map_.end())
    throw wupp::errorBase(wupp::libError, "RangeMap<T>::convert: No map exists");
  return (val - iter->first.start_) + iter->second.start_;
}

// ---------------------------- RangeSet Class ------------------------------

template <class T>
RangeMap<T> &RangeSet::alloc(RangeMap<T> &rmap)
{
  while(rmap.space() && ! set_.empty()) {
    set_type::iterator top = set_.begin();
    Range tmp = *top; // can't mutate set elements
    rmap.add(tmp);
    set_.erase(top);
    if (! tmp.empty() && ! set_.insert(tmp).second)
      throw wupp::errorBase(wupp::libError, "Error inserting entry in set_");
  }
  return rmap;
}

template <class T>
void RangeSet::free(RangeMap<T> &rmap)
{
  typename RangeMap<T>::map_type::const_iterator iter;
  for(iter = rmap.begin() ; iter != rmap.end() ; ++iter)
    add(iter->second);
  rmap.clear();
}
