/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/conf.cc,v $
 * $Author: fredk $
 * $Date: 2008/02/01 21:06:34 $
 * $Revision: 1.12 $
 * $Name:  $
 *
 * File:   main.cc
 * Author: Fred Kuhns
 * Email:  fredk@cse.wustl.edu
 * Organization: Washington University in St. Louis
 * 
 * Created:  02/14/06 17:36:11 CST
 * 
 * Description:  
 */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <exception>
#include <stdexcept>
#include <stdlib.h>


#include <inttypes.h>
#include <wulib/keywords.h>
#include <wuPP/token.h>
#include <wuPP/scanner.h>
#include <wuPP/conf.h>
#include <wuPP/stringUtils.h>

namespace wuconf {
  // ************************************************************************
  confSection get_section(wuscan::scanStream &ss);
  confSection &get_key_vals(confSection &section, wuscan::scanStream &ss);
  confVal &confGetVal(const char *str, confVal &cval, wuscan::numMode mode); 

  // ************************************************************************
  //                 class confVal definitions
  // ************************************************************************
  confVal &confVal::init(const valToken &tok)
  {
    reset();
    type_ = tok.type();
    str_  = tok.str();
    cnt_  = tok.cnt();
    if (cnt_ == 0 || ! tok )
      return *this;

    if (tok.isNumeric() && cnt_ > 0) {
      int tbase = (tok.fmat() == valToken::tokBase16Fmt) ? 16 : 10;
      errno = 0;
      switch (type_) {
        case valToken::tokIntType:
          if (cnt_ == 1)
            val.lval = (long)strtol(str_.c_str(), NULL, tbase);
          else
            wulog(wulogLib, wulogError, "wuconf::init: No support for int arrays\n");
          break;
        case valToken::tokDw8Type:
          if (cnt_ == 1)
            val.llval = (long long)strtoull(str_.c_str(), NULL, tbase);
          else
            wulog(wulogLib, wulogError, "tok2Value: No support for arrays\n");
          break;
        case valToken::tokDw4Type:
          if (cnt_ == 1)
            val.lval = (long)strtoul(str_.c_str(), NULL, tbase);
          else
            wulog(wulogLib, wulogError, "tok2Value: No support for arrays\n");
          break;
        case valToken::tokDw2Type:
          if (cnt_ == 1)
            val.lval = (long)strtoul(str_.c_str(), NULL, tbase);
          else
            wulog(wulogLib, wulogError, "tok2Value: No support for arrays\n");
          break;
        case valToken::tokDw1Type:
          // Arrays are only supported (in the tokenizer) for bytes
          if (cnt_ == 1) {
            val.lval = (long)strtoul(str_.c_str(), NULL, tbase);
          } else {
            std::vector<std::string> nums;
            string2flds(nums, tok.str(), ".:");
            if (nums.size() < cnt_)
              throw std::runtime_error("confVal: dw1 Array, fewer numbers that expected");
          
            size_t dlen = sizeof(dw1_t) * cnt_;
            dw1_t *data = (dw1_t *)malloc(dlen);
            if (data == NULL)
              throw std::runtime_error("confVal: Error allocating memory for dw1 array");
            for (size_t indx = 0; indx < cnt_; ++indx)
              data[indx] = (dw1_t)strtoul(nums[indx].c_str(), NULL, tbase);
            data_ = (void *)data;
            dlen_ = dlen;
            val.dw1_vec = data;
          }
          break;
        case valToken::tokDblType:
          if (cnt_ == 1)
            val.dval = strtod(str_.c_str(), NULL);
          else
            wulog(wulogLib, wulogError, "wuconf::init: No support for arrays\n");
          break;
        default:
          wulog(wulogLib, wulogError, "tok2Value: don\'t know what to do with type %s.\n",
                str_.c_str());
      } // switch (type_)
    } // Numeric value
    return *this;
  }

  confVal& confVal::reset()
  {
    type_ = valToken::tokInvType;
    str_.clear();
    cnt_  = 0;
    if (data_)
      free(data_);
    dlen_ = 0;
    data_ = 0;
    memset((void *)&val, 0, sizeof(val));
    return *this;
  }

  confVal &confVal::operator=(const confVal &rhs)
  {
    if (this == &rhs)
      return *this;
    return copy(rhs);
  }

  confVal &confVal::copy(const confVal &rhs)
  {
    type_ = rhs.type_;
    str_  = rhs.str_;
    cnt_  = rhs.cnt_;
    // if cnt_ > 0 then an array and the data_ pointer is used
    // otherwise the value is stored directly in the union struct.
    if (rhs.cnt_ == 0) {
      reset();
    } else if (rhs.cnt_ == 1) {
      // if we have allocated memory then free it
      if (data_) {
        free(data_);
        dlen_ = 0;
      }
      memcpy((void *)&val, (void *)&rhs.val, sizeof(val));
    } else {
      if (rhs.dlen_ > dlen_) {
        if (data_)
          free(data_);
        data_ = (void *)malloc(rhs.dlen_);
        dlen_ = rhs.dlen_;
      }
      memcpy(data_, rhs.data_, rhs.dlen_);
      val.dw1_vec = (dw1_t *)data_;
    }
    return *this;
  }

