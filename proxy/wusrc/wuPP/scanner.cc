/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/scanner.cc,v $
 * $Author: fredk $
 * $Date: 2008/02/01 21:06:34 $
 * $Revision: 1.7 $
 * $Name:  $
 *
 * File:         try.cc
 * Created:      03/12/06 09:45:39 CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */
#include <iostream>
#include <string>
#include <vector>
#include <wuPP/token.h>
#include <wuPP/scanner.h>

// One for each token type
namespace wuscan {
bool isMacro(const std::string::const_iterator &ch, const std::string::const_iterator &end);
bool isName(const std::string::const_iterator &ch, const std::string::const_iterator &end);
bool isString(const std::string::const_iterator &ch, const std::string::const_iterator &end);
bool isNum(const std::string::const_iterator &ch, const std::string::const_iterator &end, numMode mode = defNumMode);
bool isDw1Array(const std::string::const_iterator &ch, const std::string::const_iterator &end);
bool isExpr(const std::string::const_iterator &ch, const std::string::const_iterator &end);
bool isBlock(const std::string::const_iterator &ch, const std::string::const_iterator &end);
bool isIndex(const std::string::const_iterator &ch, const std::string::const_iterator &end);
bool isOp(const std::string::const_iterator &ch, const std::string::const_iterator &end);

// these string conversion funcctions will only modify the tok arg if a valid
// conversion is found. It is up to the caller to initialize tok to a known
// value (like Invalid).
std::string::iterator& str2Macro(std::string::iterator &ch,
                                  std::string::iterator &end, valToken &);
std::string::iterator& str2Name(std::string::iterator &ch,
                                  std::string::iterator &end, valToken &);
std::string::iterator& str2String(std::string::iterator &ch,
                                  std::string::iterator &end, valToken &);
std::string::iterator& str2Number(std::string::iterator &ch,
                                  std::string::iterator &end, valToken &, numMode mode = defNumMode);
std::string::iterator& str2NumArray(std::string::iterator &ch,
                                  std::string::iterator &end, valToken &, numMode mode = defNumMode);
std::string::iterator& str2Expr(std::string::iterator &ch,
                                  std::string::iterator &end, valToken &);
std::string::iterator& str2Block(std::string::iterator &ch,
                                  std::string::iterator &end, valToken &);
std::string::iterator& str2Index(std::string::iterator &ch,
                                  std::string::iterator &end, valToken &);
std::string::iterator& str2Op(std::string::iterator &ch,
                                  std::string::iterator &end, valToken &);
std::string::iterator& str2token(std::string::iterator &,
                                  std::string::iterator &, valToken &, wuscan::numMode);
}

bool wuscan::isMacro(const std::string::const_iterator &ch, const std::string::const_iterator &end)
{
  if (ch == end) return false;
  return *ch == '$' && isName(ch+1, end);
}

bool wuscan::isName(const std::string::const_iterator &ch, const std::string::const_iterator &end)
{
  if (ch == end) return false;
  return isalpha(*ch) || *ch == '_';
}

bool wuscan::isString(const std::string::const_iterator &ch, const std::string::const_iterator &end)
{
  if (ch == end) return false;
  if (*ch != '"') return false;
  return true;
}

bool wuscan::isNum(const std::string::const_iterator &ch, const std::string::const_iterator &end, wuscan::numMode mode)
{
  if (ch == end) return false;
  if (mode == wuscan::arrayIgnoreFirst)
    return isdigit(*ch);
  return (isdigit(*ch) || (*ch == '.' && ((ch+1) != end) && isdigit(*(ch+1))));
}

bool wuscan::isDw1Array(const std::string::const_iterator &ch, const std::string::const_iterator &end)
{
  std::string::const_iterator sit = ch;
  while (sit != end && isxdigit(*sit)) ++sit;
  if (sit == end)
    return false;
  return (*sit == ':' || *sit == '.');
}

