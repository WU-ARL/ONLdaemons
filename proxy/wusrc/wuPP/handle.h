/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/handle.h,v $
 * $Author: fredk $
 * $Date: 2006/10/30 19:55:51 $
 * $Revision: 1.4 $
 * $Name:  $
 *
 * File:         handle.h
 * Created:      Tue Nov 02 2004 14:41:35 CST
 * Author:       Fred Kuhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *   Non-intrusive counted pointer example.
 *
 */
#ifndef _HANDLE_H
#define _HANDLE_H

// 1) From Bjarne Stroustrup's C++ Style and Technique FAQ,
// http://public.research.att.com/~bs/bs_faq2.html
#include <wuPP/tmpUtils.h>

template <class T> class RefCntPolicy {
  private:
    T   *objPtr_;
    int *objCnt_;

    void    copyT(const T &ref);
    void allocCnt();
    int    incCnt();
    int    decCnt();
    int    objCnt() const;
    void   assign(T*, int*);
  protected:
    // Default constructor, initial pointer to 0.
    RefCntPolicy();
    // Take ownership ... well, atleast allocate the initial reference count
    // and manage the life cycle along with any other handle objects which
    // also reference this T object.
    RefCntPolicy(T *);
    // create new object as copy of passed reference.
    RefCntPolicy(const T &);
    // increment reference count to object
    RefCntPolicy(const RefCntPolicy &);
    // make sure object is free/reference count decremented
    virtual ~RefCntPolicy() {assign(0);};

    // return a non-const qualified pointer to contained object (or NULL)
    T* objPtr() const {return objPtr_;}

    void assign(T *ptr) {assign(ptr, 0);}
    void mkcopy(const RefCntPolicy &rcp) {assign(rcp.objPtr_, rcp.objCnt_);}
    std::ostream &print(std::ostream &os) const
    {
      os << "RefCntPolicy: Cnt " << objCnt() << ", " << "Ptr " << objPtr();
      if (objPtr())
        os << ", Rep " << *objPtr();
      return os;
    }
  public:
    // for debugging
    bool ckState(int cnt = 0)
    {
      if (objPtr() == 0 && objCnt() == 0)
        return true;
      if (objPtr() != 0 && objCnt() == cnt)
        return true;
      return false;
    }
};

// This policy makes a copy of the underlying object in all _but_ one case:
// constructor called with an object pointer.
template <class T> class DeepCopyPolicy {
  private:
    T   *objPtr_;

    void  copyPtr(const T *ptr);
  protected:
    // default constructor, initialize pointer to 0
    DeepCopyPolicy();
    // Take over ownership of pointer:
    //   does NOT make a copy of referenced object.
    DeepCopyPolicy(T *);
    // Makes a copy of object on the heap.
    DeepCopyPolicy(const T &);
    // Perform a deep copy of the contained object.
    DeepCopyPolicy(const DeepCopyPolicy &);
    // make sure object is deleted
    virtual ~DeepCopyPolicy() {assign(0);};

    // return non-const qualified pointer to object, could be 0
    T* objPtr() const {return objPtr_;}
    void assign(T*);
    void mkcopy(const DeepCopyPolicy &dcp);
    std::ostream &print(std::ostream &os) const
    {
      os << "<" << objPtr();
      if (objPtr())
        os << ", " << *objPtr();
      os << ">";
      return os;
    }
};

// Type T must support the following operators:
//   Default constructor
//   operator*()
// The policy class must implement
//   default constructor
//   copy constructor
//   constructor for T*
//   constructor for T& (must make a copy)
//   T* objPtr() const;
//   void assign(const T*);
//   void mkcopy(const Policy&)
//   std::ostream &print(std::ostream &os) const; // for debugging
template <class T, template <class> class OP=RefCntPolicy> class Handle
#ifdef WUDEBUG
  // Only for debugging purposes.
  : public OP<T> {
#else
  // do not want this interface exposed to public
  : public OP<T> {
#endif
  public:
    Handle() {};

    // I am not permitting a default constructor.  A Handle object can only be
    // created with a valid object of type T* (or equivalently another handle
    // object). In other words, we treat Handle as a pointer to an oject of
    // type T, therefore we can only have a Handle if there is a corresponding
    // object of type T which may be referenced.

    // take over ownership of the object pointed to by ptr, regardless of
    // ownership policy.
    explicit Handle(T *ptr)       : OP<T>(ptr) {};
    // Always copy construct a new T object on the heap (and own it).
    explicit Handle(const T &obj) : OP<T>(obj) {};
    // Let the Policy class determine how to copy construct: for example deep
    // copy or reference counting.
    Handle(const Handle& h)       : OP<T>(h) {};

    // destructor -- only delete rep object if reference count becomes zero.
    // Let ownership Policy classes decide how to free resources.
    virtual ~Handle() {clear();};

    // handle takes over ownership of this pointer, regardless of policy. In
    // other words the DeepCopy policy must not make a copy.
    void assign(T *ptr) {OP<T>::assign(ptr);}
    void clear()        {OP<T>::assign(0);}

    // assignment operator ... check for self-assignment!
    Handle& operator=(const Handle &rhs)
    {
      if (this->objPtr() == rhs.objPtr())
        return *this;
      OP<T>::mkcopy(rhs);
      return *this;
    }

    // behave like a pointer!
    T *operator->()   const {return  OP<T>::objPtr();}
    T &operator*()    const {return *OP<T>::objPtr();}
    const T* objPtr() const {return  OP<T>::objPtr();}

    // check that Handle contains a valid object pointer
    bool operator!() const {return objPtr() == NULL;}

    // for debugging only!
    std::ostream &print(std::ostream &os) const {return OP<T>::print(os);}
};

template <class T, template <class> class OP> std::ostream &operator<<(std::ostream &s, const Handle<T,OP>&);

template <class T, template <class> class OP> bool operator==(const Handle<T,OP> &lhs, const Handle<T,OP> &rhs);
template <class T, template <class> class OP> bool operator!=(const Handle<T,OP> &lhs, const Handle<T,OP> &rhs);
template <class T, template <class> class OP> bool operator<=(const Handle<T,OP> &lhs, const Handle<T,OP> &rhs);
template <class T, template <class> class OP> bool operator< (const Handle<T,OP> &lhs, const Handle<T,OP> &rhs);
template <class T, template <class> class OP> bool operator>=(const Handle<T,OP> &lhs, const Handle<T,OP> &rhs);
template <class T, template <class> class OP> bool operator> (const Handle<T,OP> &lhs, const Handle<T,OP> &rhs);

#include <wuPP/handle.cc>
#endif
