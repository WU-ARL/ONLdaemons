/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/valBuf.h,v $
 * $Author: fredk $
 * $Date: 2007/06/06 21:24:37 $
 * $Revision: 1.15 $
 * $Name:  $
 *
 * File:         msg.h
 * Created:      01/17/2005 02:42:25 PM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#ifndef _WUPP_DATA_BUFF_H
#define _WUPP_DATA_BUFF_H

#include <wulib/valTypes.h>
#include <wuPP/error.h>

#define DBUF_DEFAULT_SIZE    1024

class valBuf_t;

template <class T> struct valTraits {
  valType_t   vtype() const{ return invType; }
  size_t      valsize()  const { return 0; }
  const char *valstr() const { return "invType"; }
  T           nbo(const T& v) const { return v; }
  T           hbo(const T& v) const { return v; }
};

template <> struct valTraits<chr_t> {
  valType_t vtype() const{ return charType; }
  size_t    valsize()  const { return sizeof(chr_t); }
  const char *valstr() const { return "charType"; }
  chr_t nbo(const chr_t& v) const { return v; }
  chr_t hbo(const chr_t& v) const { return v; }
};

template <> struct valTraits<dbl_t> {
  valType_t vtype() const{ return dblType; }
  size_t    valsize()  const { return sizeof(dbl_t); }
  const char *valstr() const { return "dblType"; }
  dbl_t nbo(const dbl_t& v) const
  {
    union dtype {
      uint64_t i64;
      double   d64;
    } xx;
    xx.d64 = v;
    xx.i64 = htonll(xx.i64);
    return xx.d64;
  }
  dbl_t hbo(const dbl_t& v) const
  {
    union dtype {
      uint64_t i64;
      double   d64;
    } xx;
    xx.d64 = v;
    xx.i64 = ntohll(xx.i64);
    return xx.d64;
  }
};

template <> struct valTraits<int_t> {
  valType_t   vtype() const{ return intType; }
  size_t      valsize() const { return sizeof(int_t); }
  const char *valstr() const { return "intType"; }
  int_t       nbo(const int_t& v) const { return (int_t)htonl(v); }
  int_t       hbo(const int_t& v) const { return (int_t)ntohl(v); }
};

template <> struct valTraits<dw8_t> {
  valType_t   vtype() const{ return dw8Type; }
  size_t      valsize()  const { return sizeof(dw8_t); }
  const char *valstr() const { return "dw8Type"; }
  dw8_t       nbo(const dw8_t& v) const { return (dw8_t)htonll(v); }
  dw8_t       hbo(const dw8_t& v) const { return (dw8_t)ntohll(v); }
};

template <> struct valTraits<dw4_t> {
  valType_t   vtype() const{ return dw4Type; }
  size_t      valsize()  const { return sizeof(dw4_t); }
  const char *valstr() const { return "dw4Type"; }
  dw4_t       nbo(const dw4_t& v) const { return (dw4_t)htonl(v); }
  dw4_t       hbo(const dw4_t& v) const { return (dw4_t)ntohl(v); }
};

template <> struct valTraits<dw2_t> {
  valType_t   vtype() const{ return dw2Type; }
  size_t      valsize() const { return sizeof(dw2_t); }
  const char *valstr() const { return "dw2Type"; }
  dw2_t       nbo(const dw2_t& v) const { return (dw2_t)htons(v); }
  dw2_t       hbo(const dw2_t& v) const { return (dw2_t)ntohs(v); }
};

template <> struct valTraits<dw1_t> {
  valType_t   vtype() const{ return dw1Type; }
  size_t      valsize() const { return sizeof(dw1_t); }
  const char *valstr() const { return "dw1Type"; }
  dw1_t       nbo(const dw1_t& v) const { return v; }
  dw1_t       hbo(const dw1_t& v) const { return v; }
};

