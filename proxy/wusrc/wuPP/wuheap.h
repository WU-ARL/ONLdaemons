/*
 * $Source: /project/arlcvs/cvsroot/wu_arl/wusrc/wuPP/wuheap.h,v $
 * $Author: fredk $
 * $Date: 2008/09/16 22:06:18 $
 * $Revision: 1.3 $
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

#include <vector>
#include <vector>
#include <algorithm>
#include <exception>
#include <stdexcept>

namespace wupp {

  template <class T, class CMP>
  class wuheap {
    private:
      std::vector<T> table_;
      CMP cmp;
    public:
      typedef typename std::vector<T>::iterator iterator;
      typedef typename std::vector<T>::const_iterator const_iterator;

      iterator begin() { return table_.begin(); }
      iterator end() { return table_.end(); }
      const_iterator begin() const { return table_.begin(); }
      const_iterator end() const { return table_.end(); }
      size_t size() const { return table_.size(); }
      bool   empty() const { return table_.empty(); }
      void   remove(const T &obj);
      T      top() const { return table_[0]; }
      void   push(const T &obj);
      T      pop();
      void   heapify();
  };


template <class T, class CMP>
void wupp::wuheap<T,CMP>::push(const T &obj)
{
  if (table_.empty()) {
    table_.push_back(obj);
    heapify();
  } else {
    table_.push_back(obj);
    std::push_heap(table_.begin(), table_.end(), cmp);
  }
  return;
}

template <class T, class CMP>
T wupp::wuheap<T,CMP>::pop()
{ 
  if (table_.empty())
    throw std::runtime_error("wuheap: pop called on empty heap");

  T tmp = table_[0];
  std::pop_heap(table_.begin(), table_.end(), cmp);
  table_.pop_back();
  return tmp;
}

template <class T, class CMP>
void wupp::wuheap<T,CMP>::heapify()
{
  if (table_.empty()) return;

  std::make_heap(table_.begin(), table_.end(), cmp);
}

template <class T, class CMP>
void wupp::wuheap<T,CMP>::remove(const T &obj)
{
  iterator iter = find(table_.begin(), table_.end(), obj);
  if (iter == table_.end()) return;

  *iter = *(table_.end() - 1);
  table_.pop_back();
  heapify();
}

};
#endif