  // ************************************************************************
  //     Functions for parsing command line options (strict or permissive)
  // ************************************************************************
  valTable &mkparams(int *argc,
                     char **argv,
                     valTable &vtbl,
                     const KeyInfo_t *opts,
                     wuscan::numMode mode)
  {
    // strict version, you must supply a list of the valid command line
    // arguments.  An exception is thrown if a command line argument does
    // not appear in the list of valid args (opts). The KeyInfo_t* argument
    // has the following format:
    //   const KeyInfo_t opts[] = {
    //   { has_value, name, description}, ..., {0, NULL, NULL}}
    //   The first field 'has_value' is set to 1 if the argument has a value
    //   otherwise set to 0. For example, the argument --fname takes a value
    //   while the argument --verbose does not
    //     cmd --fname myfile.ini --verbose
    //    opts = {
    //    ...
    //    {1, "--fname", "name of configuration file"},
    //    {0, "--verbose", "print verbose debugging messages"},
    //    ...
    //   The final entry of the opts array must be the special NULl value
    //   {0, NULL, NULL} to mark the end.
    //   Finally, if the has_value field is set to -1 (WULIB_FLAG_INVALID)
    //   then the argument is ignored and will be kept in the argument list.
    int indx, cnt, start;
    confVal val;
    char *arg;
    std::vector<char *> xargv1, xargv2;

    // xargv1 is the list of arguments that will be preserved for the caller
    // xargv2 is the list of valid arguments which will be removed from the
    // input argv vector.
    xargv1.push_back(argv[0]);

    if (opts == NULL || argc == NULL || *argc <= 1) return vtbl;

    for (indx = 1; indx < *argc; ++indx) {
      arg = argv[indx];
      cnt = wukey_str2flag(arg, opts);
      if (cnt == (int)WULIB_FLAG_INVALID) {
        // not recognized flag, keep
        xargv1.push_back(argv[indx]);
        // std::cerr << "Unrecognized argument <" << arg << ">, ignoring\n";
        continue;
      }
      xargv2.push_back(argv[indx]);
      start = (arg[1] == '-') ? 2 : 1;
      if (cnt > 0) {
        // get argumet
        indx += 1;
        xargv2.push_back(argv[indx]);
        if (argv[indx] && argv[indx][0] != '-') {
          confGetVal(argv[indx], val, mode);
        } else {
          std::cerr << "Missing value for argument " << arg << "\n\t" << opts->desc << "\n";
          throw std::runtime_error("Missing argument");
        }
      } else {
        // cnt == 0
        val.reset();
      }
      vtbl[&arg[start]] = val;
    }

    std::vector<char *>::iterator iter;
    for (iter = xargv1.begin(); iter != xargv1.end(); ++iter)
      *argv++ = *iter;
    // put back on the tail of the argv vector in case the caller needs to
    // keep track of the memory.
    for (iter = xargv2.begin(); iter != xargv2.end(); ++iter)
      *argv++ = *iter;
    // set argc to the number of unprocessed arguments.
    *argc = xargv1.size();

    return vtbl;
  }

  valTable &mkparams(int argc,
                     char **argv,
                     valTable &vtbl,
                     wuscan::numMode mode)
  {
    // The permissive version of the argument processor: if an argument is
    // preceeded by a '-' or '--' then it is assumed to be valid and added to
    // the vtbl argument.
    int indx;
    char *arg;
    confVal val;

    for (indx = 1; indx < argc; ++indx) {
      if (argv[indx][0] != '-')
        // done with input arguments
        break;
      arg = (argv[indx][1] == '-') ? argv[indx]+2 : argv[indx]+1;
      if (! argv[indx+1] || argv[indx+1][0] == '-') {
        // no argument value
        val.reset();
      } else {
        ++indx;
        confGetVal(argv[indx], val, mode);
      }
      vtbl[arg] = val;
    }

    return vtbl;
  }

  // ************************************************************************
  confVal &confGetVal(const char *arg, confVal &cval, wuscan::numMode mode)
  {
    valToken tok;

    if (arg == NULL || *arg == '\0')
      // No value
      tok.reset();
    //else if (isalpha(arg[0]) || arg[0] == '_')
    //  // A name
    //  tok.init(arg, valToken::tokTerminal,
    //                valToken::tokStringType,
    //                valToken::tokStringFmt,
    //                strlen(arg)+1);
    else
      tok = wuscan::getToken(std::string(arg), mode);
    return cval.init(tok);
  }

  // ************************************************************************
  confSection get_section(wuscan::scanStream &ss)
  {
    valToken tok = ss.next();
    confSection section;

    // Get section entry: [ name ] \n
    if (tok.type() != valToken::tokIndxStartType) {
      throw std::runtime_error("format error, missing start of section");
    }
    tok = ss.next();
    if (tok.type() != valToken::tokNameType) {
      throw std::runtime_error("format error [ name ]");
    }
    section.first = tok.str();
    if (ss.next().type() != valToken::tokIndxEndType) {
      throw std::runtime_error("Error in section name, missing ]");
    }

    return get_key_vals(section, ss);
  }

