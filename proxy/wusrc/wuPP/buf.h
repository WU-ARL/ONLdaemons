/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/buf.h,v $
 * $Author: fredk $
 * $Date: 2006/12/08 20:34:28 $
 * $Revision: 1.26 $
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

#ifndef _WUNET_MBUF_H
#define _WUNET_MBUF_H

#include <algorithm>

//#include <wuPP/net.h>
#include <sys/uio.h>

#include <iomanip>

namespace wunet {

  class mbuf {
    public:
      typedef struct iovec iov_t;
      typedef size_t size_type;
      // State transitions are entirely under the control of derived classes
      // (or clients). Expected transitions are:
      //   Ready -> [ HPartial -> ] Header -> [ PPartial -> ] Complete 
      enum msgState_t {Ready, HPartial, Header, PPartial, Complete, Error};
      static std::string toString(mbuf::msgState_t st);

      // allocate MA bytes for the header _and_ MA bytes for the payload
      mbuf(size_type ma = minAlloc);
      // allocate HS bytes for the header and PS bytes for the payload
      mbuf(size_type hs, size_type ps=minAlloc);
      // free all allocated memory
      virtual ~mbuf();

      // ******** Methods to access the entire Message ********
      // pointer to the buffer containing the start of message
      iov_t    *mvec() {return hlen() > 0 ? vecs_ : pvec();}
      // message buffer count for entire message
      int       mcnt() const {return hcnt()  + pcnt();}
      // message byte count (includes all buffers)
      size_type mlen() const {return hlen()  + plen();}
      // total memory allocated but not necessarily used
      size_type msize() const {return hsize() + psize();}
      // move current byte pointer to an absolute offset from start of messge
      void       seek(size_type);
      void      mseek(size_type sz) {seek(sz);}
      size_type  offset() const {return hoffset_ + poffset_;}
      size_type moffset() const {return offset();}
      // move byte offset pointers to start of message
      void       rewind();
      void      mrewind() {rewind();}

      // Message State checks
      bool msgComplete() const {return st_ == Complete;}
      bool msgPartial()  const {return st_ == HPartial || st_ == PPartial;}
      bool msgError()    const {return st_ == Error;}
      bool msgReady()    const {return st_ == Ready;}
      msgState_t state() const {return st_;}
      void state(msgState_t st) {st_ = st;}

      // ***** Methods that a subclass may override *****
      virtual void  reset() {hreset(); preset();}
      virtual void mreset() {reset();}
      virtual void  clear() {hclear(); pclear();}
      virtual void mclear() {clear();}
      // add data to the end of the payload. Data is added at offset == plen.
      virtual void append(const char *, size_type);
      // overwrite buffer with supplied data. plan set to the size_type param.
      virtual void pcopyin(const char *, size_type);
      // copes payload into the supplied buffer
      virtual void pcopyout(char *, size_type) const;

      //  ******** Methods for the Payload ********
      // pointer to the first valid payload buffer
      iov_t    *pvec() {return plen() > 0 ? &vecs_[pbegin_] : NULL;}
      // buffer count for payload of message
      int       pcnt() const {return pend_ - pbegin_ + 1;}
      // byte count for payload of message
      size_type plen() const {return plen_ - poffset_;}
      void      plen(size_type len);
      // total size of allocated buffer(s) for the payload
      size_type psize() const {return psize_;};
      void      psize(size_type len);
      // move current byte pointer to start of payload buffer(s)
      void      prewind();
      // move the current byte offset, skips first 'offset' bytes of the payload.
      void      pseek(size_type offset);
      // report the current offset
      size_type poffset() const {return poffset_;}
      // use to ensure len bytes are in the first payload buffer, also use to
      // get a pointer to this payload buffer
      void *ppullup();
      // resets all values to the maximums: plen == psize, pcnt = mcnt-hcnt
      void preset();
      // erase all data: plen = 0, pcnt = 0
      void pclear();

