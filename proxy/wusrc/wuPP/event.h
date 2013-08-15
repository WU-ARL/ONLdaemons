/*
 * $Source: /project/arlcvs/cvsroot/wu_arl/wusrc/wuPP/event.h,v $
 * $Author: mah5 $
 * $Date: 2008/10/14 18:29:24 $
 * $Revision: 1.6 $
 * $Name:  $
 *
 * File:   event.h
 * Author: Fred Kuhns
 * Email:  fredk@cse.wustl.edu
 * Organization: Washington University in St. Louis
 *
 * Created:  02/06/07 15:14:24 CST
 *
 * Modified: Jonathon Goldsmith<jas7@cec.wustl.edu>
 *
 * Description:
 */

#ifndef _WU_EVENT_H
#define _WU_EVENT_H

#include <wulib/wulog.h>

#include <wuPP/error.h>
#include <wuPP/timer.h>

#include <map>
#include <queue>
#include <wuPP/wuset.h>

namespace wupp {

  typedef void * eventID_t;
  const eventID_t invalid_enventID = 0;

  class eventNode_t { //eventNode base class, subclasses must override run
    private:
      struct timeval       tout_;
      struct timeval     period_;
      bool                valid_;
    public:
      eventNode_t();
      eventNode_t(uint32_t msec, bool repeat=true);
      eventNode_t(const eventNode_t &);
      virtual ~eventNode_t() {}
      eventNode_t& operator=(const eventNode_t &rhs);

      eventID_t id() const {return (eventID_t)this;}

      virtual void run() = 0;//pure virtual

      const struct timeval &tout() const { return tout_; }
      struct timeval       &tout()       { return tout_; }
      void                  tout(uint32_t msec) { msec2tval(msec, tout_); }
      uint32_t         tout2msec() const {return tval2msec(tout_);}

      struct timeval       &period()       { return period_; }
      const struct timeval &period() const { return period_; }
      void                  period(uint32_t msec);
      uint32_t              period2msec() const { return tval2msec(period_); }
      bool                  periodic() const { return nonzero(period_) ; }
      bool                  valid() const { return valid_; }
      void                  valid(bool val) { valid_ = val; }


      struct timeval time2fire();
      bool  expired();
  };

  std::ostream &operator<<(std::ostream &os, const eventNode_t &en);
  bool operator<(const eventNode_t &, const eventNode_t &);

  class eventCmp {
    public:
      typedef eventNode_t * value_type;
      bool operator()(const eventNode_t *lhs, const eventNode_t *rhs) const
      {
        // std::cout << "Cmp: " << (rhs->tout() < lhs->tout())
        //   << "\n\tlhs: " << *lhs
        //   << "\n\trhs: " << *rhs
        //   << "\n";
        return lhs->tout() < rhs->tout();
      }
  };

  class eventManager {
    protected:
      // Table of all defined events
      wuset< eventNode_t *, eventCmp > table_;
    public:
      eventManager() {}
      virtual ~eventManager()
      {
        eventNode_t *evnt;
        while (! table_.empty()) {
          evnt = table_.top();
          table_.pop();
          delete evnt;
        }
      }
      typedef  wuset< eventNode_t *, eventCmp >::iterator iterator;
      typedef  wuset< eventNode_t *, eventCmp >::const_iterator const_iterator;

      virtual eventID_t  add(eventNode_t *event);
      virtual void       rem(eventID_t id) { rem((wupp::eventNode_t *) id); }
      virtual size_t    size() const { return table_.size(); }
      virtual void run();
      void debug_print() const;

      // protected:
      struct timeval next_tout() const;
      eventNode_t *top() { return (table_.empty() ? 0 : table_.top()); }
      iterator   begin() { return table_.begin(); }
      iterator     end() { return table_.end(); }
      eventNode_t*	    find(eventID_t id);

    protected:
      void delete_event(eventNode_t *event);
      void delete_event(eventID_t id);
      void rem(eventNode_t *event);
  };

  static inline bool eventDue(const struct timeval &tout);
  static inline bool eventDue(const struct timeval &tout)
  {
    struct timeval now;
    if (gettimeofday(&now, 0) == -1)
      return false;
    return tout <= now;
  }

}; // namespace wupp
#endif
