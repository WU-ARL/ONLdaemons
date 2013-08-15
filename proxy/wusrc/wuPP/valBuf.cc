/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/valBuf.cc,v $
 * $Author: fredk $
 * $Date: 2007/02/15 18:52:49 $
 * $Revision: 1.6 $
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

#include <iostream>
#include <string>
#include <stdlib.h>
#include <stdio.h>

#include <errno.h>
#include <stddef.h>
#include <inttypes.h>

#include <wulib/valTypes.h>

#include <wuPP/valBuf.h>
#include <wuPP/error.h>

#define VALBUF_HIWAT_DEFAULT 2048

// ****************************** Constructors ******************************
valBuf_t::valBuf_t(size_t sz)
  : data_(0), size_(0), cnt_(0), dlen_(0), vtype_(dw1Type), hiwat_(VALBUF_HIWAT_DEFAULT)
{
  size(sz);
  reset();
}
valBuf_t::valBuf_t(size_t sz, char x)
  : data_(0), size_(0), cnt_(0), dlen_(0), vtype_(dw1Type), hiwat_(VALBUF_HIWAT_DEFAULT)
{
  size(sz);
  if (sz)
    memset((void *)data_, (int)x, size_);
}
valBuf_t::valBuf_t(const valBuf_t &dbuf)
  : data_(0), size_(0), cnt_(0), dlen_(0), vtype_(dw1Type), hiwat_(VALBUF_HIWAT_DEFAULT)
{
  // count will set the size properly
  count(dbuf.vtype_, dbuf.cnt_);
  if (dbuf.dlen())
    memcpy((void *)data_, (const void *)dbuf.data_, dbuf.dlen());
}
valBuf_t::valBuf_t(valType_t vt, size_t cnt)
  : data_(0), size_(0), cnt_(0), dlen_(0), vtype_(dw1Type), hiwat_(VALBUF_HIWAT_DEFAULT)
{
  count(vt, cnt);
  if (dlen())
    memset(data_, (int)0, dlen());
}
valBuf_t::~valBuf_t() { clear(); }

// **************************************************************************
// intended to be fast, so assume user knows what they are doing!
valBuf_t &valBuf_t::copyin_nbo(const void *buf, size_t cnt)
{
  if (data_ == buf || buf == NULL)
    return *this;

  switch (vtype_) {
    case dw1Type:
      copyin_nbo((const dw1_t *)buf, cnt);
      break;
    case dw2Type:
      copyin_nbo((const dw2_t *)buf, cnt);
      break;
    case dw4Type:
      copyin_nbo((const dw4_t *)buf, cnt);
      break;
    case dw8Type:
      copyin_nbo((const dw8_t *)buf, cnt);
      break;
    default:
      {
        char *myptr = (char *)data_;
        const char *inbuf = (char *)buf;

        // make sure there is enough room, throws an exception on error
        count(cnt);
        size_t vtsz = vtsizeof(); // number of bytes in one element
        void *end = myptr + dlen(); //dlen() = count() * vtsizeof()
        while(myptr < end) {
          valset_ntoh(myptr, inbuf, vtype_);
          myptr += vtsz;
          inbuf += vtsz;
        }
        break;
      }
  }
  return *this;
}

valBuf_t &valBuf_t::copyin_nbo(const dw1_t *buf, size_t cnt)
{
  count(dw1Type, cnt);
  dw1_t *ptr = (dw1_t *)data_;
  for (;cnt > 0; --cnt)
    *ptr++ = *buf++;

  return *this;
}

valBuf_t &valBuf_t::copyin_nbo(const dw2_t *buf, size_t cnt)
{
  count(dw2Type, cnt);
  dw2_t *ptr = (dw2_t *)data_;
  for (;cnt > 0; --cnt)
    *ptr++ = ntohs(*buf++);

  return *this;
}

valBuf_t &valBuf_t::copyin_nbo(const dw4_t *buf, size_t cnt)
{
  count(dw4Type, cnt);
  dw4_t *ptr = (dw4_t *)data_;
  for (;cnt > 0; --cnt)
    *ptr++ = ntohl(*buf++);

  return *this;
}

valBuf_t &valBuf_t::copyin_nbo(const dw8_t *buf, size_t cnt)
{
  count(dw8Type, cnt);
  dw8_t *ptr = (dw8_t *)data_;
  for (;cnt > 0; --cnt)
    *ptr++ = ntohll(*buf++);

  return *this;
}

