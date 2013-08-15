/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/scanner.h,v $
 * $Author: fredk $
 * $Date: 2007/09/10 16:13:56 $
 * $Revision: 1.7 $
 * $Name:  $
 *
 * File:         scan.h
 * Created:      Thu Sep 23 2004 12:14:28 CDT
 * Author:       Fred Kuhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#ifndef _VALSCANNER_H
#define _VALSCANNER_H

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <wuPP/token.h>
#include <wuPP/error.h>

namespace wuscan {
  enum numMode {arrayDefault, arrayIgnoreFirst, numberDefault};
  const numMode defNumMode = numberDefault;


  inline std::ostream &operator<<(std::ostream& os, numMode mode);
  inline std::ostream &operator<<(std::ostream& os, numMode mode)
  {
    switch (mode) {
      case arrayDefault:
        os << "arrayDefault";
        break;
      case arrayIgnoreFirst:
        os << "arrayIgnoreFirst";
        break;
      case numberDefault:
        os << "numberDefault";
        break;
    }
    return os;
  }

  std::string &remComments(std::string &line);

  valToken getToken(const std::string&, wuscan::numMode=wuscan::defNumMode);
  valToken getToken(std::string::iterator &, std::string::iterator &, wuscan::numMode=wuscan::defNumMode);

  int getTokens(std::vector<valToken> &tokens,
                const std::string &line,
                numMode mode = defNumMode);

  class scanStream {
    private:
      std::istream&                    fin_;
      std::string                     line_;
      size_t                          lnum_;
      numMode                         mode_;
      std::string                    fname_;
      std::string::iterator          lcurr_, lend_, prev_;
      bool ispushed_;
      bool needLine_;
      valToken pushed_tok_;

      std::string &_getline(std::string &line)
      {
        std::getline(fin_, line);
        remComments(line);
        return line;
      }

      void readLine()
      {
        line_.clear();
        if (! fin_.eof() && fin_.good()) {
          ++lnum_;
          _getline(line_);
          if (line_.size() > 0 && line_[line_.size()-1] == '\\') {
            std::string tmp;
            do {
              line_[line_.size()-1] = ' ';
              _getline(tmp);
              line_ += tmp;
            } while (! tmp.empty() && tmp[tmp.size()-1] == '\\');
          }
        }
        lcurr_ = line_.begin();
        lend_  = line_.end();
        prev_  = lend_;
      }


      valToken getTok()
      {
        valToken tok;

        if (needLine_) {
          if (fin_.eof() || ! fin_.good())
            return valToken("EOF", valToken::tokInternal,
                                   valToken::tokEOFType,
                                   valToken::tokBuiltInFmt, 1);
          readLine();
          needLine_ = false;
        }

#if 0
        tok = getToken(lcurr_, lend_, mode_);
        if (tok.type() == valToken::tokEOLType) {
          if (fin_.eof() || ! fin_.good())
            return valToken("EOF", valToken::tokInternal,
                                   valToken::tokEOFType,
                                   valToken::tokBuiltInFmt, 1);
          readLine();
        }
#endif
        tok = getToken(lcurr_, lend_, mode_);
        if (tok.type() == valToken::tokEOLType)
          needLine_ = true;
        return tok;
      }

    public:
      scanStream(std::istream &fin, numMode mode, const std::string fname)
        : fin_(fin), lnum_(0), mode_(mode), fname_(fname), ispushed_(false), needLine_(true)
        { readLine(); needLine_ = false;}
      virtual ~scanStream() {}

      bool operator!() {return line_.empty() && ! ispushed_ && ( fin_.eof() || ! fin_.good()) ;}

      void    setMode(numMode mode) {mode_ = mode;}
      numMode getMode() const {return mode_;}
      const std::string& line() const {return line_;}
      size_t  lnum() const {return lnum_;}

      void clearHist() {prev_ = lend_;}
      void reset() {if (prev_ != lend_) lcurr_ = prev_;}
      valToken next()
      {
        if (ispushed_) {
          ispushed_ = false;
          return pushed_tok_;
        }
        prev_ = lcurr_;
        return getTok();
      }
      void push(valToken &tok) {pushed_tok_ = tok; ispushed_ = true;}

      void clear()
      {
        line_.clear(); prev_ = lcurr_ = lend_ = line_.end();
      }
      bool eof()
      {
        return (fin_.eof() || ! fin_.good());
      }
      std::string get_state()
      {
        std::ostringstream ss;
        ss << "(" << lnum_ << "): " << line_;
        return ss.str();
      }

      void report_stream()
      {
        std::cout << lnum_ << ") Line: " << line_ << std::endl;
        return;
      }
  };
}

#endif /* _SCANNER_H */
