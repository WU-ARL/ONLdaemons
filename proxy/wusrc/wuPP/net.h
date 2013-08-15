/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/net.h,v $
 * $Author: fredk $
 * $Date: 2007/05/10 17:43:12 $
 * $Revision: 1.14 $
 * $Name:  $
 *
 * File:         net.h
 * Created:      01/05/2005 04:32:38 PM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#ifndef _WUNET_NET_H
#define _WUNET_NET_H

#include <wuPP/error.h>
#include <wulib/wunet.h>

namespace wunet {

  class netErr : public wupp::errorBase {
    public:
      netErr(wupp::errType et = wupp::noError)
        : wupp::errorBase(et) {}
      netErr(wupp::errType et, const std::string& lm)
        : wupp::errorBase(et, lm) {}
      netErr(wupp::errType et, const std::string& lm, int err, const std::string& sm)
        : wupp::errorBase(et, lm, err, sm) {}
      netErr(wupp::errType et, const std::string& lm, int err)
        : wupp::errorBase(et, lm, err) {}
      netErr(wupp::errType et, const wupp::errInfo &einfo)
        : wupp::errorBase(et, einfo) {}
      virtual ~netErr() {}

#if 0
      virtual std::ostream& print(std::ostream& os = std::cerr) const
      {
        os << "NetErr type " << et_
           << ", Errno = "    << se_;
        if (! lm_.empty())
           os << "\n\tMsg: " << lm_;
        if (!sm_.empty())
          os << "\n\tSys: " << sm_;
        return os;
      }
#endif
  };

  template <class T, class Y> T& turnon_bit(T& flags, Y flag)
  {
    flags = flags | T(flag);
    return flags;
  }

  template <class T, class Y> T& turnoff_bit(T& flags, Y flag)
  {
    flags = flags & ~T(flag);
    return flags;
  }

  template <class T, class Y> T isBiton(T& flags, Y flag)
  {
    return flags & T(flag);
  }

  template <class T, class Y> T isBitoff(T& flags, Y flag)
  {
    return T(flag) & ~flags;
  }

}

#endif
