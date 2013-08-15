/*
 * $Source: /project/arlcvs/cvsroot/wu_arl/wusrc/wuPP/confVal.h,v $
 * $Author: fredk $
 * $Date: 2008/10/30 22:54:57 $
 * $Revision: 1.4 $
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

#ifndef _WUCONFVAL_H
#define _WUCONFVAL_H
#include <map>
#include <wuPP/token.h>
#include <wuPP/scanner.h>
#include <wulib/keywords.h>
#include <wulib/valTypes.h>
#include <wuPP/net.h>

namespace wuconf {

  static inline valType_t ttype2vtype(valToken::token_type tt);
  static inline size_t ttype2size(valToken::token_type tt);

  static inline valType_t ttype2vtype(valToken::token_type tt)
  {
    switch (tt) {
      case valToken::tokIntType:
        return intType;
      case valToken::tokDw8Type:
        return dw8Type;
      case valToken::tokDw4Type:
        return dw4Type;
      case valToken::tokDw2Type:
        return dw2Type;
      case valToken::tokDw1Type:
        return dw1Type;
      case valToken::tokDblType:
        return dblType;
      case valToken::tokStringType:
        return strType;
      default:
        return invType;
    }
  }

  static inline size_t ttype2size(valToken::token_type tt)
  {
    return val_type2size(ttype2vtype(tt));
  }
  // Represents the value of a configuration parameter
  // Currently only support strings, number and byte arrays.
#define CONFVAL_BUF_SIZE 64
  class confVal {
    private:
      valToken::token_type type_;// type of object(s)
      std::string          str_; // string rep of value
      size_t               cnt_; // number of objects of type type_
      void*               data_; // memory allocated by malloc
      size_t              size_; // size of allocated memory
      void*               dvec_; // pointer to data buffer
      union {
        int_t            int_val;
        dw1_t            dw1_val;
        dw2_t            dw2_val;
        dw4_t            dw4_val;
        dw8_t            dw8_val;
        dbl_t            dbl_val;
        dw1_t            buff_[CONFVAL_BUF_SIZE];
      } val_;

      confVal &copy(const confVal &rhs);
      void check_val() const;
      void check_mem();
    public:
      confVal()
        : type_(valToken::tokInvType), str_(""), cnt_(0), data_(0), size_(0), dvec_(0) { reset(); }
      confVal(const valToken &tok)
        : type_(valToken::tokInvType), str_(""), cnt_(0), data_(0), size_(0), dvec_(0) { init(tok); }
      confVal(const confVal &cval)
        : type_(valToken::tokInvType), str_(""), cnt_(0), data_(0), size_(0), dvec_(0) { copy(cval); }
      ~confVal() { reset(); }

      bool operator!() const { return type_ == valToken::tokInvType; }
      bool has_value() const { return cnt_; }

      confVal& init(const valToken &tok);
      confVal& reset();

      // I rely on  two's complement representation, i.e. signed and unsigned
      // have the same representation. So a simple cast is sufficient and doesn't
      // result in any code.
      dw8_t         dw8_val() const;
      dw4_t         dw4_val() const;
      dw2_t         dw2_val() const;
      dw1_t         dw1_val() const;
      int_t         int_val() const;
      double        dbl_val() const;
      dw1_t*        dw1_vec() const { return (dw1_t *)dvec_; }
      dw1_t*        dw1_vec() { return (dw1_t *)dvec_; }

      long long       llval() const { return (long long) dw8_val(); }
      long             lval() const;
      int_t            ival() const { return int_val(); }

      valToken::token_type type() const {return type_;}
      const std::string    &str() const {return str_;}
      size_t                cnt() const {return cnt_;}
      size_t               dlen() const {return cnt_ * ttype2size(type_);}

      wunet::ipAddr_t ipaddr() const;
      wunet::ipPort_t ipport() const;
      wunet::ipProto_t ipproto() const;
      dw1_t*          ethaddr() const;

      confVal &operator=(const confVal &rhs);
  };
  // Utilities
};
#endif