bool wuscan::isExpr(const std::string::const_iterator &ch, const std::string::const_iterator &end)
{
  if (ch == end) return false;
  if (*ch == '(' || *ch == ')') return true;
  return false;
}

bool wuscan::isBlock(const std::string::const_iterator &ch, const std::string::const_iterator &end)
{
  if (ch == end) return false;
  if (*ch == '{' || *ch == '}') return true;
  return false;
}

bool wuscan::isIndex(const std::string::const_iterator &ch, const std::string::const_iterator &end)
{
  if (ch == end) return false;
  if (*ch == '[' || *ch == ']') return true;
  return false;
}

bool wuscan::isOp(const std::string::const_iterator &ch, const std::string::const_iterator &end)
{
  bool result = false;

  if (ch == end) return false;

  // valid operators
  switch(*ch) {
    case '=': // = ==
    case '+': // +
    case '-': // -
    case '*': // *
    case '/': // /
    case '!': // ! !=
    case '<': // < <= >>
    case '>': // > >= >>
    case '&': // & &&
    case '|': // | ||
    case '^': // ^
      result = true;
      break;
  }
  return result;
}

std::string::iterator &wuscan::str2Op(std::string::iterator &ch, std::string::iterator &end, valToken &tok)
{
  std::string str;
  valToken::token_type tt = valToken::tokInvType;

  if (ch == end)
    return ch;

  switch(*ch) {
    case '=': // = ==
      str += *ch++;
      if (*ch == '=') {
        str += *ch++;
        tt = valToken::tokEqType;
      } else {
        tt = valToken::tokAssignType;
      }

      break;
    case '+': // +
      str += *ch++;
      tt = valToken::tokAddType;
      break;
    case '-': // -
      str += *ch++;
      tt = valToken::tokSubType;
      break;
    case '*': // *
      str += *ch++;
      tt = valToken::tokMultType;
      break;
    case '/': // /
      str += *ch++;
      tt = valToken::tokDivType;
      break;
    case '!': // ! !=
      str += *ch++;
      if (*ch == '=') {
        str += *ch++;
        tt = valToken::tokNotEqType;
      } else {
        tt = valToken::tokNotType;
      }
      break;
    case '<': // < <= <<
      str += *ch++;
      if (*ch == '=') {
        str += *ch++;
        tt = valToken::tokLtEqType;
      } else if (*ch == '<') {
        str += *ch++;
        tt = valToken::tokLShiftType;
      } else {
        tt = valToken::tokLtType;
      }
      break;
    case '>': // > >= >>
      str += *ch++;
      if (*ch == '=') {
        str += *ch++;
        tt = valToken::tokGtEqType;
      } else if (*ch == '>') {
        str += *ch++;
        tt = valToken::tokRShifttType;
      } else {
        tt = valToken::tokGtType;
      }
      break;
    case '~': // Bit not
      str += *ch++;
      tt = valToken::tokBitNotType;
      break;
    case '^': // Bit XOR
      str += *ch++;
      tt = valToken::tokBitXorType;
      break;
    case '|': // | ||
      str += *ch++;
      if (*ch == '|') {
        str += *ch++;
        tt = valToken::tokOrType;
      } else {
        tt = valToken::tokBitOrType;
      }
      break;
    case '&': // & &&
      str += *ch++;
      if (*ch == '&') {
        str += *ch++;
        tt = valToken::tokAndType;
      } else {
        tt = valToken::tokBitAndType;
      }
      break;
    default:
      tt = valToken::tokInvType;
  }
  if (tt != valToken::tokInvType)
    tok.init(str, valToken::tokNonTerminal, tt, valToken::tokBuiltInFmt);

  return ch;
}

std::string::iterator &wuscan::str2Expr(std::string::iterator &ch, std::string::iterator &end, valToken &tok)
{
  std::string str;
  valToken::token_type ttype = valToken::tokInvType;
  valToken::token_cat  tc;

  if (isExpr(ch, end)) {
    if (*ch == '(') {
      ttype = valToken::tokSubStartType;
      tc = valToken::tokNonTerminal;
    } else {
      ttype = valToken::tokSubEndType;
      tc = valToken::tokTerminal;
    }
    str += *ch++;
    tok.init(str, tc, ttype, valToken::tokBuiltInFmt);
  }
  return ch;
}