template <class T> T& valref(valBuf_t &vbuf, size_t indx);
template <class T> void valset(valBuf_t &vbuf, size_t indx, T val);
template <class T> void push_back(valBuf_t &vbuf, T val);

class valBuf_t {
  public:
    valBuf_t(size_t sz=0);
    valBuf_t(size_t sz, char x);
    valBuf_t(const valBuf_t &dbuf);
    valBuf_t(valType_t vt, size_t cnt);
    virtual ~valBuf_t();

    void count(valType_t vt, size_t cnt);

    valBuf_t &operator=(const valBuf_t&);
    valBuf_t &operator+=(const valBuf_t&);

    valBuf_t &copyin_nbo(const void *, size_t);
    valBuf_t &copyin_nbo(const dw1_t *buf, size_t cnt);
    valBuf_t &copyin_nbo(const dw2_t *buf, size_t cnt);
    valBuf_t &copyin_nbo(const dw4_t *buf, size_t cnt);
    valBuf_t &copyin_nbo(const dw8_t *buf, size_t cnt);
    void copyout_nbo(void *) const;
    void copyout_nbo(dw1_t *buf) const;
    void copyout_nbo(dw2_t *buf) const;
    void copyout_nbo(dw4_t *buf) const;
    void copyout_nbo(dw8_t *buf) const;

    size_t   vtsizeof() const { return val_type2size(vtype_); }
    size_t       size() const { return size_; }
    size_t       dlen() const { return dlen_; }
    size_t      count() const { return cnt_; }

    size_t      space() const { return (size_ - dlen())/vtsizeof(); }
    valType_t   vtype() const { return vtype_; }
    const char *vtstr() const { return val_type2str(vtype_); }
    const unsigned char* data() const {return data_;}
    const unsigned char* end() const {return data_ + dlen();}

    /* */
    unsigned char* data() {return data_;}
    void *vptr(size_t indx) { return (void *)((char *)data_ + (indx * vtsizeof())); }

    void clear();
    void reset(valType_t vt=dw1Type) { count(vt, 0); return; }
    void vtype(valType_t vt);
    void size(size_t sz);
    void count(size_t cnt);

    void assign(unsigned char *data, size_t dlen, valType_t vt=dw1Type, size_t cnt=0);
    // just like memset
    void valset(chr_t x);
    void valset(dw1_t x);
    void valset(dw2_t x);
    void valset(dw4_t x);
    void valset(dw8_t x);
    void valset(int_t x);
    void valset(dbl_t x);
    void valzero() {if (data_) {memset((void *)data_, 0, size_);} return;}

    // Unchecked access
    chr_t &chr(size_t indx) { return *((chr_t *)data_ + indx); }
    dw1_t &dw1(size_t indx) { return *((dw1_t *)data_ + indx); }
    dw2_t &dw2(size_t indx) { return *((dw2_t *)data_ + indx); }
    dw4_t &dw4(size_t indx) { return *((dw4_t *)data_ + indx); }
    dw8_t &dw8(size_t indx) { return *((dw8_t *)data_ + indx); }
    int_t &dint(size_t indx){ return *((int_t *)data_ + indx); }
    dbl_t &dbl(size_t indx) { return *((dbl_t *)data_ + indx); }

    chr_t chr(size_t indx) const { return *((chr_t *)data_ + indx); }
    dw1_t dw1(size_t indx) const { return *((dw1_t *)data_ + indx); }
    dw2_t dw2(size_t indx) const { return *((dw2_t *)data_ + indx); }
    dw4_t dw4(size_t indx) const { return *((dw4_t *)data_ + indx); }
    dw8_t dw8(size_t indx) const { return *((dw8_t *)data_ + indx); }
    int_t dint(size_t indx)const { return *((int_t *)data_ + indx); }
    dbl_t dbl(size_t indx) const { return *((dbl_t *)data_ + indx); }

