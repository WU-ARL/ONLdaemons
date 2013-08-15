/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/RangeMap.h,v $
 * $Author: mah5 $
 * $Date: 2007/10/19 03:46:23 $
 * $Revision: 1.5 $
 * $Name:  $
 *
 * File:   RangeMap.h
 * Author: Fred Kuhns
 * Email:  fredk@cse.wustl.edu
 * Organization: Washington University in St. Louis
 * 
 * Created:  07/13/07 14:59:04 CDT
 * 
 * Description:
 */
#include <set>
#include <map>
#include <stdexcept>
#include <stdio.h>

// ------------------------ Range Class ------------------------------
struct Range {
  // Defines a contiguous range of integer values
  //     start_ <= integer < end_ or [start_, end_)
  int start_, end_;

  Range() : start_(0), end_(0) {}
  Range(const Range& other) : start_(other.start_), end_(other.end_) {}
  Range(int start, int cnt) : start_(start), end_(start+cnt) {}
  // Allow simple default ranges starting at 'start'
  //   [start, start+1, ..., start+cnt)
  Range(int cnt) : start_(0), end_(cnt) {}
  virtual ~Range() {}

  // ************************** const methods *************************
  // Since the value of end_ is one past the max value in the range the
  // length is simply end - start.
  int len() const {return end_ - start_;}
  // returns true if ranges do not overlap
  // Overlaps if our end_ value is >= other.start or 
  //             our start_ value is < other.end_
  bool can_merge(const Range &other) const { return start_ <= other.end_ || end_ >= other.start_; }
  // returns true if end_ == start_
  bool empty() const {return start_ == end_; }
  bool operator!() const {return empty();}

  // If the two ranges overlap or are contiguous (x.end_ == y.start_) then
  // combine into one range. If sets may not be merged then throws an
  // exception.
  Range &merge(const Range &);

  // remove the first 'cnt' elements from the Range.  Thie range of this
  // object is reduces:
  //   Range obj(10); {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}
  //   Range tmp = obj.slice(4);
  //     tmp = {0, 1, 2, 3}; tmp.start_ = 0; tmp.end_ = 4
  //     obj = {4, 5, 6, 7, 8, 9}; obj.start_ = 4; obj.end_ = 10
  //  BTW, cnt should never be grate then this->len() ... currently I just
  //  pretend that cnt == this->len().
  Range slice(int cnt);

  void clear() {start_ = 0; end_ = 0;}
};

std::ostream &operator<<(std::ostream &os, const Range &ref);
// Enforce Strick Weak Ordering on Range class (only define <)
bool operator<(const Range &lhs, const Range &rhs);

// ---------------------------- RangeMap Class ------------------------------
// You can add to or clear but you can't remove individual ranges.
template <class T>
class RangeMap {
  public:
    typedef std::map<Range, T> map_type;
    typedef Range key_type;
  private:
    map_type map_;
    int  start_;
    int  end_;
    int  next_;
  // Maps a contiguous range of integers to another possibly discontinuous range:
  //   RangeMap obj(10);  '{0,1, ..., 9}' -> NULL
  //   Range r1(5, 5),  {5, 6, 7, 8, 9}
  //   Range r2(20, 5), {20, 21, 22, 23, 24}
  //   obj.add(r1)
  //     obj = { [0, 5) -> [5, 10) }
  //   obj.add(r2)
  //     obj = { [0, 5) -> [5, 10), [5, 10) -> [20, 25) }
  //
  public:

    RangeMap() : start_(0), end_(0),   next_(0) {}
    RangeMap(int cnt) : start_(0), end_(cnt), next_(0) {}
    RangeMap(int start, int cnt) : start_(start), end_(start+cnt), next_(start) {}

    // operates on const objects
    int  len()   const {return end_ - start_;}
    int  space() const {return end_ - next_; }
    const map_type& map() const {return map_;}

    // Assume the value type also represents a range of values.
    int convert(int val) const;

    typename map_type::const_iterator begin() const {return map_.begin();}
    typename map_type::const_iterator end()   const {return map_.end();}
    typename map_type::const_iterator find(int val) const { return map_.find(Range(val,1)); }
    typename map_type::const_iterator find(const key_type &ref) const { return map_.find(ref); }

    // Either make T a pointer or passs a pointer as the parameter
    T lookup(const key_type &key)
    {
      typename map_type::iterator iter = map_.find(key);
      if (iter == map_.end())
        return NULL;
      return iter->second;
    }
    // modifies object
    void set(int cnt) { set(0, cnt); } 
    void set(int start, int cnt) {
      map_.clear();
      start_ = start;
      end_   = start_ + cnt;
      next_  = start;
      return;
    }
    void grow(int cnt)
    {
      if (cnt <= 0) return;
      end_ += cnt;
      return;
    }
    void reset() { set(start_, len()); }
    void clear() { set(0, 0); }
    void add(T &ref);
    void push(const Range &key, T val)
    {
      // XXX you want to make sure that this Range is not already in the
      // mapping
      //if (!
	 map_.insert(make_pair(key, val)).second;
	//)
        //continue;//throw XXX;
      return;
    }

};

template <class T>
std::ostream &operator<<(std::ostream &os, const RangeMap<T> &rm);

class RangeSet;

std::ostream &operator<<(std::ostream &os, const RangeSet &rs);

// ----------------------------- RangeSet --------------------------------------
class RangeSet {
  // Should really be called the FreeList class.
  // set_ := { Range1, Range2, ..., }
  public:
    typedef std::set<Range> set_type;

    RangeSet(int cnt) : start_(0), end_(cnt)
    {
      if (cnt > 0) {
        if (! set_.insert(Range(cnt)).second) {
          printf("RangeSet: Error initializing RangeSet with count %d\n", cnt);
          end_ = 0;
        }
      } else {
        end_ = 0;
      }
    }
    void grow(int cnt)
    {
      int old_end = end_;

      if (cnt <= 0) return;

      try {
        end_ += cnt;
        add(Range(old_end, cnt));
      } catch(...) {
        end_ = old_end;
      }
    }
    //void add(const Range &ref);
    void add(const Range &other)
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
    template <class T> RangeMap<T> &alloc(RangeMap<T> &rmap);
    template <class T> void free(RangeMap<T> &rmap);

    // operate on const object
    set_type::const_iterator begin() const {return set_.begin();}
    set_type::const_iterator end() const {return set_.end();}

  private:
    friend std::ostream &operator<<(std::ostream &os, const RangeSet &rs);
    set_type set_;
    int start_, end_;
};

#include <wuPP/RangeMap.i>
