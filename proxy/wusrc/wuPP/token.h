/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/token.h,v $
 * $Author: fredk $
 * $Date: 2007/07/31 16:26:52 $
 * $Revision: 1.12 $
 * $Name:  $
 *
 * File:         token.h
 * Created:      Thu Sep 23 2004 12:14:28 CDT
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#ifndef _VALTOKEN_H
#define _VALTOKEN_H

class valToken {
  public:
    enum token_cat   {tokInvCat,
                      tokNonTerminal,
                      tokTerminal,
                      tokInternal
                      };
    enum token_type  {tokInvType,     // Invalid
                  // Start of Terminal types (immediates, Symbols, Keywords)
                      tokTermStartTypes,
                      tokMacroType,   // Terminal, Variable
                      tokNameType,    // Terminal, Keyword/Command
                      tokStringType,  // Terminal, Immediate "xxxx"
                  // ***** Start of Numeric and Integer types *****
                      tokNumericSTART,
                      tokIntegerSTART = tokNumericSTART,
                      tokIntType,     // Terminal, Immediate, natural int, int 
                      tokDw8Type,     // Terminal, Immediate 8-Byte, uint64_t
                      tokDw4Type,     // Terminal, Immediate 4-Byte, uint32_t
                      tokDw2Type,     // Terminal, Immediate 2-Byte, uint16_t
                      tokDw1Type,     // Terminal, Immediate 1-Byte, uint8_t
                      tokIntegerEND,
                      tokDblType,     // Terminal, Immediate, natural double, Double
                      tokNumericEND,
                      tokTermEndType,
                  // ***** Start of Operators *****
                      tokOpSTART,
                      // Generic Operator
                      tokOpType,
                      // Arithmetic Operators
                      tokAssignType, // assignment
                      tokAddType,    // addition
                      tokSubType,    // subtraction
                      tokMultType,   // multiplication
                      tokDivType,    // division
                      tokModType,    // modulo
                      // Bit Shift operators
                      tokLShiftType,  // shift left
                      tokRShifttType, // shift right
                      // Logical Operators
                      tokNotType,    // logical: '!' not
                      tokLtType,     // logical: '<' less than
                      tokLtEqType,   // logical: '<=' less or equal
                      tokGtType,     // logical: '>' greater than
                      tokGtEqType,   // logical: '>=' greater or equal 
                      tokEqType,     // logical: '==' equal
                      tokNotEqType,  // logical: '!=' not equal
                      tokAndType,    // Logical: '&&' and
                      tokOrType,     // Logical: '||' and
                      // Bit level operators
                      tokBitNotType, // Bit: '~'
                      tokBitAndType, // Bit: '&'
                      tokBitXorType, // Bit: '^'
                      tokBitOrType,  // Bit: '|'
                      tokOpEND,
                  // *****  Subexpression, Blocks and Indexing
                      // Subexpression '(' expression ')'
                      tokSubStartType,
                      tokSubEndType,
                      // Code Block: '{' block '}'
                      tokBlockStartType,
                      tokBlockEndType,
                      // Array index: '[' expr ']'
                      tokIndxStartType,
                      tokIndxEndType,
                 // Everything else: all single character punctuation not an operator
                      tokPunctType,
                      // <EOL> := '\n'
                      tokEOLType,
                      // ';'
                      tokExprEndType,
                      // ^D
                      tokEOFType,
                      };
    enum token_format{tokInvFormat,
                      tokStringFmt,
                      tokBase10Fmt,
                      tokBase16Fmt,
                      tokBuiltInFmt
                      };

    typedef size_t size_type;

    valToken(const valToken& vt)
      : str_(vt.str_), cat_(vt.cat_), type_(vt.type_), fmat_(vt.fmat_), cnt_(vt.cnt_) {}
    valToken() : cat_(tokInvCat), type_(tokInvType), fmat_(tokInvFormat), cnt_(0) {}
    valToken(const std::string str, token_cat cat, token_type type, token_format fmat, size_type cnt)
      : str_(str), cat_(cat), type_(type), fmat_(fmat), cnt_(cnt)
      {}
    virtual ~valToken() {}

    valToken &operator=(const valToken& rhs)
    {
      if (this == &rhs)
        return *this;
      str_  = rhs.str_;
      cat_  = rhs.cat_;
      type_ = rhs.type_;
      fmat_ = rhs.fmat_;
      cnt_  = rhs.cnt_;

      return *this;
    }

    // Predicates
    bool    isIntegral() const {return (type_ > tokIntegerSTART && type_ < tokIntegerEND);}
    bool     isNumeric() const {return (type_ > tokNumericSTART && type_ < tokNumericEND);}
    bool     isDouble()  const {return (type_ == tokDblType); }
    bool      isString() const {return type_ == tokStringType;}
    bool         valid() const {return type_ != tokInvType;}
    bool isNonTerminal() const {return cat_ == tokNonTerminal;}
    bool    isTerminal() const {return cat_ == tokTerminal;}
    bool    isOperator() const {return (type_ > tokOpSTART && type_ < tokOpEND);}
    bool     operator!() const {return !valid();}

    const std::string &str() const {return str_;}
    token_cat          cat() const {return cat_;}
    token_type        type() const {return type_;}
    token_format      fmat() const {return fmat_;}
    size_type          cnt() const {return cnt_;}

    void reset()
      {str_.clear(); cat_ = tokInvCat; type_ = tokInvType; fmat_ = tokInvFormat; cnt_ = 0;}
    void clear() {reset();}
    valToken &init(const std::string &s, token_cat c, token_type t, token_format f, size_t l = 0)
      {str_ = s; cat_ = c; type_ = t; fmat_ = f; cnt_ = l; return *this;};

    std::string toString() const;

  private:
    std::string   str_;
    token_cat     cat_;
    token_type   type_;
    token_format fmat_;
    size_type     cnt_;
};


std::ostream &operator<<(std::ostream &s, const valToken::token_type &t);
std::ostream &operator<<(std::ostream &s, const valToken::token_format &f);
std::ostream &operator<<(std::ostream &s, const valToken::token_cat &c);
std::ostream &operator<<(std::ostream &s, const valToken &vt);

#endif