  confSection &get_key_vals(confSection &section, wuscan::scanStream &ss)
  {
    valToken tok;

    while ((tok = ss.next()).type() != valToken::tokEOFType) {
      if (tok.type() == valToken::tokEOLType)
        // skip blank lines
        continue;

      if (tok.type() == valToken::tokIndxStartType) {
        // start of new section so return
        ss.push(tok);
        return section;
      }
      // String format: key\s*=[\s*value\s*]
      // (1) get key
      if (tok.type() != valToken::tokNameType)
        throw std::runtime_error("Format error, missing key name in section "
                                + section.first
                                + "\nLine: "
                                + ss.get_state());
      std::string key = tok.str();

      // (2) assignment operator '='
      tok = ss.next();
      if (tok.type() != valToken::tokAssignType)
        throw std::runtime_error("Format error, missing \'=\' symbol in section "
                                + section.first
                                + "\nLine: "
                                + ss.get_state());
      // (3) get value
      tok = ss.next();
      if (tok.type() == valToken::tokEOLType || tok.type() == valToken::tokEOFType)
        section.second[key] = confVal();
      else
        // should fist check to see if this key is already in the list
        section.second[key] = confVal(tok);
    }
    return section;
  }

  // ************************************************************************
  confTable &mkconf(confTable &ctbl,
                    const std::string &fname,
                    const std::string &gname,
                    const valTable &gparams)
  {
    valTable::iterator viter;
    valTable::const_iterator macro;

    get_conf(ctbl, fname, gname);
    confTable::iterator xiter = ctbl.find(gname);
    valTable::const_iterator ro_viter = gparams.begin();
    for (; ro_viter != gparams.end(); ++ro_viter)
      xiter->second[ro_viter->first] = ro_viter->second;

    confTable::iterator citer = ctbl.begin();
    for (; citer != ctbl.end(); ++citer) {
      // citer => pair = {"section_name", valTable}
      // xiter => pair = {gname, valTable}
      // viter => pair = {"param_str", confVal}
      if (citer->first == xiter->first)
        continue;
      for (viter = citer->second.begin();
          viter != citer->second.end();
          ++viter) {
        if (viter->second.type() == valToken::tokMacroType) {
          macro = xiter->second.find(viter->second.str());
          if (macro == xiter->second.end())
            throw std::runtime_error("mkconf: macro error not found.\n\tMacro "
                                    + viter->second.str()
                                    + ", Key = "
                                    + citer->first
                                    + "::"
                                    + viter->first);
          else
            viter->second = macro->second;
        }
      }
    }

    return ctbl;
  }

  void print_conf(const confTable &ctable, std::ostream &os)
  {
    valTable::const_iterator ro_viter;
    confTable::const_iterator ro_citer = ctable.begin();
    for (; ro_citer != ctable.end(); ++ro_citer) {
      ro_viter = (*ro_citer).second.begin();
      os << "Section: " << (*ro_citer).first << "\n";
      for (; ro_viter != (*ro_citer).second.end(); ++ro_viter)
        os << "\t" << ro_viter->first << " = " << ro_viter->second.str()
            << ", Type " << ro_viter->second.type() << "\n";
    }
    return;
  }
  // ************************************************************************
  //        The primary interface for creating symbol/params tables
  // ************************************************************************
  void get_conf(confTable &ctable,
                const std::string &fname,
                const std::string &gname,
                wuscan::numMode mode)
  {
    std::ifstream fin(fname.c_str());
    if (!fin) {
      std::cerr << "Unable to open input file " << fname << std::endl;
      throw std::runtime_error("Unable to open input file ");
    }
    wuscan::scanStream ss(fin, mode, fname);
    valToken tok;

    // First get any global (top-level) definitions
    confSection gsect;
    gsect.first = gname;
    ctable.insert(get_key_vals(gsect, ss));

    while((tok = ss.next()).type() != valToken::tokEOFType) {
      if (tok.type() == valToken::tokIndxStartType) {
        ss.push(tok);
        ctable.insert(get_section(ss));
      } else if (tok.type() == valToken::tokEOLType) {
        continue;
      } else {
        std::cout << "Unexpected token: " << tok.str() << "\n";
      }
    }

    return;
  }

  void print_vtbl(const valTable &vtbl, std::ostream &os, const std::string &pre)
  {
    valTable::const_iterator viter;
    for (viter = vtbl.begin(); viter != vtbl.end(); ++viter)
      os << pre
         << viter->first << " = " << viter->second.str()
         << ", Type " << viter->second.type() << "\n";
  }
};
std::ostream &operator<<(std::ostream &os, const wuconf::confTable &ct)
{
  wuconf::print_conf(ct, os);
  return os;
}
std::ostream &operator<<(std::ostream &os, const wuconf::valTable &vt)
{
  wuconf::print_vtbl(vt, os);
  return os;
}
