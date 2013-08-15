/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/Attic/handleTest2.cc,v $
 * $Author: fredk $
 * $Date: 2006/04/17 22:56:54 $
 * $Revision: 1.1 $
 * $Name:  $
 *
 * File:         main.cc
 * Created:      Tue Nov 02 2004 14:41:35 CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *   Non-intrusive counted pointer example.
 *
 */

#include <iostream>
#include <string>
#include <exception>

//using namespace std;

#include <wuPP/handle.h>

using std::cout;
using std::cerr;
using std::endl;
using std::string;

struct testClass {
  int    cnt;
  std::string str;
  static int numObjects;

  testClass()
    : cnt(0), str("Default")
    {++numObjects;}
  testClass(int i)
    : cnt(i), str("Int")
    {++numObjects;}
  testClass(const std::string &s)
    : cnt(0), str(s)
    {++numObjects;}
  testClass(int i, const std::string &s)
    : cnt(i), str(s)
    {++numObjects;}
  testClass(const testClass &tc)
    : cnt(tc.cnt+1), str(std::string("copy_")+tc.str)
    {++numObjects;}

  testClass &operator+=(const testClass &tc)
  {
    cnt += tc.cnt;
    str = str + std::string("_") + tc.str;
    return *this;
  }

  testClass &operator+=(int x)
  {
    cnt += x;
    str += string("_") + str;
    return *this;
  }

  testClass &operator=(const testClass &tc)
  {
    cnt = tc.cnt;
    str = std::string("op=_") + tc.str;
    return *this;
  }

  ~testClass() {--numObjects;}
};

testClass operator+(const testClass &tc1, const testClass &tc2);
testClass operator+(const testClass &tc1, const testClass &tc2)
{
  testClass tmp(tc1);
  return tmp += tc2;
}

bool testConstructors();
template <class T> bool ckHandle(Handle<T>&, int cnt = -1);
template <class T> bool ckHandle(Handle<T>&, const T*, int cnt = -1);

template <class T> bool ckHandle(Handle<T>&h, int cnt)
{
  bool result = true;

#if 0
  if (! h.ckState() ) {
    cerr << "\t*** ckHandle: failed h.ckState()\n";
    result = false;
  }
#endif
  if ( ! h.ckState(cnt) ) {
    cerr << "\t*** ckHandle: failed h.ckState(cnt)\n";
    result = false;
  }

  return result;
}

template <class T> bool ckHandle(Handle<T>&h, const T *ptr, int cnt = -1)
{
  bool result = true;
  if (h.objPtr() != ptr) {
    cerr << "\t*** ckHandleT: objPtr (" << h.objPtr() << ") != ptr (" << ptr << ")\n";
    result = false;
  }
  return result && ckHandle(h, cnt);
}

int testClass::numObjects = 0;

std::ostream &operator<<(std::ostream &os, const testClass &tc);
inline std::ostream &operator<<(std::ostream &os, const testClass &tc)
{
  return os << "<" << tc.str << ", " << tc.cnt << ">";
}

bool ckTestClass(const testClass *tc, const std::string str, int cnt)
{
  bool result = true;
  if (tc == NULL)
    return true;
  if (tc->str != str) {
    result = false;
    cerr << "\t*** ckTestClass: str (" << tc->str << ") != \"" << str << "\"\n";
  }
  if (tc->cnt != cnt) {
    result = false;
    cerr << "\t*** ckTestClass: cnt (" << tc->cnt << ") != " << cnt << "\n";
  }
  return result;
}

bool testConstructors()
{
  bool result = true;
  {
    Handle<testClass, RefCntPolicy> defHandle;
    if (! ckHandle(defHandle, (testClass *)NULL, 0)) {
      cerr << "\t*** testConstructors: Default Handle constructor broken!\n\t" << defHandle << endl;
      result = false;
    }

    testClass *tc1 = new testClass(0, "tcHandle");
    Handle<testClass, RefCntPolicy> tcHandle(tc1);
    if (! ckHandle(tcHandle, tc1, 1)) {
      cerr << "\t*** testConstructors: Assign constructor broken!\n\t" << tcHandle << endl;
      result = false;
    }

    Handle<testClass, RefCntPolicy> cpHandle(tcHandle);
    if (! ckHandle(cpHandle, tc1, 2)) {
      cerr << "\t*** testConstructors: Copy constructor broken!\n\t" << cpHandle << endl;
      result = false;
    }

    testClass *tc2 = new testClass(0, "exHandle");
    Handle<testClass, RefCntPolicy> exHandle(tc2);
    if (! ckHandle(exHandle, tc2, 1)) {
      cerr << "\t*** testConstructors: Assign (#2) constructor broken!\n\t" << tcHandle << endl;
      result = false;
    }
  }
  if (testClass::numObjects != 0) {
    cerr << "\t*** testConstructors: Not all objects destroyed!\n";
    result = false;
  }

  return result;
}

bool testOps()
{
  bool result = false;

  Handle<testClass, RefCntPolicy> handle;
  testClass *tc0 = new testClass(0, "tc0");
  testClass *tc1 = new testClass(1, "tc1");
  testClass *tc2 = new testClass(2, "tc2");
  Handle<testClass, RefCntPolicy> h0(tc0);
  Handle<testClass, RefCntPolicy> h1(tc1);
  Handle<testClass, RefCntPolicy> h2(tc2);

  // numObjects == 3
  if (!ckTestClass(tc0, h0->str, h0->cnt)) {
    cerr << "\t***testOps: Error with arrow operator or object construction\n";
    result = false;
  }

  // numObjects still == 3 since handle did not have a testClass object
  handle = h0;
  if (! ckHandle(handle, tc0, 2)) {
    cerr << "\t***testOps: Assignment operator to handle broken!\n\t" << handle << endl;
    result = false;
  }
  if (! ckHandle(h0, tc0, 2)) {
    cerr << "\t***testOps: Assignment operator (FROM) broken\n";
    result = false;
  }
  if (testClass::numObjects != 3) {
    cerr << "\t***testOps: check of numObjects (" << testClass::numObjects
         << ") after assignment != 3!\n";
    result = false;
  }

  *handle = *h1 + *h2;
  if (! ckHandle(handle, tc0, 2)) {
    cerr << "\t***testOps: Dereference operator may be broken\n";
    cerr << "\tObj = " << handle << endl;
    result = false;
  }
  if (testClass::numObjects != 3) {
    cerr << "\t***testOps: check of numObjects after sum and assignment broken, should be 3\n";
    result = false;
  }

  handle.clear();
  if (testClass::numObjects != 3) {
    cerr << "\t***testOps: clear() is broken: numObjects (" << testClass::numObjects << ") != 3\n";
    result = false;
  }
  if (handle.objPtr() != NULL) {
    cerr << "\t***testOps: clear() broken\n";
    result = false;
  }
  h0.clear();
  if (testClass::numObjects != 2) {
    cerr << "\t***testOps: clear() is broken: numObjects (" << testClass::numObjects << ") != 2\n";
    result = false;
  }

  return result;
}

int main()
{
  try {
    cout << "Testing constructors ...\n";
    testConstructors();
    cout << "Testing operators ...\n";
    testOps();

    if (testClass::numObjects != 0)
      cerr << "*** main: After running tests some testClass objects were not released!\n";

  } catch (std::exception& ne) {
    std::cerr << "\t*** Caught exception in main\n" << ne.what() << std::endl;
  } catch (...) {
    std::cerr << "\t*** Caught exception in main\n";
  }

  return 0;
}