      // ******** Methods for the Header ********
      // pointe to the header buffer
      iov_t    *hvec() {return (hlen() > 0) ? &vecs_[HSTART_] : NULL;}
      // number of buffers containing valid header data
      int       hcnt() const {return hlen() > 0 ? 1 : 0;}
      // byte count for the message header
      size_type hlen() const {return hlen_ - hoffset_;}
      void      hlen(size_type len);
      // total size of allocated buffer(s) for the header
      void      hsize(size_type len);
      size_type hsize() const {return hsize_;};
      // move current byte pointer to start of header buffer(s)
      void      hrewind();
      // move current byte pointer to the start of the header buffer
      // move the current byte offset for header
      void      hseek(size_type);
      size_type hoffset() {return hoffset_;}
      void *hpullup();
      // reset header values to max valus: hlen = hsize, hcnt = 1
      void hreset();
      void hclear();

      void ckbuf(const std::string &, int, size_type hlen=0) const;
#define MBUF_CKBUF() ckbuf(__func__, __LINE__)
#define MBUF_CKBUF2(hl) ckbuf(__func__, __LINE__, (hl))

      std::ostream &printHeader(std::ostream &ss, size_t width=16) const;
      std::ostream &printData(std::ostream &ss, size_t width=16) const;

      virtual std::string toString() const;

    protected:
      msgState_t st_;

    private:
      // Allocate message memory for head and payload. Header is always the
      // first buffer and the payload consumes the remaining buffers.
      // Sets:  msize_, mcnt_, hsize_, psize_
      void alloc_vec(size_type n, void *buf=0);
      void free_vec();
      void free_all();
      // XXX void dgrow(size_type Total);

      size_type iovOffset(size_type indx) const;
      size_type iovDLen(size_type indx)  const;
      size_type iovSpace(size_type indx) const;

      void reportError(const std::string&);
      size_type doVecAppend(const void *from, size_type indx, size_type len);

    private:
      // UIO_MAXIOV on linux is 1024, define our own limit <= UIO_MAXIOV
      static const size_type  vmaxCnt = 8;
      static const size_type minAlloc = 512; // for testing use 8, otherwise 512

      // Array of message buffers
      iov_t vecs_[vmaxCnt]; // start and length of valid data
      iov_t vszs_[vmaxCnt]; // start and length of allocated data
      //
      // Messsage totals
      size_type msize_; // total memory allocated, header plus pauload
      size_type mcnt_;  // total number of vecs allocated, header plus payload
      //
      // header
      size_type hsize_; // total bytes allocated for the header buffer
      size_type hlen_;  // bytes of valid header data
      size_type hoffset_; // buffer offset to start of data
      static const size_type HSTART_ = 0; // starting index of buffer allocated for header

      // Buffers allocated for the payload
      size_type psize_; // total bytes allocated
      size_type plen_;  // bytes of valid data
      size_type poffset_; // byte offset to start of payload
      size_type pbegin_; // first payload buffer containing valid data
      size_type pend_; // Last payload buffer containing valid data
      static const size_type PSTART_ = 1; // starting index of allocated pauload buffers
  };

  inline std::ostream &operator<<(std::ostream &os, mbuf::msgState_t &st);
  inline std::ostream& operator<<(std::ostream&, const mbuf&);

  inline std::ostream& operator<<(std::ostream& os, const mbuf& mb)
  {
    os << mb.toString();
    return os;
  }

  inline std::ostream &operator<<(std::ostream &os, mbuf::msgState_t &st)
  {
    return os << mbuf::toString(st);
  }

  inline void mbuf::pclear()
  {
    plen_    = 0;
    pbegin_  = PSTART_;
    pend_    = PSTART_;
    poffset_ = 0;

    for (size_type indx = PSTART_; indx < mcnt_; ++indx) {
      vecs_[indx].iov_base = vszs_[indx].iov_base;
      vecs_[indx].iov_len  = 0;
    }
#   ifdef WUBUF_DEBUG
      MBUF_CKBUF();
#   endif
  }

  inline void mbuf::preset()
  {
    // preset can be called by internal functions to 'fix' state
    // variables. It is also called by clients to set plen
    // equal to the max size.  Because of this I do not call the
    // constraint checker on entry during debug.  MBUF_CKBUF();
    plen_    = psize_;
    pbegin_  = PSTART_;
    pend_    = (mcnt_ > PSTART_) ? mcnt_ - 1 : PSTART_;
    poffset_ = 0;

    for (size_type indx = PSTART_; indx < mcnt_; ++indx) {
      vecs_[indx].iov_base = vszs_[indx].iov_base;
      vecs_[indx].iov_len  = vszs_[indx].iov_len;
    }
#   ifdef WUBUF_DEBUG
      MBUF_CKBUF();
#   endif
  }

