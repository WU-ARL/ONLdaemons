/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/buf.cc,v $
 * $Author: fredk $
 * $Date: 2007/01/25 18:45:55 $
 * $Revision: 1.12 $
 * $Name:  $
 *
 * File:         buf.cc
 * Created:      01/28/2005 07:35:26 PM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */
#include <iostream>
#include <sstream>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <stdint.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <wulib/wulog.h>

#include <wuPP/net.h>
#include <wuPP/buf.h>

using namespace wunet;

const mbuf::size_type mbuf::HSTART_;
const mbuf::size_type mbuf::PSTART_;
const mbuf::size_type mbuf::vmaxCnt;
const mbuf::size_type mbuf::minAlloc;

void wunet::mbuf::reportError(const std::string &msg)
{
  wulog(wulogBuf, wulogError,  "mbuf::%s.\n\tBuf = %s\n", msg.c_str(), toString().c_str());
  clear();
  throw netErr(wupp::resourceErr, "mbuf:: " + msg);
  return;
}

wunet::mbuf::mbuf(mbuf::size_type ml)
  : st_(wunet::mbuf::Ready),
    msize_(0), mcnt_(0),
    hsize_(0), hlen_(0), hoffset_(0),
    psize_(0), plen_(0), poffset_(0), pbegin_(PSTART_), pend_(PSTART_)
{
  for (size_type i=0; i < vmaxCnt; i++) {
    vecs_[i].iov_base = NULL;
    vecs_[i].iov_len  = 0;
    vszs_[i].iov_base = NULL;
    vszs_[i].iov_len  = 0;
  }
  hlen(ml); // header
  plen(ml); // payload
}

wunet::mbuf::mbuf(mbuf::size_type hl, mbuf::size_type ml)
  : st_(wunet::mbuf::Ready),
    msize_(0), mcnt_(0),
    hsize_(0), hlen_(0), hoffset_(0),
    psize_(0), plen_(0), poffset_(0), pbegin_(PSTART_), pend_(PSTART_)
{
  for (size_type i=0; i < vmaxCnt; i++) {
    vecs_[i].iov_base = NULL;
    vecs_[i].iov_len  = 0;
    vszs_[i].iov_len  = 0;
    vszs_[i].iov_base = 0;
  }
  hlen(hl);
  plen(ml);
}

wunet::mbuf::~mbuf()
{ 
#ifdef WUDEBUG
  MBUF_CKBUF();
#endif
  free_all();
}

std::string mbuf::toString(mbuf::msgState_t st)
{
  std::string str;
  switch (st) {
    case mbuf::Ready:    str = "Ready"; break;
    case mbuf::HPartial: str = "HPartial"; break;
    case mbuf::Header:   str = "Header"; break;
    case mbuf::PPartial: str = "PPartial"; break;
    case mbuf::Complete: str = "Complete"; break;
    case mbuf::Error:    str = "Error"; break;
    default:             str = "Unknown"; break;
  }
  return str;
}

void mbuf::alloc_vec(size_type len, void *buf)
{
  // Need to be smarter here, can allocate a new and larger buffer then free
  // existing buffers and assign newle allocated buffer.

  if (mcnt_ == vmaxCnt)
    reportError("alloc_vec: can not add another vector\n");

  if (buf == 0) {
    len = std::max(len, minAlloc << mcnt_);
    buf = malloc(len);
    if (buf == NULL) {
      // Throws an exception
      // std::cerr << "alloc_vec: Unable to allocate " << len << " bytes\n";
      reportError("alloc_vec:  malloc failed");
    }
  }

  vecs_[mcnt_].iov_base = buf; // new buffer
  vecs_[mcnt_].iov_len  = 0;   // bytes of valid data in buffer, may be 0
  vszs_[mcnt_].iov_base = buf; // total buffer space, > 0
  vszs_[mcnt_].iov_len  = len; // total buffer space, > 0

  if (mcnt_ == 0) {
    hsize_ = len;
  } else {
    psize_ += len;
  }

  mcnt_  += 1;
  msize_ += len;
}

// free last allocated index
void mbuf::free_vec()
{
  if (mcnt_ == 0)
    return;

  mcnt_  -= 1;
  size_type vlen = vszs_[mcnt_].iov_len;
  msize_ -= vlen;

  free(vszs_[mcnt_].iov_base);
  vecs_[mcnt_].iov_base = NULL;
  vecs_[mcnt_].iov_len  = 0;
  vszs_[mcnt_].iov_base = NULL;
  vszs_[mcnt_].iov_len  = 0;


  if (mcnt_ == 0) {
    hsize_ = 0;
    hreset();
  } else {
    psize_ -= vlen;
    preset();
  }
}

