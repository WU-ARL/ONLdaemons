/*
 * $Source: /project/arlcvs/cvsroot/wu_arl/wusrc/wuPP/confVal.cc,v $
 * $Author: fredk $
 * $Date: 2008/10/30 23:03:20 $
 * $Revision: 1.6 $
 * $Name:  $
 *
 * File:   main.cc
 * Author: Fred Kuhns
 * Email:  fredk@cse.wustl.edu
 * Organization: Washington University in St. Louis
 * 
 * Created:  02/14/06 17:36:11 CST
 * 
 * Description:  
 */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <exception>
#include <stdexcept>
#include <stdlib.h>

#include <inttypes.h>
#include <wulib/keywords.h>
#include <wulib/valTypes.h>
#include <wuPP/token.h>
#include <wuPP/scanner.h>
#include <wuPP/confVal.h>
#include <wuPP/stringUtils.h>

namespace wuconf {
  // ************************************************************************
  //                 class confVal definitions
  // ************************************************************************
  // Modifies:
  //  val_, data_, size_ and dvec_
  void confVal::check_mem()
  {
    // assume type_ and cnt_ have been properly set but memory has not be
    // alloced and values have not been stored.
    size_t need = cnt_ * ttype2size(type_);
    if (need <= CONFVAL_BUF_SIZE) {
      if (data_) 
        free(data_);
      data_ = 0;
      size_ = 0;
      dvec_ = (void *)&val_;
      memset(dvec_, 0, sizeof(val_));
      return;
    }
    if (need > size_) {
      void *tmp = calloc(1, need);
      if (tmp == NULL) {
        reset();
        throw wupp::errorBase(wupp::resourceErr, "confVal: Error allocating memory for dw1 array");
      }
      if (data_)
        free(data_);
      data_ = tmp;
      size_ = need;
      dvec_ = data_;
    } else {
      memset(data_, 0, size_);
      dvec_ = data_;
    }
  }
  confVal &confVal::init(const valToken &tok)
  {
    // First clear any old data
    reset();
    type_ = tok.type();
    str_  = tok.str();
    cnt_  = tok.cnt();
    check_mem();

    if ( cnt_ == 0 || ! tok )
      return *this;

    if (tok.isNumeric() && cnt_ > 0) {
      int tbase = (tok.fmat() == valToken::tokBase16Fmt) ? 16 : 10;
      // Important observation:
      //   Memory allocated to hold the requested values is guaranteed to have
      //   the proper alignment so it is safe to cast the void pointer dvec_.
      switch (type_) {
        case valToken::tokIntType:
          if (cnt_ == 1)
            val_.int_val = (int_t)strtol(str_.c_str(), NULL, tbase);
          else
            wulog(wulogLib, wulogError, "wuconf::init: No support for int arrays\n");
          break;
        case valToken::tokDw8Type:
          if (cnt_ == 1)
            val_.dw8_val = (dw8_t)strtoull(str_.c_str(), NULL, tbase);
          else
            wulog(wulogLib, wulogError, "tok2Value: No support for arrays\n");
          break;
        case valToken::tokDw4Type:
          if (cnt_ == 1)
            val_.dw4_val = (dw4_t)strtoul(str_.c_str(), NULL, tbase);
          else
            wulog(wulogLib, wulogError, "tok2Value: No support for arrays\n");
          break;
        case valToken::tokDw2Type:
          if (cnt_ == 1)
            val_.dw2_val = (dw2_t)strtoul(str_.c_str(), NULL, tbase);
          else
            wulog(wulogLib, wulogError, "tok2Value: No support for arrays\n");
          break;
        case valToken::tokDw1Type:
          // Arrays are only supported (in the tokenizer) for bytes
          if (cnt_ == 1) {
            val_.dw1_val = (dw1_t)strtoul(str_.c_str(), NULL, tbase);
          } else {
            std::vector<std::string> nums;
            string2flds(nums, tok.str(), ".:");
            if (nums.size() < cnt_)
              throw wupp::errorBase(wupp::fmtError, "confVal: dw1 Array, fewer numbers than expected");
            dw1_t *ptr = (dw1_t *)dvec_;
            for (size_t indx = 0; indx < cnt_; ++indx)
              ptr[indx] = (dw1_t)strtoul(nums[indx].c_str(), NULL, tbase);
          }
          break;
        case valToken::tokDblType:
          if (cnt_ == 1)
            val_.dbl_val = strtod(str_.c_str(), NULL);
          else
            wulog(wulogLib, wulogError, "wuconf::init: No support for arrays\n");
          break;
        default:
          wulog(wulogLib, wulogError, "tok2Value: don\'t know what to do with type %s.\n", str_.c_str());
      } // switch (type_)
    } // Numeric value
    return *this;
  }

  confVal& confVal::reset()
  {
    type_ = valToken::tokInvType;
    str_.clear();
    cnt_  = 0;
    if (data_)
      free(data_);
    size_ = 0;
    data_ = 0;
    dvec_ = 0;
    return *this;
  }