std::string::iterator &wuscan::str2Block(std::string::iterator &ch, std::string::iterator &end, valToken &tok)
{
  std::string str;
  valToken::token_type tt;
  valToken::token_cat  tc;

  if (isBlock(ch, end)) {
    if (*ch == '{') {
      tt = valToken::tokBlockStartType;
      tc = valToken::tokNonTerminal;
    } else {
      tt = valToken::tokBlockEndType;
      tc = valToken::tokTerminal;
    }
    str += *ch++;
    tok.init(str, tc, tt, valToken::tokBuiltInFmt);
  }
  return ch;
}

std::string::iterator &wuscan::str2Index(std::string::iterator &ch, std::string::iterator &end, valToken &tok)
{
  std::string str;
  valToken::token_type tt;
  valToken::token_cat  tc;

  if (isIndex(ch, end)) {
    if (*ch == '[') {
      tt = valToken::tokIndxStartType;
      tc = valToken::tokNonTerminal;
    } else {
      tt = valToken::tokIndxEndType;
      tc = valToken::tokTerminal;
    }
    str += *ch++;
    tok.init(str, tc, tt, valToken::tokBuiltInFmt);
  }
  return ch;
}

std::string::iterator &wuscan::str2Number(std::string::iterator &ch, std::string::iterator &end, valToken &tok, wuscan::numMode mode)
{
  // only called if the first char is a valid number
  //   Integer: [0-9]+
  //   Hex: 0x[a-f,A-F,0-9]+
  //   Double: .[0-9]+ | [0-9]+.[0-9]*
  std::string str;
  valToken::token_type   ttype = valToken::tokInvType;
  valToken::token_format tfmat = valToken::tokInvFormat;

  if (! isNum(ch, end, mode)) {
    return ch;
  }

  if (*ch == '0' && *(ch+1) == 'x') {
    int cnt = 0;
    ttype = valToken::tokDw4Type;
    tfmat = valToken::tokBase16Fmt;
    str = "0x"; ch += 2;
    while (ch != end && isxdigit(*ch)) {
      str += *ch++;
      ++cnt;
    }
    if (cnt > 8)
      ttype = valToken::tokDw8Type;
  } else {
    // float or int (base 10)
    if ((*ch == '.' && isdigit(*(ch+1))) || isdigit(*ch)) {
      tfmat = valToken::tokBase10Fmt;
      ttype = (*ch == '.') ? valToken::tokDblType : valToken::tokIntType;
      str  += *ch++;
      while (ch != end && (isdigit(*ch) || *ch == '.')) {
        if (*ch == '.') {
          if (ttype == valToken::tokDblType)
            break;
          ttype = valToken::tokDblType;
        }
        str += *ch++;
      }
    }
  }

  tok.init(str, valToken::tokTerminal, ttype, tfmat, 1);
  return ch;
}

