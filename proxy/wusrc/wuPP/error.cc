/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/error.cc,v $
 * $Author: fredk $
 * $Date: 2008/02/11 16:41:01 $
 * $Revision: 1.10 $
 * $Name:  $
 *
 * File:   error.h
 * Author: Fred Kuhns
 * Email:  fredk@cse.wustl.edu
 * Organization: Washington University in St. Louis
 * 
 * Created:  04/07/06 15:08:11 CDT
 * 
 * Description:  
 */
#include <iostream>
#include <sstream>
#include <exception>

#include <errno.h>
#include <wulib/wulog.h>
#include <wuPP/error.h>

std::ostream& wupp::operator<<(std::ostream& os, wupp::errLevel el)
{
  switch (el) {
    case wun_el_Info:  os << "Info"; break;
    case wun_el_Warn:  os << "Warn"; break;
    case wun_el_Err:   os << "Err"; break;
    default: os << "Invalid Error Level"; break;
  }
  return os;
}

std::ostream& wupp::operator<<(std::ostream& os, wupp::errType et)
  { return os << makeNetName(err2str)(et); }

/* -------------------------- errLevel -------------------------- */
wupp::errorBase::errorBase(wupp::errType et, wupp::errLevel el, const std::string& lm)
  : et_(et), el_(el),         /* who_(""), */   lm_(lm),   se_(0) { set_syserr(0); }

wupp::errorBase::errorBase(wupp::errType et, wupp::errLevel el, const std::string& lm, int err)
  : et_(et), el_(el),         /* who_(""), */   lm_(lm),   se_(0) { set_syserr(err); }

/* -------------------------- ! errLevel -------------------------- */
wupp::errorBase::errorBase(wupp::errType et)
  : et_(et), el_(wun_el_Err), /* who_(""),      lm_(""),*/ se_(0) { set_syserr(0); }

wupp::errorBase::errorBase(wupp::errType et, const std::string& lm)
  : et_(et), el_(wun_el_Err), /* who_(""), */   lm_(lm),   se_(0) { set_syserr(0); }

wupp::errorBase::errorBase(wupp::errType et, const std::string& lm, int err)
  : et_(et), el_(wun_el_Err), /* who_(""), */   lm_(lm),   se_(0) { set_syserr(err); }

wupp::errorBase::errorBase(wupp::errType et, const std::string& lm, int err, const std::string& sm)
  : et_(et), el_(wun_el_Err), /* who_(""), */   lm_(lm),   se_(err), sm_(sm) {}

wupp::errorBase::errorBase(wupp::errType et, const wupp::errInfo &einfo)
  : et_(et), el_(einfo.el_),  who_(einfo.who_), lm_(einfo.msg_), se_(0) { set_syserr(einfo.syserr_); }

void wupp::errorBase::set_syserr(int syserr)
{
  se_ = syserr;
  if (se_ > 0) {
    // originally used strerror_r() but it didn't behave as described in
    // the man page. It is supposed to return an int and copy std::string
    // to the supplied buffer. But rather it returned a pointer to the
    // error std::string (which was not the buffer address).
    char *str =  strerror(se_);
    if (str)
      sm_ = str;
  }
}

wupp::errorBase::~errorBase() {}

void wupp::errorBase::logmsg() const
{
  wulogLvl_t lvl = wulogVerb;
  switch (el_) {
    case makeNetLvlName(Err):  lvl = wulogError; break;
    case makeNetLvlName(Warn): lvl = wulogWarn; break;
    case makeNetLvlName(Info): lvl = wulogInfo; break;
  }

  wulog(wulogLib, lvl, "%s: %s.\n\terrType %d: %s\n",
        (who_.empty() ? "errorBase::print" : who_.c_str()),
        (lm_.empty()  ? "Reporting error"  : lm_.c_str()),
        et_, makeNetName(err2str)(et_));
  if (se_ > 0)
    wulog(wulogLib, lvl, "\tsysErr %d: %s\n", se_, strerror(se_));

  return;
}
std::ostream& wupp::errorBase::print(std::ostream& os) const
{
  os << "(" << makeNetName(errlvl2str)(el_) << ") "
     << (who_.empty() ? "errorBase::print" : who_) << ": "
     << (lm_.empty()  ? "Reporting error"  : lm_ )
     << "\n\terrType " << et_ << ": " << makeNetName(err2str)(et_) << "\n";
  if (se_ > 0)
    os << "\tsysErr " << se_ << ": " << strerror(se_);
  if (! sm_.empty())
    os << "\nsysMsg: " << sm_ << std::endl;
  return os;
}

std::string wupp::errorBase::toString() const
{
  std::ostringstream ss;
  print(ss);
  return ss.str();
}

std::ostream& wupp::operator<<(std::ostream& os, const wupp::errorBase& eb)
{
  return eb.print(os);
}
