/*
 * $Source: /project/arlcvs/cvsroot/wu_arl/wusrc/wuPP/wuset.h,v $
 * $Author: fredk $
 * $Date: 2008/09/17 14:05:56 $
 * $Revision: 1.1 $
 * $Name:  $
 *
 * File:   try.cc
 * Author: Fred Kuhns
 * Email:  fredk@cse.wustl.edu
 * Organization: Washington University in St. Louis
 * 
 * Created:  09/02/08 17:02:24 CDT
 * 
 * Copyright (C) 2008 Fred Kuhns and Washington University in St. Louis.
 * All rights reserved.
 *
 * Description:
 *
 */

#ifndef _WUHEAP_H
#define _WUHEAP_H

#include <set>
#include <algorithm>
#include <exception>
#include <stdexcept>

namespace wupp {

  template <class T, class CMP>
  class wuset {
    private:
      std::multiset<T, CMP> table_;
      CMP cmp;
    public:
      typedef typename std::multiset<T>::iterator iterator;
      typedef typename std::multiset<T>::const_iterator const_iterator;

      iterator begin() { return table_.begin(); }
      iterator end() { return table_.end(); }
      const_iterator begin() const { return table_.begin(); }
      const_iterator end() const { return table_.end(); }
      size_t size() const { return table_.size(); }
      bool   empty() const { return table_.empty(); }
      void   remove(const T &obj);
      T      top() const { return *table_.begin(); }
      void   push(const T &obj) { table_.insert(obj); }
      T      pop();
  };

template <class T, class CMP>
T wupp::wuset<T,CMP>::pop()
{ 
  if (table_.empty()) throw std::runtime_error("wuset: pop called on empty set");
  T tmp = *table_.begin();
  table_.erase(tmp);
  return tmp;
}

template <class T, class CMP>
void wupp::wuset<T,CMP>::remove(const T &obj)
{
  iterator iter;
  std::pair< iterator, iterator > ret = table_.equal_range(obj);
  for (iter = ret.first ; iter != ret.second; ++iter) {
    // Note: the comparator function used to sort the list enforces strict
    // weak ordering and thus only uses the '<' operator. So here I explicitly
    // require the equality operator to work properly.
    if (*iter == obj) break;
  }

  if (iter == ret.second) return; // not found
  table_.erase(iter);
}

};
#endif