std::string::iterator &wuscan::str2NumArray(std::string::iterator &ch, std::string::iterator &end, valToken &tok, wuscan::numMode mode)
{
  //   Byte Array (Hex): 
  //     :hex(:+hex)*
  //     hex:(hex:*)*
  //   Byte Array (Dec):
  //     .dec.(dec.*)*
  //     dec.dec.(dec.*)*
  std::string str;
  bool done = false;

  if (!isDw1Array(ch, end))
    return ch;

  valToken::token_type   ttype = valToken::tokDw1Type;
  valToken::token_format tfmat = valToken::tokInvFormat;
  size_t vlen = 0;

  while (ch != end && done == false) {
    switch (tfmat) {
      case valToken::tokInvFormat:
        if (*ch == ':') {
          tfmat = valToken::tokBase16Fmt;
          if (mode == arrayIgnoreFirst) {
            ++ch;
            vlen = 1;
            continue;
          }
          vlen = 2;
        } else if (*ch == '.') {
          tfmat = valToken::tokBase10Fmt;
          if (mode == arrayIgnoreFirst) {
            ++ch;
            vlen = 1;
            continue;
          }
          vlen = 2;
        } else if (isdigit(*ch)) {
          vlen = 1;
          tfmat = valToken::tokBase10Fmt;
        } else if (isxdigit(*ch)) {
          vlen = 1;
          tfmat = valToken::tokBase16Fmt;
        } else {
          done = true; // not an array
        }
        break;
      case  valToken::tokBase10Fmt:
        // this is the default,
        // if vlen == 1 then we don't really know if this is a base10 or
        // base16 byte array.
        if (vlen == 1) {
          if (*ch == '.') {
            ++vlen;
          } else if (*ch == ':') {
            ++vlen;
            tfmat = valToken::tokBase16Fmt;
          } else if (isxdigit(*ch) && !isdigit(*ch)) {
            tfmat = valToken::tokBase16Fmt;
          } else if (!isdigit(*ch)) {
            done = true; // not a Byte array
          }
        } else {
          // we know this is a Base10 Byte array now
          if (*ch == '.') {
            ++vlen;
          } else if (!isdigit(*ch)) {
            done = true;
          }
        }
        break;
      case valToken::tokBase16Fmt:
        if (*ch == ':') {
          ++vlen;
        } else if (!isxdigit(*ch)) {
          done = true;
        }
        break;
      default:
        // should never happen
        done = true;
        break;
    }
    if (!done)
      str += *ch++;
  } // while
  if (vlen != 0)
    tok.init(str, valToken::tokTerminal, ttype, tfmat, vlen);
  return ch;
}

std::string::iterator &wuscan::str2Name(std::string::iterator &ch, std::string::iterator &end, valToken &tok)
{
  if (!isName(ch, end))
    return ch;

  std::string str;

  // we know the first char is correct since isName() was true
  while (ch != end && (isalnum(*ch) || *ch == '_'))
    str += *ch++;

  tok.init(str, valToken::tokTerminal, valToken::tokNameType, valToken::tokBuiltInFmt, 1);
  return ch;
}

std::string::iterator &wuscan::str2Macro(std::string::iterator &ch, std::string::iterator &end, valToken &tok)
{
  if (!isMacro(ch, end))
    return ch;

  std::string str;

  // get rid of leading '$'
  ++ch;
  //str += *ch++;
  while (ch != end && (isalnum(*ch) || *ch == '_'))
    str += *ch++;

  tok.init(str, valToken::tokTerminal, valToken::tokMacroType, valToken::tokBuiltInFmt, 1);
  return ch;
}

std::string::iterator &wuscan::str2String(std::string::iterator &ch, std::string::iterator &end, valToken &tok)
{
  size_t vlen = 0;

  if (!isString(ch, end))
    return ch;

  std::string str;
  ++ch;
  // we don't keep the '"'
  while (ch != end && *ch != '"') {
    if (*ch == '\\') {
      switch (*(ch+1)) {
        case 'a': // bell
          str += '\a';
          ch += 2;
          break;
        case 'b': // backspace
          str += '\b';
          ch += 2;
          break;
        case 't': // horizontal tab
          str += '\t';
          ch += 2;
          break;
        case 'n': // new line
          str += '\n';
          ch += 2;
          break;
        case 'v': // vertical tab
          str += '\v';
          ch += 2;
          break;
        case 'f': // form feed
          str += '\f';
          ch += 2;
          break;
        case 'r': // carriage return
          str += '\r';
          ch += 2;
          break;
        default: // ???
          ++ch;
          str += *ch++;
          break;
      }
    } else {
      str += *ch++;
    }
    ++vlen;
  }
  ++vlen; // for the last '\0'

  if (*ch == '"') {
    ++ch; // throw out terminating quote
    tok.init(str, valToken::tokTerminal, valToken::tokStringType, valToken::tokStringFmt, vlen);
  } else {
    std::cerr << "str2string: Invalid string format \'" << str << "<" << *ch << ">\'\n";
  }

  return ch;
}

