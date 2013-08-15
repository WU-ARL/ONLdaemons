/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/tmpUtils.h,v $
 * $Author: fredk $
 * $Date: 2006/04/17 22:56:55 $
 * $Revision: 1.1 $
 * $Name:  $
 *
 * File:         tmpUtils.h
 * Created:      04/03/06 17:15:12 CDT
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#ifndef _TMPUTILS_H
#define _TMPUTILS_H

// Template meta programming utuls. many of these came from "Modern C++
// Design: Generic Programming and Design Patters" by Andrei Alexandrescu.
// Also, Extensible Templates: Via Inheritance or Traits?
// More Exceptional C++ by Herb Sutter,
// http://www.gotw.ca/publications/mxc++-item-4.htm

// For requiring a clone method
template <class T> class hasClone {
  public:
    // instantiating this class template will cause the constructor to be
    // instantiated which defines a funciton pointer to the clone method or
    // class T. If T doesn't have a clone method the nthe compiler signals an
    // error.
    static void Constraints()
    {
      T* (T::*test)() const = &T::clone;
      test; // suppress warnings about unused variables
    }
    hasClone() { void (*p)() = Constraints;}
};

// Using traits
template <class T> class PolicyTraits {
  public:
    // default behavior is to simply use the new operator, that is assume the
    // type T is the actual concrete (dynamic) type of the object pointed to
    // by ptr. However, if ptr points to a derived object then you will need
    // to specialize this traits class to clone the object (see valoueObject).
    static T* clone(const T *ptr) {return new T(*ptr);}
};

// Type selection
template <bool flag, class T, class U> struct Select {
  typedef T Result;
};
template <class T, class U> struct Select<false, T, U> {
  typedef U Result;
};

// Using the Select Template
// template <class T, bool isPolymorphic> class SomeClass {
//   typedef Select<isPolymorphic, T*, T>::Result value_type;
//   ...
// };

// Converting and interger value to a type
template <int I> struct int2Type {
  enum {value = I};
};

// template <class T, bool isPolymorphic> class SomeClass {
//   private:
//     void someMethod(T*x, int2Type<true>) { T *newObj = x->clone(); ...}
//     void someMethod(T*x, int2Type<false>){ T *newObj = new T(*x); ...}
//   public:
//     void someMethod(T*x) {someMethod(x, int2Type<isPolymorphic>());
// };


// class base {...};
// class derived : public base {...};
// base *A = new derived();
// So derived from base if can convert from derived to base
// Conversion<Derived *, Base *>::exists
#if 0
template <class fromT, class toT> class Conversion {
  private:
    // sizeof(char) := 1, so sizeof(Big) >= Small
    typedef char Small;
    class Big {char pad[2];};
    static Small testType(toT);
    static Big   testType(...);
    static fromT makeFrom();
  public:
    // sizeof(expression) equals the sif of the resulting value of the
    // expression. In this case the return type of hte selected testType method.
    // if fromT can be converted to toT then the test method returning 
    // Small is selected, otherwise the one returning Big is selected.
    enum { exists = sizeof(testType(makeFrom())) == sizeof(Small) };
    enum { exists2Way = exists && Conversion<toT, fromT>::exists };
    enum { sameType = 0 };
};

template <class T> class Conversion<T, T> {
  public:
    enum {exists = 1, exists2Way = 1, sametype = 1 };
};

// T is derived from Base if we can convert T to base and Base is not void. In
// particular the degenerate case of T is derived from T is true.
#define DERIVEDFROM(Base,T) \
  (Conversion<const T*, const Base*>::exists && !Conversion<const Base*, const void*>::sameType)
#endif

#endif