void valBuf_t::copyout_nbo(void *buf) const
{
  if (data_ == buf || buf == NULL || data_ == NULL)
    throw wupp::errorBase(wupp::resourceErr, "dbuf::copyout_nbo: Invalid arguments");

  switch (vtype_) {
    case dw1Type:
      copyout_nbo((dw1_t *)buf);
      break;
    case dw2Type:
      copyout_nbo((dw2_t *)buf);
      break;
    case dw4Type:
      copyout_nbo((dw4_t *)buf);
      break;
    case dw8Type:
      copyout_nbo((dw8_t *)buf);
      break;
    default:
      {
        const char *myptr = (char *)data_;
        char *outbuf = (char *)buf;
        size_t vtsz = vtsizeof(); // number of bytes in one element
        const void *end = myptr + dlen(); //dlen() = count() * vtsizeof()
        while(myptr < end) {
          valset_hton(outbuf, myptr, vtype_);
          myptr  += vtsz;
          outbuf += vtsz;
        }
      }
      break;
  }
  return;
}

// intended to be fast, so assume user knows what they are doing!
void valBuf_t::copyout_nbo(dw1_t *buf) const
{
  size_t indx = 0;
  dw1_t *ptr = (dw1_t *)data_;
  for (;indx < cnt_; ++indx)
    *buf++ = *ptr++;
}
void valBuf_t::copyout_nbo(dw2_t *buf) const
{
  size_t indx = 0;
  dw2_t *ptr = (dw2_t *)data_;
  for (;indx < cnt_; ++indx)
    *buf++ = htons(*ptr++);
}
void valBuf_t::copyout_nbo(dw4_t *buf) const
{
  size_t indx = 0;
  dw4_t *ptr = (dw4_t *)data_;
  for (;indx < cnt_; ++indx)
    *buf++ = htonl(*ptr++);
}
void valBuf_t::copyout_nbo(dw8_t *buf) const
{
  size_t indx = 0;
  dw8_t *ptr = (dw8_t *)data_;
  for (;indx < cnt_; ++indx)
    *buf++ = htonll(*ptr++);
}

// **************************************************************************
valBuf_t &valBuf_t::operator=(const valBuf_t& rhs)
{
  if (this == &rhs)
    return *this;
  reset();
  count(rhs.vtype_, rhs.cnt_);
  if (rhs.dlen())
    memcpy((void *)data_, (const void *)rhs.data_, dlen());
  return *this;
}

valBuf_t &valBuf_t::operator+=(const valBuf_t& rhs)
{
  if (cnt_ != rhs.cnt_)
    throw wupp::errorBase(wupp::resourceErr, "dbuf::operator+=: lhs.cnt_ != rhs.cnt_");
  if (vtype_ != rhs.vtype_)
    throw wupp::errorBase(wupp::resourceErr, "dbuf::operator+=: lhs.vtype_ != rhs.vtype_");

  for (size_t indx = 0; indx < cnt_; ++indx) {
    switch(vtype_) {
      case strType:
      case charType:
        chr(indx) += rhs.chr(indx);
        break;
      case dblType:
        dbl(indx) += rhs.dbl(indx);
        break;
      case intType:
        dint(indx) += rhs.dint(indx);
        break;
      case dw8Type:
        dw8(indx) += rhs.dw8(indx);
        break;
      case dw4Type:
        dw4(indx) += rhs.dw4(indx);
        break;
      case dw2Type:
        dw2(indx) += rhs.dw2(indx);
        break;
      case dw1Type:
        dw1(indx) += rhs.dw1(indx);
        break;
      default:
        throw wupp::errorBase(wupp::resourceErr, "dbuf::operator+=: Invalid type for operation");
    }
  }
  return *this;
}
// **************************************************************************

/* */
void valBuf_t::clear()
{
  reset(); // sets cnt_ = 0 and vtype_ = dw1
  if (data_) {
    free(data_);
    data_ = 0;
    size_ = 0;
  }
  return;
}

void valBuf_t::assign(unsigned char *data, size_t dlen, valType_t vt, size_t cnt)
{
  clear();
  data_  = data;
  size_  = dlen;
  count(vt, cnt);
  return;
}

void valBuf_t::size(size_t sz)
{
  if (sz == size_ || (sz < size_ && size_ <= hiwat_))
    return;
  // sz < size_ implies that size_ > hiwat_ -- from above
  if (sz < size_ && sz < hiwat_)
    sz = hiwat_; // shrink

  unsigned char *tmp = (unsigned char *)realloc(data_, sz);
  // if realloc() fails then the original memory is not changed (it is
  // still allocated). So change nothing and return an error.
  if (tmp == NULL)
    throw wupp::errorBase(wupp::resourceErr, "dbuf::size: Unable to realloc memory", errno);

  data_ = tmp;
  size_ = sz;

  if (dlen_ > size_) {
    cnt_  = size_ / val_type2size(vtype_);
    dlen_ = cnt_ * val_type2size(vtype_);
  }

  return;
}

