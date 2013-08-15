/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/handle.cc,v $
 * $Author: fredk $
 * $Date: 2006/05/15 22:39:55 $
 * $Revision: 1.2 $
 * $Name:  $
 *
 * File:         handle.cc
 * Created:      03/29/06 07:57:38 CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

// ------------ RefCntPolicy ------------
template <class T> void RefCntPolicy<T>::copyT(const T &ref)
{
  // a new T will also get a new count so first decrement then remove
  // reference to current count
  T *obj;
  try {
    obj = PolicyTraits<T>::clone(&ref);
    assign(obj, 0);
  } catch (...) {
    if (obj)
      delete obj;
    // assign doesn't throw an exception, only the new operator does.
    // so we don't need to clean up this object. It stays as it was.
  }
}

template <class T> void RefCntPolicy<T>::allocCnt()
{
  try {
    if (objPtr_)
      objCnt_ = new int(0);
  } catch (...) {
    // don't really care what exception (bad_alloc)
    objPtr_ = NULL;
    objCnt_ = NULL;
  }
}

template <class T> int RefCntPolicy<T>::incCnt()
{
  // this is where you would add a lock.
  return (objCnt_ ? ++(*objCnt_) : 0);
}

template <class T> int RefCntPolicy<T>::decCnt()
{
  // decrement objPtr_'s reference count and delete of it becomes 0.
  if (objCnt_ == NULL) return 0;
  if(--(*objCnt_) == 0) {
    delete objPtr_;
    objPtr_ = NULL;
    delete objCnt_;
    objCnt_ = NULL;
    return 0;
  }
  return *objCnt_;
}

template <class T> int RefCntPolicy<T>::objCnt() const
{
  return (objCnt_ ? *objCnt_ : 0);
}

template <class T> RefCntPolicy<T>::RefCntPolicy()                          : objPtr_(0), objCnt_(0) {}
template <class T> RefCntPolicy<T>::RefCntPolicy(T *ptr)                    : objPtr_(0), objCnt_(0) {assign(ptr);}
template <class T> RefCntPolicy<T>::RefCntPolicy(const T &obj)              : objPtr_(0), objCnt_(0) {copyT(obj);}
template <class T> RefCntPolicy<T>::RefCntPolicy(const RefCntPolicy<T> &op) : objPtr_(0), objCnt_(0) {mkcopy(op);}

template <class T> void RefCntPolicy<T>::assign(T *optr, int *cptr)
{
  decCnt();
  if (optr == NULL) {
    objPtr_ = NULL;
    objCnt_ = NULL;
  } else {
    objPtr_ = optr;
    if (cptr == NULL)
      allocCnt();
    else
      objCnt_ = cptr;
  }
  incCnt();
}

// --------------- DeepCopyPolicy --------------------------
template <class T> DeepCopyPolicy<T>::DeepCopyPolicy()                         : objPtr_(0) {}
template <class T> DeepCopyPolicy<T>::DeepCopyPolicy(T *ptr)                   : objPtr_(0) {assign(ptr);}
template <class T> DeepCopyPolicy<T>::DeepCopyPolicy(const T &obj)             : objPtr_(0) {copyPtr(&obj);}
template <class T> DeepCopyPolicy<T>::DeepCopyPolicy(const DeepCopyPolicy &op) : objPtr_(0) {mkcopy(op);}

template <class T> void DeepCopyPolicy<T>:: mkcopy(const DeepCopyPolicy &dcp) { copyPtr(dcp.objPtr_); }
template <class T> void DeepCopyPolicy<T>::copyPtr(const T *ptr)
{
  T *old  = objPtr_;
  objPtr_ = 0;
  if (ptr) {
    try {
      objPtr_ = PolicyTraits<T>::clone(ptr);
    } catch (...) {
      std::cerr << "DeepCopyPolicy::assign: Caught exception while cloning.\n";
      if (objPtr_) {
        delete objPtr_;
        objPtr_ = 0;
      }
      objPtr_ = old;
      old = 0;
    }
  }
  delete old; // OK to delete NULL pointer
}

template <class T> void DeepCopyPolicy<T>::assign(T *ptr)
{
  if (objPtr_ != NULL)
    delete objPtr_;
  objPtr_ = ptr;
}

// --------------- Handle ------------------
template <class T, template <class> class OP> std::ostream &operator<<(std::ostream &s, const Handle<T,OP>& h)
  { return h.print(s); }
template <class T, template <class> class OP> bool operator==(const Handle<T,OP> &lhs, const Handle<T,OP> &rhs)
  {return lhs.objPtr() == rhs.objPtr();}
template <class T, template <class> class OP> bool operator!=(const Handle<T,OP> &lhs, const Handle<T,OP> &rhs)
  {return lhs.objPtr() != rhs.objPtr();}
template <class T, template <class> class OP> bool operator<=(const Handle<T,OP> &lhs, const Handle<T,OP> &rhs)
  {return lhs.objPtr() <= rhs.objPtr();}
template <class T, template <class> class OP> bool operator< (const Handle<T,OP> &lhs, const Handle<T,OP> &rhs)
  {return lhs.objPtr() <  rhs.objPtr();}
template <class T, template <class> class OP> bool operator>=(const Handle<T,OP> &lhs, const Handle<T,OP> &rhs)
  {return lhs.objPtr() >= rhs.objPtr();}
template <class T, template <class> class OP> bool operator> (const Handle<T,OP> &lhs, const Handle<T,OP> &rhs)
  {return lhs.objPtr() >  rhs.objPtr();}
