/*
 * $Source: /project/arlcvs/cvsroot/wu_arl/wusrc/wuPP/event.cc,v $
 * $Author: mah5 $
 * $Date: 2008/10/14 18:29:24 $
 * $Revision: 1.7 $
 * $Name:  $
 *
 * File:   try.cc
 * Author: Fred Kuhns
 * Email:  fredk@cse.wustl.edu
 * Organization: Washington University in St. Louis
 *
 * Created:  02/06/07 15:14:24 CST
 *
 * Description:
 */

#include <iostream>
#include <map>

#include <stddef.h>
#include <inttypes.h>

#include "event.h"


// #define WUEVENT_DEBUG

// ******** Event Node ********
wupp::eventNode_t::eventNode_t()
  : valid_(true)
{
  tout_.tv_sec    = 0;
  tout_.tv_usec   = 0;
  period_.tv_sec  = 0;
  period_.tv_usec = 0;
}

wupp::eventNode_t::eventNode_t(const wupp::eventNode_t &obj)
{
  tout_   = obj.tout_;
  period_ = obj.period_;
  valid_  = obj.valid_;
}

wupp::eventNode_t::eventNode_t(uint32_t msec, bool repeat)
  : valid_(true)
{
  struct timeval tout, tval;
  msec2tval(msec, tout);

  if (repeat) {
    period_ = tout;
  } else {
    period_.tv_sec = 0; period_.tv_usec = 0;
  }
  tout_ = wupp::gettod(tval) + tout;
}

void wupp::eventNode_t::period(uint32_t msec)
{
  struct timeval now;
  msec2tval(msec, period_);
  tout_ = wupp::gettod(now) + period_;
}

wupp::eventNode_t& wupp::eventNode_t::operator=(const eventNode_t &rhs)
{
  if (this == &rhs)
    return *this;

  valid_ = rhs.valid_;
  tout_   = rhs.tout_;
  period_ = rhs.period_;

  return *this;
}

struct timeval wupp::eventNode_t::time2fire()
{
  struct timeval now, tval = {0,0};
  gettod(now);
  if (tout_ > now)
    tval = tout_ - now;

  return tval;
}

bool wupp::eventNode_t::expired()
{
  struct timeval tval;
  return tout_ <= gettod(tval);
}

std::ostream &wupp::operator<<(std::ostream &os, const wupp::eventNode_t &en)
{
  os << "{id " << (unsigned int)en.id()
     << ", P " << en.period2msec()
     //<< ", kpa "   << std::hex << en.kpa() << std::dec
     << ", tout " << en.tout()
     //<< "\n\trbuf " << en.vals()
     << "}";
  return os;
}

// ******** Event Manager ********
wupp::eventID_t wupp::eventManager::add(wupp::eventNode_t *event)
{
  table_.push(event);
  return event->id();
}

void wupp::eventManager::rem(wupp::eventNode_t *event)
{
  if (event == 0) return;
  try {
    table_.remove(event);
  } catch (...) {
  }
  delete_event(event);
}

void wupp::eventManager::delete_event(eventNode_t *event)
{
  if (event == 0) return;
  event->valid(false);
  event->tout(0); // past due
  delete event;
}

static const timeval tval_zero = {0, 0};
struct timeval wupp::eventManager::next_tout() const
{
  struct timeval tval = tval_zero;

  if (! table_.empty()) {
    tval = table_.top()->time2fire();
    if (tval < tval_zero) {
      tval = tval_zero;
    }
  }
  return tval;
}

void wupp::eventManager::run()
{
  eventNode_t *nptr;
  for (nptr = top() ; ! table_.empty() && (! nptr->valid() || nptr->expired()) ; nptr = top()) {
    table_.pop();

    if (nptr->valid()) {
      if ( ! nptr->periodic() ) nptr->valid(false);
      try {
        nptr->run();
      } catch (wupp::errorBase& e) {
        wulog(wulogServ, wulogError, "Error processing periodic counter read\n\terrorBase: %s",
              e.toString().c_str());
        delete_event(nptr);
        throw;
      } catch (...) {
        wulog(wulogServ, wulogError, "Error processing periodic counter read\n");
        delete_event(nptr);
        throw;
      }
    }
    // State of event handler may have changed
    if ( ! nptr->valid() ) {
      delete_event(nptr);
      continue;
    }

    if (nptr->periodic())
      nptr->tout() = nptr->tout() + nptr->period();

    table_.push(nptr);
  }
  return;
}

wupp::eventNode_t *wupp::eventManager::find(eventID_t id)
{
  eventManager::const_iterator iter;
  iter = std::find(table_.begin(), table_.end(), id);
  if (iter == table_.end()) return 0;
  return *iter;
}

void wupp::eventManager::debug_print() const
{
  eventManager::const_iterator iter = table_.begin();
  std::cout << "Events:\n";
  for (; iter != table_.end(); ++iter)
    std::cout << "\t" << *(*iter) << "\n";
  return;
}