static inline void ckvtype(valType_t vt);
static inline void ckvtype(valType_t vt)
{
  if (val_type2size(vt) == 0)
    throw wupp::errorBase(wupp::paramError, "dbuf::vtype: Invalid vtype has zero size", 0);
  return;
}

void valBuf_t::vtype(valType_t vt)
{
  ckvtype(vt);

  vtype_ = vt;
  cnt_   = dlen_ / val_type2size(vt);
  dlen_  = cnt_ * val_type2size(vt);
  return;
}

void valBuf_t::count(size_t cnt)
{
  size_t sz = cnt * val_type2size(vtype_);
  // just in case object needs to grow., call size() first
  size(sz);
  dlen_ = sz;
  cnt_  = cnt;
  return;
}

void valBuf_t::count(valType_t vt, size_t cnt)
{
  ckvtype(vt);
  size_t sz = cnt * val_type2size(vt);
  size(sz);
  vtype_ = vt;
  dlen_  = sz;
  cnt_   = cnt;
  return;
}

long valBuf_t::val2long(size_t indx)
{
  long val;
  switch(vtype_) {
    case strType:
    case charType:
      val = (long)chr(indx);
      break;
    case dblType:
      val = (long)dbl(indx);
      break;
    case intType:
      val = (long)dint(indx);
      break;
    case dw8Type:
      val = (long)dw8(indx);
      break;
    case dw4Type:
      val = (long)dw4(indx);
      break;
    case dw2Type:
      val = (long)dw2(indx);
      break;
    case dw1Type:
      val = (long)dw1(indx);
      break;
    default:
      throw wupp::errorBase(wupp::resourceErr, "dbuf::val2long: invalid type");
  }
  return val;
}

void valBuf_t::long2val(size_t indx, long val)
{
  switch(vtype_) {
    case strType:
    case charType:
      chr(indx) = (char_t)val;
      break;
    case dblType:
      dbl(indx) = (dbl_t)val;
      break;
    case intType:
      dint(indx) = (int_t)val;
      break;
    case dw8Type:
      dw8(indx) = (dw8_t)val;
      break;
    case dw4Type:
      dw4(indx) = (dw4_t)val;
      break;
    case dw2Type:
      dw2(indx) = (dw2_t)val;
      break;
    case dw1Type:
      dw1(indx) = (dw1_t)val;
      break;
    default:
      throw wupp::errorBase(wupp::resourceErr, "dbuf::val2long: invalid type");
  }
  return;
}

void valBuf_t::print()
{
  if (data_ && cnt_)
    val_print_data(data_, vtype_, cnt_);
  else
    printf("Null\n");
  return;
}

void valBuf_t::debug_print()
{
  printf("vbuf: data 0x%08x, size %u, cnt %u, dlen %u, vt %s\n",
          (unsigned int)data_, size_, cnt_, dlen_, vtstr());
  print();
}

void valBuf_t::push_backL(long val)
{
  size_t indx = cnt_;
  count(indx+1);
  try {
    long2val(indx, val);
  } catch (...) {
    count(indx);
    throw;
  }
  return;
}

void valBuf_t::push_back(dw1_t val)
{
  size_t indx = cnt_;
  if (vtype_ != dw1Type)
    throw wupp::errorBase(wupp::resourceErr, "dbuf::push_back<dw1>: invalid type");
  count(indx+1);
  dw1(indx) = val;
  return;
}
void valBuf_t::push_back(dw2_t val)
{
  size_t indx = cnt_;
  if (vtype_ != dw2Type)
    throw wupp::errorBase(wupp::resourceErr, "dbuf::push_back<dw2>: invalid type");
  count(indx+1);
  dw2(indx) = val;
  return;
}
void valBuf_t::push_back(dw4_t val)
{
  size_t indx = cnt_;
  if (vtype_ != dw4Type)
    throw wupp::errorBase(wupp::resourceErr, "dbuf::push_back<dw4>: invalid type");
  count(indx+1);
  dw4(indx) = val;
  return;
}
void valBuf_t::push_back(dw8_t val)
{
  size_t indx = cnt_;
  if (vtype_ != dw8Type)
    throw wupp::errorBase(wupp::resourceErr, "dbuf::push_back<dw8>: invalid type");
  count(indx+1);
  dw8(indx) = val;
  return;
}
void valBuf_t::push_back(int_t val)
{
  size_t indx = cnt_;
  if (vtype_ != intType)
    throw wupp::errorBase(wupp::resourceErr, "dbuf::push_back<int>: invalid type");
  count(indx+1);
  dint(indx) = val;
  return;
}
void valBuf_t::push_back(dbl_t val)
{
  size_t indx = cnt_;
  if (vtype_ != dblType)
    throw wupp::errorBase(wupp::resourceErr, "dbuf::push_back<dbl>: invalid type");
  count(indx+1);
  dbl(indx) = val;
  return;
}

