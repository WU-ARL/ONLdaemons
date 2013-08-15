/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/Attic/handleTest1.cc,v $
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

#include <handler/handle.h>
#include <valueObject/value.h>

using namespace std;
//#include "Number.h"
//#include "named.h"
template <class T> void fn(Handle<T> arg);
template <class T> void gn(Handle<T>& arg);

// These template functions are just a convenience, so I don't
// have to repeat this code for each class type (handle type)
template <class T, template <class> class Y>
void fn(Handle<T, Y> arg)
{
  cout << "\tFN: arg Before <" << arg << ">\n";
  Handle<T, Y> h1(arg);
  cout << "\t\th1  <" << h1  << ">\n";
  Handle<T, Y> h2(h1);
  cout << "\t\th2  <" << h2  << ">\n";
  cout << "\tFN: arg After <" << arg << ">\n";
  return;
}

template <class T, template <class> class Y>
void gn(Handle<T, Y>& arg)
{
  cout << "\tGN: arg Before <" << arg << ">\n";
  Handle<T, Y> h1(arg);
  cout << "\t\t--*h1 += 1--\n";
  //h1 +=  Handle<T, Y>(1);
  cout << " \t\t-- h1  <" << h1  << "> --\n";
  Handle<T, Y> h2(h1);
  h2 = h2 + h1;
  cout << "\t\th2  <" << h2  << ">\n";
  cout << "\tGN: arg After <" << arg << ">\n";

  //cout << "\t\t--*h1 + 8--\n";
  //*h1 = *h1 + 8;
  //cout << "\t\th1  <" << h1  << ">\n";
  return;
}

int main()
{
  try {
    // creae a few Handles
    Handle<valScalar<int>, DeepCopyPolicy>         intPtr(new valScalar<int>(3));
    Handle<valScalar<double>, DeepCopyPolicy>      numPtr(new valScalar<double>(4));
    Handle<valScalar<char>, DeepCopyPolicy> namePtr(new valScalar<char>('A')); //NamedObject("No1", 5, NamedObject::RW));
  
    cout << "\nCalling FN, which is call by value (i.e. makes a copy of the arg)\n";
    cout << "main: intPtr before calling fn<" << intPtr << ">\n";
    fn(intPtr);
    cout << "main: numPtr before calling fn<" << numPtr << ">\n";
    fn(numPtr);
    cout << "main: namePtr before calling fn<" << namePtr << ">\n";
    fn(namePtr);
  
    cout << "\nNow calling GN, which takes a reference\n";
    cout << "main: intPtr before calling GN<" << intPtr << ">\n";
    gn(intPtr);
    cout << "main: numPtr before calling GN<" << numPtr << ">\n";
    gn(numPtr);
    cout << "main: namePtr before calling GN<" << namePtr << ">\n";
    gn(namePtr);
  
    cout << "\nNow getting a reference to the int and changing it\n";
    //int &i = *intPtr;
    //i = 333;
    //cout << "main: &i = *intPtr = 333 <" << intPtr << ">\n";
    fn(intPtr);
  } catch (exception& ne) {
    cerr << "Caught exception in main\n" << ne.what() << endl;
  } catch (...) {
    cerr << "Caught exception in main\n";
  }

  return 0;
}