void mbuf::plen(mbuf::size_type Total)
{
#   ifdef WUBUF_DEBUG
    MBUF_CKBUF();
#   endif
  pclear();     // sets pend_ = pbegin_ == PSTART_, plen_ = 0 and poffset_ = 0
  psize(Total); // just in case we need to grow.
  plen_ = Total;

  // now set the iovec lenghs
  size_type nleft = Total;
  for (pend_ = PSTART_; pend_ < mcnt_; ++pend_) {
    register size_type vlen = vszs_[pend_].iov_len;
    if (nleft <= vlen) {
      vecs_[pend_].iov_len = nleft;
      break;
    } else {
      vecs_[pend_].iov_len = vlen;
      nleft -= vlen;
    }
  }

  if (pend_ == mcnt_) {
    reportError("plen: unable to allocate requested memory\n");
  }
#   ifdef WUBUF_DEBUG
    MBUF_CKBUF();
#   endif
}

void mbuf::hsize(mbuf::size_type Total)
{
#   ifdef WUBUF_DEBUG
    MBUF_CKBUF();
#   endif
  hrewind();
  // the buffer must have a minimum of Total Bytes of buffer
  // space free

  if (mcnt_ == 0) {
    alloc_vec(Total);
  } else if (Total > hsize_) {
    char *buf;
    if ((buf = (char *)realloc(vecs_[HSTART_].iov_base, Total)) == NULL) {
      reportError("hsize: Unable to realloc memory\n");
    }

    vecs_[HSTART_].iov_base = (void *)buf;
    // vecs_[HSTART_].iov_len is left as is.
    vszs_[HSTART_].iov_base = (void *)buf;
    vszs_[HSTART_].iov_len = Total;
    hsize_ = Total;
    msize_ = hsize_ + psize_;
  }
  hreset();
#   ifdef WUBUF_DEBUG
    MBUF_CKBUF();
#   endif
}

void mbuf::hlen(size_type Total)
{
#   ifdef WUBUF_DEBUG
    MBUF_CKBUF();
#   endif

  // the buffer must have a minimum of Total Bytes of buffer
  // space free. hsize will also undo any seeks.
  hsize(Total);
  // vecs_[HSTART_].iov_base already references start of buffer
  vecs_[HSTART_].iov_len = Total;
  hlen_ = Total;

#   ifdef WUBUF_DEBUG
    MBUF_CKBUF();
#   endif
}

// seek relative to beginning
void mbuf::pseek(size_type offset)
{
  size_type indx, nleft = offset;
#   ifdef WUBUF_DEBUG
    MBUF_CKBUF();
#   endif

  // start from the beginning for simplicity
  prewind();
  if (offset == 0)
    return;
  if (offset > plen_) {
    wulog(wulogBuf, wulogError, "pseek: invalid seek offset (%d) > plen_ (%d)\n", offset, plen_);
    reportError("pseek: invalid seek offset > plen");
  }

  for (indx = PSTART_; indx <= pend_; ++indx) {
    register size_type vlen = vecs_[indx].iov_len;
    if (nleft <= vlen) {
      vecs_[indx].iov_base = (char *)vecs_[indx].iov_base + nleft;
      vecs_[indx].iov_len  = vlen - nleft;
      pbegin_  = indx;
      poffset_ = offset;
      break;
    } else {
      nleft -= vlen;
      vecs_[indx].iov_len = 0;
    }
  }
  if (indx > pend_) {
    wulog(wulogBuf, wulogError, "bad pseek offset = %d\n", offset);
    reportError("pseek: Unable to seek to offset\n");
  }
#   ifdef WUBUF_DEBUG
    MBUF_CKBUF();
#   endif

}

void *mbuf::ppullup()
{
#   ifdef WUBUF_DEBUG
  MBUF_CKBUF();
#   endif

  size_type Total = psize_, len = plen_;

  if (plen_ <= vecs_[PSTART_].iov_len)
    return vecs_[PSTART_].iov_base;

  size_type offset = poffset();
  prewind();

  char *buf;
  if ((buf = (char *)malloc(Total)) == NULL) {
    reportError("pullup: unable to allocate memory");
  }
  pcopyout(buf, Total); // only need to copy plen_ but just in case I get it all

  while (psize_ > 0)
    free_vec();
  alloc_vec(Total, buf);
  // need to make sure we are in a consistant state
  // call preset since it wont first verify state is consistant
  // then call plen() to set len and pend properly.
  pclear();
  plen(len);

  // finally re-apply any seek
  pseek(offset);
#   ifdef WUBUF_DEBUG
  MBUF_CKBUF();
#   endif
  return vecs_[PSTART_].iov_base;
}