  inline void mbuf::hreset()
  {
    // preset cam be called by internal functions to 'fix' state
    // variables. I is also called by clients essentially set plen
    // equal to the max size.  Because of this I do not call the
    // constraint checker on entry during debug.  MBUF_CKBUF();
    vecs_[HSTART_].iov_base = vszs_[HSTART_].iov_base;
    vecs_[HSTART_].iov_len  = vszs_[HSTART_].iov_len;
    hlen_    = hsize_;
    hoffset_ = 0;
#   ifdef WUBUF_DEBUG
      MBUF_CKBUF();
#   endif
  }

  inline void mbuf::hclear()
  {
#   ifdef WUBUF_DEBUG
      MBUF_CKBUF();
#   endif

    vecs_[HSTART_].iov_base = vszs_[HSTART_].iov_base;
    vecs_[HSTART_].iov_len  = 0;
    hlen_    = 0;
    hoffset_ = 0;

#   ifdef WUBUF_DEBUG
      MBUF_CKBUF();
#   endif
  }

  inline mbuf::size_type mbuf::iovOffset(size_type indx) const
  {
    if (vecs_[indx].iov_base  == NULL)
      return 0;

    if (indx == HSTART_)
      return hoffset_; // 
    if (indx < pbegin_)
      return vszs_[indx].iov_len;
    if (indx == pbegin_)
      return (size_type)((char *)vecs_[indx].iov_base - (char *)vszs_[indx].iov_base);
    // if (indx > pend_) return 0;
    return (size_type)0;
  }

  inline mbuf::size_type mbuf::iovDLen(size_type indx) const
  {
    return vecs_[indx].iov_len + iovOffset(indx);
  }

  inline mbuf::size_type mbuf::iovSpace(size_type indx)  const
  {
    return vszs_[indx].iov_len - iovDLen(indx);
  }

  inline void mbuf::psize(size_type Total)
  {
#   ifdef WUBUF_DEBUG
      MBUF_CKBUF();
#   endif
    if (Total > psize_)
      alloc_vec(Total - psize_);
#   ifdef WUBUF_DEBUG
      MBUF_CKBUF();
#   endif
  }

  inline void mbuf::hrewind()
  {
#   ifdef WUBUF_DEBUG
      MBUF_CKBUF();
#   endif

    vecs_[HSTART_].iov_base = vszs_[HSTART_].iov_base;
    vecs_[HSTART_].iov_len  = hlen_;
    hoffset_ = 0;

#   ifdef WUBUF_DEBUG
          MBUF_CKBUF(); 
#   endif
  }

  inline void mbuf::rewind()
  {
#   ifdef WUBUF_DEBUG
      MBUF_CKBUF();
#   endif
    hrewind();
    prewind();
#   ifdef WUBUF_DEBUG
      MBUF_CKBUF();
#   endif
  }

  inline void mbuf::prewind()
  {
#   ifdef WUBUF_DEBUG
      MBUF_CKBUF();
#   endif
    if (poffset_ == 0)
      return;

    plen(plen_);

#   ifdef WUBUF_DEBUG
      MBUF_CKBUF();
#   endif
  }

  inline void mbuf::hseek(size_type offset)
  {
#   ifdef WUBUF_DEBUG
      MBUF_CKBUF();
#   endif

    hrewind();
    if (offset > hlen_)
      offset = hlen_;
    vecs_[HSTART_].iov_base = (void *)((char *)vecs_[HSTART_].iov_base + offset);
    vecs_[HSTART_].iov_len  = vecs_[HSTART_].iov_len - offset;
    hoffset_ = offset;

    return;
  }

  inline void mbuf::seek(size_type offset)
  {
    rewind();

    hseek(offset);
    if (hlen() == 0)
      pseek(offset - hlen_);

    return;
  }

  inline void *mbuf::hpullup()
  {
#   ifdef WUBUF_DEBUG
      MBUF_CKBUF();
#   endif
    return (hlen() > 0) ? vecs_[HSTART_].iov_base : NULL;
  }

  inline void mbuf::free_all()
  {
    while (mcnt_ > 0)
      free_vec();
  }


}

#endif