  void confVal::check_val() const
  {
    if (type_ != valToken::tokDw1Type && cnt_ > 1)
      throw wupp::errorBase(wupp::libError, "confVal: cnt must be 0 or 1 for all but dw1");
    if (type_ == valToken::tokInvType)
      throw wupp::errorBase(wupp::opError, "confVal: attempt to read an invalid object");
    if (cnt_ == 0)
      throw wupp::errorBase(wupp::opError, "confVal: attempt to read from object without a value");
  }

  confVal &confVal::operator=(const confVal &rhs)
  {
    if (this == &rhs)
      return *this;
    return copy(rhs);
  }

  confVal &confVal::copy(const confVal &rhs)
  {
    reset();
    type_ = rhs.type_;
    str_  = rhs.str_;
    cnt_  = rhs.cnt_;

    check_mem();

    if ( rhs.cnt_ )
      memcpy(dvec_, rhs.dvec_, rhs.dlen());

    return *this;
  }

  // ---
  dw8_t confVal::dw8_val() const
  {
    dw8_t val;
    check_val();
    switch (type_) {
      case valToken::tokDw8Type:
        val = (dw8_t)val_.dw8_val;
        break;
      case valToken::tokIntType:
        val = (dw8_t)val_.int_val;
        break;
      case valToken::tokDw4Type:
        val = (dw8_t)val_.dw4_val;
        break;
      case valToken::tokDw2Type:
        val = (dw8_t)val_.dw2_val;
        break;
      case valToken::tokDblType:
        val = (dw8_t)val_.dbl_val;
        break;
      case valToken::tokDw1Type:
        switch (cnt_) {
          case sizeof(dw1_t):
            val = (dw8_t)val_.dw1_val;
            break;
          case sizeof(dw2_t):
            val = (dw8_t)val_.dw2_val;
            break;
          case sizeof(dw4_t):
            val = (dw8_t)val_.dw4_val;
            break;
          case sizeof(dw8_t):
            val = (dw8_t)val_.dw8_val;
            break;
          default:
            throw wupp::errorBase(wupp::paramError, "dw8_val: can not conver dw1 array");
            break;
        }
      default:
        throw wupp::errorBase(wupp::paramError, "confVal: invalid conversion to dw8");
        break;
    }
    return val;
  }

  dw4_t confVal::dw4_val() const
  {
    dw4_t val;
    check_val();
    switch (type_) {
      case valToken::tokDw8Type:
        val = (dw4_t)val_.dw8_val;
        break;
      case valToken::tokIntType:
        val = (dw4_t)val_.int_val;
        break;
      case valToken::tokDw4Type:
        val = (dw4_t)val_.dw4_val;
        break;
      case valToken::tokDw2Type:
        val = (dw4_t)val_.dw2_val;
        break;
      case valToken::tokDblType:
        val = (dw4_t)val_.dbl_val;
        break;
      case valToken::tokDw1Type:
        switch (cnt_) {
          case sizeof(dw1_t):
            val = (dw4_t)val_.dw1_val;
            break;
          case sizeof(dw2_t):
            val = (dw4_t)val_.dw2_val;
            break;
          case sizeof(dw4_t):
            val = (dw4_t)val_.dw4_val;
            break;
          case sizeof(dw8_t):
            val = (dw4_t)val_.dw8_val;
            break;
          default:
            throw wupp::errorBase(wupp::paramError, "dw4_val: can not conver dw1 array");
            break;
        }
        break;
      default:
        throw wupp::errorBase(wupp::paramError, "confVal: invalid conversion to dw4");
        break;
    }
    return val;
  }

  dw2_t confVal::dw2_val() const
  {
    dw2_t val;
    check_val();
    switch (type_) {
      case valToken::tokDw8Type:
        val = (dw2_t)val_.dw8_val;
        break;
      case valToken::tokIntType:
        val = (dw2_t)val_.int_val;
        break;
      case valToken::tokDw4Type:
        val = (dw2_t)val_.dw4_val;
        break;
      case valToken::tokDw2Type:
        val = (dw2_t)val_.dw2_val;
        break;
      case valToken::tokDblType:
        val = (dw2_t)val_.dbl_val;
        break;
      case valToken::tokDw1Type:
        switch (cnt_) {
          case sizeof(dw1_t):
            val = (dw2_t)val_.dw1_val;
            break;
          case sizeof(dw2_t):
            val = (dw2_t)val_.dw2_val;
            break;
          case sizeof(dw4_t):
            val = (dw2_t)val_.dw4_val;
            break;
          case sizeof(dw8_t):
            val = (dw2_t)val_.dw8_val;
            break;
          default:
            throw wupp::errorBase(wupp::paramError, "dw2_val: can not conver dw1 array");
            break;
        }
      default:
        throw wupp::errorBase(wupp::paramError, "confVal: invalid conversion to dw2");
        break;
    }
    return val;
  }