    void push_back(dw1_t val);
    void push_back(dw2_t val);
    void push_back(dw4_t val);
    void push_back(dw8_t val);
    void push_back(int_t val);
    void push_back(dbl_t val);
    void push_backL(long val);

    long val2long(size_t indx);
    void long2val(size_t indx, long val);

    void print();
    void debug_print();

    void hiwat(size_t hiwat) {hiwat_ = hiwat;}
    size_t hiwat() const {return hiwat_;}

  private:
    unsigned char *data_;
    size_t         size_;
    size_t          cnt_;
    size_t         dlen_; // redundant but more efficient (slightly)
    valType_t     vtype_;
    size_t        hiwat_;
};

template <class T> T& valref(valBuf_t &vbuf, size_t indx)
{
  if (indx >= vbuf.count())
    throw wupp::errorBase(wupp::resourceErr, "valref: Invalid index");
  return *(T *)(vbuf.vptr(indx));
}

template <class T> void valset(valBuf_t &vbuf, size_t indx, T val)
{
  switch(vbuf.vtype()) {
    case strType:
    case charType:
      vbuf.chr(indx) = (chr_t)val;
      break;
    case dblType:
      vbuf.dbl(indx) = (dbl_t)val;
      break;
    case intType:
      vbuf.dint(indx) = (int_t)val;
      break;
    case dw8Type:
      vbuf.dw8(indx) = (dw8_t)val;
      break;
    case dw4Type:
      vbuf.dw4(indx) = (dw4_t)val;
      break;
    case dw2Type:
      vbuf.dw2(indx) = (dw2_t)val;
      break;
    case dw1Type:
      vbuf.dw1(indx) = (dw1_t)val;
      break;
    default:
      throw wupp::errorBase(wupp::resourceErr, "valset: invalid type");
  }
}

template <class T> void push_back(valBuf_t &vbuf, T val)
{
  T *ptr = (T *)vbuf.data();
  T *end = (T *)vbuf.end();

  size_t indx = vbuf.count();
  vbuf.count(indx+1);
  switch(vbuf.vtype()) {
    case strType:
    case charType:
        vbuf.chr(indx) = (chr_t)val;
      break;
    case dblType:
      vbuf.dbl(indx)   = (dbl_t)val;
      break;
    case intType:
      vbuf.dint(indx)  = (int_t)val;
      break;
    case dw8Type:
      vbuf.dw8(indx)   = (dw8_t)val;
      break;
    case dw4Type:
      vbuf.dw4(indx)   = (dw4_t)val;
      break;
    case dw2Type:
      vbuf.dw2(indx)   = (dw2_t)val;
      break;
    case dw1Type:
      vbuf.dw1(indx)   = (dw1_t)val;
      break;
    default:
      vbuf.count(indx);
      throw wupp::errorBase(wupp::resourceErr, "dbuf::push: failed to set new value\n");
      break;
  }
  return;
}

inline void valBuf_t::valset(chr_t x) {for (size_t indx=0; indx < cnt_; ++indx) ::valset(*this, indx, x);}
inline void valBuf_t::valset(dw1_t x) {for (size_t indx=0; indx < cnt_; ++indx) ::valset(*this, indx, x);}
inline void valBuf_t::valset(dw2_t x) {for (size_t indx=0; indx < cnt_; ++indx) ::valset(*this, indx, x);}
inline void valBuf_t::valset(dw4_t x) {for (size_t indx=0; indx < cnt_; ++indx) ::valset(*this, indx, x);}
inline void valBuf_t::valset(dw8_t x) {for (size_t indx=0; indx < cnt_; ++indx) ::valset(*this, indx, x);}
inline void valBuf_t::valset(int_t x) {for (size_t indx=0; indx < cnt_; ++indx) ::valset(*this, indx, x);}
inline void valBuf_t::valset(dbl_t x) {for (size_t indx=0; indx < cnt_; ++indx) ::valset(*this, indx, x);}

#endif