std::string::iterator &wuscan::str2token(std::string::iterator &ch,
                                         std::string::iterator &end,
                                         valToken &tok, wuscan::numMode mode)
{
  tok.reset();

  // first skip whitespace:
  // Note: it is safe to ignore new-lines, the caller will have already stripped
  // them from the input lines. So the end of string is equivalent to EOL.
  while (ch != end && isspace(*ch)) ++ch;

  if (ch == end) {
    tok.init("EOL",                   // token string (string)
             valToken::tokInternal,   // token category (token_cat)
             valToken::tokEOLType,    // token type (token_type)
             valToken::tokBuiltInFmt, // token format (token_format)
             1);                      // number of values token represents (e.g. an array)
    return ch;
  }
  // Precedence order:
  //   0) EOE
  //   1) Number
  //   2) Byte Array
  //   3) Name
  //   4) Macro
  //   5) String
  //   Expression
  //   Block
  if (*ch == ';') {
    ++ch;
    // End-Of-Expression or EOE
    tok.init(std::string(";"),
             valToken::tokTerminal,
             valToken::tokExprEndType,
             valToken::tokBuiltInFmt);
    return ch;
  }

  if (mode == wuscan::numberDefault) {
    if (wuscan::isNum(ch, end, mode))
      return wuscan::str2Number(ch, end, tok, mode);
    if (wuscan::isDw1Array(ch, end))
      return wuscan::str2NumArray(ch, end, tok, mode);
  } else {
    if (wuscan::isDw1Array(ch, end))
      return wuscan::str2NumArray(ch, end, tok, mode);
    if (wuscan::isNum(ch, end, mode))
      return wuscan::str2Number(ch, end, tok, mode);
  }
  if (wuscan::isName(ch, end))
    return wuscan::str2Name(ch, end, tok);
  if (wuscan::isMacro(ch, end))
    return wuscan::str2Macro(ch, end, tok);
  if (wuscan::isString(ch, end))
    return wuscan::str2String(ch, end, tok);
  if (wuscan::isExpr(ch, end))
    return wuscan::str2Expr(ch, end, tok);
  if (wuscan::isIndex(ch, end))
    return wuscan::str2Index(ch, end, tok);
  if (wuscan::isBlock(ch, end))
    return wuscan::str2Block(ch, end, tok);
  if (wuscan::isOp(ch, end))
    return wuscan::str2Op(ch, end, tok);

  return ch;
}

valToken wuscan::getToken(const std::string& arg, wuscan::numMode mode)
{
  std::string str = arg;
  std::string::iterator iter = str.begin();
  std::string::iterator end = str.end();
  return getToken(iter, end, mode);
}

valToken wuscan::getToken(std::string::iterator &ch, std::string::iterator &end,
                           wuscan::numMode mode)
{
  valToken tok;
  // assume comments have already been removed
  str2token(ch, end, tok, mode);
  return tok;
}

std::string &wuscan::remComments(std::string &line)
{
  std::string::size_type cmt = line.find_first_of("#", 0);
  if (cmt != std::string::npos)
    line.erase(cmt, std::string::npos);
  return line;
}

// getTokens will not modify the char string passed as an argument
int wuscan::getTokens(std::vector<valToken> &tokens,
                       const std::string &line,
                       wuscan::numMode mode)
{
  std::string sbuf = line;

  if (remComments(sbuf).empty())
    return 0;

  std::string::iterator  ch = sbuf.begin();
  std::string::iterator end = sbuf.end();

  valToken tok;
  while (str2token(ch, end, tok, mode) != end) {
    if (!tok) {
      std::cerr << "Invalid token in line (ch = "
                << *ch << ") <" << line
                << ">\n\tTok: " << tok << std::endl;
      break;
    } else
      tokens.push_back(tok);
  }
  if (tok.valid())
    tokens.push_back(tok);
  return tokens.size();
}