void mbuf::pcopyin(const char *ptr, size_type len)
{
#   ifdef WUBUF_DEBUG
    MBUF_CKBUF();
#   endif
  pclear();
  plen(len);

  size_type nleft = len;
  for (size_type indx = PSTART_; indx <= pend_; ++indx) {
    // the iovecs have already had their base address and lengths set by plen()
    memcpy(vecs_[indx].iov_base, (void *)ptr, vecs_[indx].iov_len);
    nleft -= vecs_[indx].iov_len;
    ptr   += vecs_[indx].iov_len;
  }
#   ifdef WUBUF_DEBUG
    MBUF_CKBUF();
#   endif
}

std::ostream &mbuf::printHeader(std::ostream &ss, size_t width) const
{
  size_type cnt = 0;
  if (hlen() == 0)
    ss << "\n  No Header (hlen == 0)\n";
  unsigned char *ptr = (unsigned char *)vecs_[HSTART_].iov_base + hoffset_;
  for (size_type off = 0; off < vecs_[HSTART_].iov_len; ++off, ++cnt) {
    if ((cnt % width) == 0)
      ss << "\n  " << std::hex << std::setw(3) << std::setfill('0') << (int)cnt << ":  ";
    ss << std::hex << std::setw(2) << std::setfill('0') << (int)ptr[off] << " ";
  }
  ss << std::dec << "\n";
  return ss;
}

std::ostream &mbuf::printData(std::ostream &ss, size_t width) const
{
  size_type cnt = 0;
  if (plen() == 0)
    ss << "\n  No Data (plen == 0)\n";
  for (size_type indx = pbegin_; indx <= pend_; ++indx) {
    unsigned char *ptr = (unsigned char *)vecs_[indx].iov_base;
    for (size_type off = 0; off < vecs_[indx].iov_len; ++off, ++cnt) {
      if ((cnt % width) == 0)
        ss << "\n  " << std::hex << std::setw(3) << std::setfill('0') << (int)cnt << ":  ";
      ss << std::hex << std::setw(2) << std::setfill('0') << (int)ptr[off] << " ";
    }
  }
  ss << std::dec << "\n";
  return ss;
}

void mbuf::pcopyout(char *ptr, size_type len) const
{
#   ifdef WUBUF_DEBUG
    MBUF_CKBUF();
#   endif
  // start from any offset value
  size_type nleft = std::min(len, plen());
  for (size_type indx = pbegin_; nleft > 0; ++indx) {
    size_type n = (size_type)std::min(vecs_[indx].iov_len, nleft);
    memcpy((void *)ptr, vecs_[indx].iov_base, n);
    nleft -= n;
    ptr   += n;
  }
#   ifdef WUBUF_DEBUG
    MBUF_CKBUF();
#   endif
}

// simple utility function
mbuf::size_type mbuf::doVecAppend(const void *from, size_type indx, size_type nleft)
{
  char *to = ((char *)vecs_[indx].iov_base + vecs_[indx].iov_len);
  size_type space = iovSpace(indx);

  if (nleft <= space) {
    vecs_[indx].iov_len += nleft;
    space = nleft;
  } else {
    vecs_[indx].iov_len += space;
  }
  plen_ += space;
  if (pend_ < indx)
    pend_ = indx;

  memcpy((void *)to, from, space);
  return space;
}

void mbuf::append(const char *ptr, size_type len)
{
#   ifdef WUBUF_DEBUG
    MBUF_CKBUF();
#   endif
  // WULOG((wulogBuf, wulogLoud, "append: adding %d bytes. Buff: %s\n", len, mbuf::toString().c_str()));

  // before increasing the buffer size, we need to determine where
  // to start writing after we grow the buffer(s)
  if (len == 0 || ptr == NULL)
    return;

  size_type newLen = plen_ + len;

  // make sure we have room
  psize(newLen);

  size_type nleft = len;
  for (size_type indx = pend_; indx < mcnt_ && nleft > 0 ; ++indx) {
    size_type n = doVecAppend(ptr, indx, nleft);
    ptr   += n;
    nleft -= n;
  }

  if (pend_ >= mcnt_) {
    reportError("append: Error appending data\n");
  }

  // WULOG((wulogBuf, wulogLoud, "append (After): %s\n", mbuf::toString().c_str()));
#   ifdef WUBUF_DEBUG
    MBUF_CKBUF();
#   endif
}