  dw1_t confVal::dw1_val() const
  {
    dw1_t val;
    check_val();
    switch (type_) {
      case valToken::tokDw8Type:
        val = (dw1_t)val_.dw8_val;
        break;
      case valToken::tokIntType:
        val = (dw1_t)val_.int_val;
        break;
      case valToken::tokDw4Type:
        val = (dw1_t)val_.dw4_val;
        break;
      case valToken::tokDw2Type:
        val = (dw1_t)val_.dw2_val;
        break;
      case valToken::tokDblType:
        val = (dw1_t)val_.dbl_val;
        break;
      case valToken::tokDw1Type:
        switch (cnt_) {
          case sizeof(dw1_t):
            val = (dw1_t)val_.dw1_val;
            break;
          case sizeof(dw2_t):
            val = (dw1_t)val_.dw2_val;
            break;
          case sizeof(dw4_t):
            val = (dw1_t)val_.dw4_val;
            break;
          case sizeof(dw8_t):
            val = (dw1_t)val_.dw8_val;
            break;
          default:
            throw wupp::errorBase(wupp::paramError, "dw1_val: can not conver dw1 array");
            break;
        }
      default:
        throw wupp::errorBase(wupp::paramError, "confVal: invalid conversion to dw1");
        break;
    }
    return val;
  }

  int_t confVal::int_val() const
  {
    int_t val;
    check_val();
    switch (type_) {
      case valToken::tokDw8Type:
        val = (int_t)val_.dw8_val;
        break;
      case valToken::tokIntType:
        val = (int_t)val_.int_val;
        break;
      case valToken::tokDw4Type:
        val = (int_t)val_.dw4_val;
        break;
      case valToken::tokDw2Type:
        val = (int_t)val_.dw2_val;
        break;
      case valToken::tokDblType:
        val = (int_t)val_.dbl_val;
        break;
      case valToken::tokDw1Type:
        switch (cnt_) {
          case sizeof(dw1_t):
            val = (int_t)val_.dw1_val;
            break;
          case sizeof(dw2_t):
            val = (int_t)val_.dw2_val;
            break;
          case sizeof(dw4_t):
            val = (int_t)val_.dw4_val;
            break;
          case sizeof(dw8_t):
            val = (int_t)val_.dw8_val;
            break;
          default:
            throw wupp::errorBase(wupp::paramError, "int_val: can not conver dw1 array");
            break;
        }
      default:
        throw wupp::errorBase(wupp::paramError, "confVal: invalid conversion to int");
        break;
    }
    return val;
  }

  dbl_t confVal::dbl_val() const
  {
    dbl_t val;
    check_val();
    switch (type_) {
      case valToken::tokDw8Type:
        val = (dbl_t)val_.dw8_val;
        break;
      case valToken::tokIntType:
        val = (dbl_t)val_.int_val;
        break;
      case valToken::tokDw4Type:
        val = (dbl_t)val_.dw4_val;
        break;
      case valToken::tokDw2Type:
        val = (dbl_t)val_.dw2_val;
        break;
      case valToken::tokDblType:
        val = (dbl_t)val_.dbl_val;
        break;
      case valToken::tokDw1Type:
        switch (cnt_) {
          case sizeof(dw1_t):
            val = (dbl_t)val_.dw1_val;
            break;
          case sizeof(dw2_t):
            val = (dbl_t)val_.dw2_val;
            break;
          case sizeof(dw4_t):
            val = (dbl_t)val_.dw4_val;
            break;
          case sizeof(dw8_t):
            val = (dbl_t)val_.dw8_val;
            break;
          default:
            throw wupp::errorBase(wupp::paramError, "dbl_val: can not conver dw1 array");
            break;
        }
      default:
        throw wupp::errorBase(wupp::paramError, "confVal: invalid conversion to dbl");
        break;
    }
    return val;
  }

  long confVal::lval() const
  {
    check_val();
    if (sizeof(long) == sizeof(dw4_t)) return (long) dw4_val();
    return (long)dw8_val();
  }

  wunet::ipAddr_t confVal::ipaddr() const
  {
    if (type_ == valToken::tokDw1Type) {
      if (cnt_ != sizeof(wunet::ipAddr_t))
        throw wupp::errorBase(wupp::opError, "confVal:ipaddr(): dw1 vec is the wrong size");
      return dw4_val();
    }
    return htonl(dw4_val());
  }

  wunet::ipPort_t confVal::ipport() const
  {
    if (type_ == valToken::tokDw1Type) {
      if (cnt_ != sizeof(wunet::ipPort_t))
        throw wupp::errorBase(wupp::opError, "confVal:port(): dw1 vec is wrong size");
      return dw2_val();
    }
    return htons(dw2_val());
  }

  wunet::ipProto_t confVal::ipproto() const
  {
    return dw1_val();
  }

  dw1_t* confVal::ethaddr() const
  {
    if (type_ == valToken::tokDw1Type && cnt_ == sizeof(wunet::ethAddr_t))
      return dw1_vec();
    throw wupp::errorBase(wupp::opError, "confVal:ethaddr(): invalid format");
  }

};
