/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/token.cc,v $
 * $Author: fredk $
 * $Date: 2007/03/16 23:03:08 $
 * $Revision: 1.6 $
 * $Name:  $
 *
 * File:         token.cc
 * Created:      Thu Sep 23 2004 12:14:28 CDT
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#include <iostream>
#include <sstream>
#include <string>

using namespace std;
#include <wuPP/token.h>

std::ostream &operator<<(std::ostream &s, const valToken::token_cat &c)
{
  switch (c) {
    case valToken::tokInvCat:
      s << "Invalid Cat";
      break;
    case valToken::tokNonTerminal:
      s << "NonTerminal";
      break;
    case valToken::tokTerminal:
      s << "Terminal";
      break;
    case valToken::tokInternal:
      s << "tokInternal";
      break;
    default:
      s << "Unknown Cat";
      break;
  }
  return s;
}


std::ostream &operator<<(std::ostream &s, const valToken::token_type &t)
{
  switch (t) {
    case valToken::tokInvType:
      s << "Invalid";
      break;
    case valToken::tokMacroType:
      s << "Macro";
      break;
    case valToken::tokNameType:
      s << "Name";
      break;
    case valToken::tokStringType:
      s << "String";
      break;
    case valToken::tokIntType:
      s << "Int";
      break;
    case valToken::tokDw8Type:
      s << "dw8";
      break;
    case valToken::tokDw4Type:
      s << "dw4";
      break;
    case valToken::tokDw2Type:
      s << "dw2";
      break;
    case valToken::tokDw1Type:
      s << "dw1";
      break;
    case valToken::tokDblType:
      s << "Double";
      break;
    case valToken::tokSubStartType:
      s << "Sub Start \'(\'";
      break;
    case valToken::tokSubEndType:
      s << "Sub End \')\'";
      break;
    case valToken::tokBlockStartType:
      s << "Block Start \'{\'";
      break;
    case valToken::tokBlockEndType:
      s << "Block End \'}\'";
      break;
    case valToken::tokIndxStartType:
      s << "Index Start \'[\'";
      break;
    case valToken::tokIndxEndType:
      s << "Index End \']\'";
      break;
    case valToken::tokEOLType:
      s << "EOL";
      break;
    case valToken::tokExprEndType:
      s << "\';\'";
      break;
    case valToken::tokPunctType:
      s << "Punct";
      break;
    case valToken::tokEOFType:
      s << "EOF";
      break;
    case valToken::tokOpType:
      s << "Operator";
      break;
    case valToken::tokAssignType: // assignment
      s << "Assignment";
      break;
    case valToken::tokAddType:    // addition
      s << "Addition";
      break;
    case valToken::tokSubType:    // subtraction
      s << "Subtraction";
      break;
    case valToken::tokMultType:   // multiplication
      s << "Multiplication";
      break;
    case valToken::tokDivType:    // division
      s << "Division";
      break;
    case valToken::tokModType:    // modulo
      s << "Modulo";
      break;
    case valToken::tokNotType:    // logical not
      s << "Logical Not";
      break;
    case valToken::tokLtType:     // less than
      s << "Less";
      break;
    case valToken::tokLtEqType:   // less or equal
      s << "Less or Equal";
      break;
    case valToken::tokGtType:     // greater than
      s << "Greater";
      break;
    case valToken::tokGtEqType:   // greater or equal 
      s << "Greater or Equal";
      break;
    case valToken::tokEqType:     // logical equal
      s << "Equal";
      break;
    case valToken::tokNotEqType:  // logical not equal
      s << "Not Equal";
      break;
    case valToken::tokBitNotType:
      s << "Bit Not";
      break;
    case valToken::tokBitAndType:
      s << "Bit And";
      break;
    case valToken::tokBitXorType:
      s << "Bit Xor";
      break;
    case valToken::tokBitOrType:
      s << "Bit Or";
      break;
    case valToken::tokLShiftType:  // shift left
      s << "Left Shift";
      break;
    case valToken::tokRShifttType: // shift right
      s << "Right Shift";
      break;
    default:
      s << "Unknown";
  }
  return s;
}

std::ostream &operator<<(std::ostream &s, const valToken::token_format &f)
{
  switch (f) {
    case valToken::tokInvFormat:
      s << "Undefined";
      break;
    case valToken::tokStringFmt:
      s << "String";
      break;
    case valToken::tokBase10Fmt:
      s << "Base 10";
      break;
    case valToken::tokBase16Fmt:
      s << "Base 16";
      break;
    case valToken::tokBuiltInFmt:
      s << "BuiltIn";
      break;
    default:
      s << "Unknown";
      break;
  }
  return s;
}

std::ostream &operator<<(std::ostream &s, const valToken &r)
{
  s << "<"
      << r.str()
      << ", " << r.cat()
      << ", " << r.type()
      << ", " << r.fmat()
      << ", " << r.cnt()
    << ">";
  return s;
}

std::string valToken::toString() const
{
  std::ostringstream ss;
  ss << *this;
  return ss.str();
}