std::string mbuf::toString() const {
  std::ostringstream ss;
  ss << "mbuf: "
      << "\n\tmsize_ "  << (int)msize_  << ", mcnt_ "  << (int)mcnt_
      << "\n\thsize_ "  << (int)hsize_  << ", hlen_  " << (int)hlen_ << ", hoffset " << (int)hoffset_
      << "\n\tpsize_ "  << (int)psize_  << ", plen_  " << (int)plen_ << ", poffset_ "<< (int)poffset_
                        << "\n\t\tpbegin_ " << (int)pbegin_ << ", pend_ "  << (int)pend_
      << "\n\tmlen() "  << (int)mlen()  << ", mcnt() " << (int)mcnt()
      << "\n\tplen() "  << (int)plen()  << ", pcnt() " << (int)pcnt()
      << "\n\thlen() "  << (int)hlen()  << ", hcnt() " << (int)hcnt();
  for (size_type i = 0; i < vmaxCnt; i++) {
    if (vszs_[i].iov_len > 0) {
      ss << "\n\tvec["  << (int)i << "]"
          << " base = "   << vecs_[i].iov_base << ", len = " << (int)vecs_[i].iov_len
          << ", Abase = " << vszs_[i].iov_base << ", Alen = "<< (int)vszs_[i].iov_len
          << "\n\t\tOffset " << (int)iovOffset(i)
            << ", iovDLen " << (int)iovDLen(i)
            << ", iovSpace " << (int)iovSpace(i);
    }
  }
  return ss.str();
}
/* -------------------------------------------------------------------------- */

// who is the function name and ln the line number
void wunet::mbuf::ckbuf(const std::string& who, int ln, mbuf::size_type hl) const
{
  bool err = false;
  if (hl > 0 && hl != hlen()) {
    wulog(wulogBuf, wulogFatal,
        "mbuf::ckbuf(%s:%d): hlen() (%u) != header\'s dlen() (%u)\n",
        who.c_str(), ln, hlen(), hl);
    err = true;
  }
  // message parameters
  if (msize_ != hsize_ + psize_) {
    wulog(wulogBuf, wulogFatal,
          "mbuf::ckbuf(%s:%d): msize_ (%u) != hsize_ (%u) + psize_ (%u)\n",
          who.c_str(), ln, msize_, hsize_, psize_);
    err = true;
  }

  // payload total versus data
  if (plen_ > psize_) {
    wulog(wulogBuf, wulogFatal,
        "mbuf::ckbuf(%s:%d): plen_ (%u) Not <= psize_ (%u)\n", 
        who.c_str(), ln, plen_, psize_);
    err = true;
  }

  // header total versus data
  if (hlen_ > hsize_) {
    wulog(wulogBuf, wulogFatal,
        "mbuf::ckbuf(%s:%d): hlen_ (%u) Not <= hsize_ (%u)\n", 
        who.c_str(), ln, hlen_, hsize_);
    err = true;
  }

  if (hlen_ != vecs_[HSTART_].iov_len + hoffset_) {
    wulog(wulogBuf, wulogFatal, "mbuf::ckbuf(%s:%d): hlen_ (%u) Not = vecs_[HSTART_].iov_len (%u) + hoffset_ (%u)\n", 
        who.c_str(), ln, hlen_, vecs_[HSTART_].iov_len + hoffset_);
    err = true;
  }

  if (iovOffset(HSTART_) != hoffset_) {
    wulog(wulogBuf, wulogFatal, "mbuf::ckbuf(%s:%d): iovOffset(HSTART_) (%u) Not = hoffset_ (%u)\n",
        who.c_str(), ln, iovOffset(HSTART_), hoffset_);
    err = true;
  }

  if (hsize_ != iovSpace(HSTART_) + hlen_) {
    wulog(wulogBuf, wulogFatal, "mbuf::ckbuf(%s:%d): hsize_ (%u) Not = iovSpace(HSTART_) (%u) + hlen_ (%u) + hoffset_ (%u)\n",
        who.c_str(), ln, hsize_, iovSpace(HSTART_), hlen_, hoffset_);
    err = true;
  }

  if (pbegin_ > pend_) {
    wulog(wulogBuf, wulogFatal, "mbuf::ckbuf(%s:%d): pbegin_ %u > pend_ %u\n",
        who.c_str(), ln, pbegin_, pend_);
    err = true;
  }

  size_type offsetLenSum  = 0;
  size_type iovLenSum     = 0;
  size_type iovDLenSum    = 0;
  size_type vszsLenSum    = 0;
  size_type availSpaceLen = 0;
  for (size_type i = PSTART_; i < mcnt_; ++i) {
    // adding vec lengths corresponding to payload data minus any offsets
    iovLenSum     += vecs_[i].iov_len;
    // includes offset
    iovDLenSum    += iovDLen(i);
    // total allocated lengths
    vszsLenSum    += vszs_[i].iov_len;
    // should == poffset
    offsetLenSum  += iovOffset(i);
    // psize_ - plen_ == Spacelen
    availSpaceLen += iovSpace(i);

    // badness!
    if (vecs_[i].iov_base < vszs_[i].iov_base) {
      wulog(wulogBuf, wulogFatal,
          "mbuf::ckbuf(%s:%d): vecs_[%u].iov_base < vszs_[%u].iov_base!\n", 
          who.c_str(), ln, i, i);
      err = true;
    }
    if (vecs_[i].iov_len > vszs_[i].iov_len) {
      wulog(wulogBuf, wulogFatal,
          "mbuf::ckbuf(%s:%d): vecs_[%u].iov_len (%u) > vszs_[%u].iov_len (%u)!\n", 
          who.c_str(), ln, i, vecs_[i].iov_len, i, vszs_[i].iov_len);
      err = true;
    }

    // if offset > 0 then these will all have their iov_len's == 0
    if (i < pbegin_) {
      if (vecs_[i].iov_len != 0) {
        wulog(wulogBuf, wulogFatal,
            "mbuf::ckbuf(%s:%d): index (%u) < pstart (%u) but iov_len (%u) > 0\n",
            who.c_str(), ln, i, pbegin_, vecs_[i].iov_len);
        err = true;
      }
    } else if (i == pbegin_) {
      if ((iovOffset(i) + vecs_[i].iov_len + iovSpace(i)) != vszs_[i].iov_len) {
        wulog(wulogBuf, wulogFatal,
            "mbuf::ckbuf(%s:%d): for indx (%u) == pstart (%u): iovOffset %u + iov_len %u + availSpaceLen %u = %u != actual %u\n",
            who.c_str(), ln, i, pbegin_,
            iovOffset(i), vecs_[i].iov_len, iovSpace(i), (iovOffset(i) + vecs_[i].iov_len + iovSpace(i)), vszs_[i].iov_len);
        err = true;
      }
    }

  }
  if (poffset_ != offsetLenSum) {
    wulog(wulogBuf, wulogFatal,
          "mbuf::ckbuf(%s:%d): poffset_ (%u) Not == sum of base offsets (%u)\n",
          who.c_str(), ln, poffset_, offsetLenSum);
    err = true;
  }
  if (iovLenSum + poffset_ != plen_) {
    wulog(wulogBuf, wulogFatal,
          "mbuf::ckbuf(%s:%d): plen_ (%u) Not == sum of iov_len\'s (%u) + poffset_ (%u)\n", 
          who.c_str(), ln, plen_, iovLenSum, poffset_);
    err = true;
  }
  if(iovLenSum != plen()) {
    wulog(wulogBuf, wulogFatal,
          "mbuf::ckbuf(%s:%d): iovLenSum (%u) Not == plen() (%u)\n", 
          who.c_str(), ln, iovLenSum, plen());
    err = true;
  }

  if (iovDLenSum != plen_) {
    wulog(wulogBuf, wulogFatal,
          "mbuf::ckbuf(%s:%d): Sum iov_len + iovSpace (%u) Not = plen_ (%u)\n", 
          who.c_str(), ln, iovDLenSum, plen_);
    err = true;
  }

  if ((plen_ + availSpaceLen) != psize_) {
    wulog(wulogBuf, wulogFatal,
          "mbuf::ckbuf(%s:%d): psize_ (%u) Not == sum of plen_ (%u) + availSpaceLen (%u) = %u\n", 
          who.c_str(), ln, psize_, plen_, availSpaceLen, iovLenSum+availSpaceLen);
    err = true;
  }
  if (vszsLenSum != psize_) {
    wulog(wulogBuf, wulogFatal,
          "mbuf::ckbuf(%s:%d): psize_ (%u) Not == sum of vszs_[].iov_len\'s (%u)\n", 
          who.c_str(), ln, psize_, vszsLenSum);
    err = true;
  }

  if (err)
    std::cout << "*******:\n" << *this << "\n**********\n";
}
