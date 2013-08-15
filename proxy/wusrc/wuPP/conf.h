/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/conf.h,v $
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

#ifndef _WUCONF_H
#define _WUCONF_H
#include <map>
#include <wuPP/token.h>
#include <wuPP/scanner.h>
#include <wulib/keywords.h>
#include <wulib/valTypes.h>

namespace wuconf {
  // Represents the value of a configuration parameter
  // Currently only support strings, number and byte arrays.
  class confVal {
    private:
      valToken::token_type type_;
      std::string          str_;
      size_t               cnt_;
      void*               data_;
      size_t              dlen_;
      union {
        long long     llval;
        long           lval;
        dbl_t          dval;
        //dbl_t*         dbl_vec;
        //int_t*         int_vec;
        //chr_t*         chr_vec;
        dw1_t*         dw1_vec;
        //dw2_t*         dw2_vec;
        //dw4_t*         dw4_vec;
        //dw8_t*         dw8_vec;
      } val;

      confVal &copy(const confVal &rhs);
    public:
      confVal()
        : type_(valToken::tokInvType), str_(""), cnt_(0), data_(0), dlen_(0) { reset(); }
      confVal(const valToken &tok)
        : type_(valToken::tokInvType), str_(""), cnt_(0), data_(0), dlen_(0) { init(tok); }
      confVal(const confVal &cval)
        : type_(valToken::tokInvType), str_(""), cnt_(0), data_(0), dlen_(0) { copy(cval); }

      confVal& init(const valToken &tok);
      confVal& reset();

      // I rely on  two's complement representation, i.e. signed and unsigned
      // have the same representation. So a simple cast is sufficient and doesn't
      // result in any code.
      long long     llval() const {return val.llval;}
      long           lval() const {return val.lval;}
      int            ival() const {return (int)val.lval;}
      unsigned long long ullval() const {return (unsigned long long)val.llval;}
      unsigned long ulval() const {return (unsigned long)val.lval;}
      unsigned int  uival() const {return (unsigned int)val.lval;}
      double         dval() const {return val.dval;}

      valToken::token_type type() const {return type_;}
      const std::string    &str() const {return str_;}
      size_t                cnt() const {return cnt_;}

      confVal &operator=(const confVal &rhs);
  };

  // 
  //  Format of a configuration table (i.e. confTable):
  //    confTable : { {string, valTable}, ..., {string, valTable}}
  //   or
  //    {
  //     { section_1  : { {param_1_1, val}, ..., {param_1_m, val}}},
  //     { section_2  : { {param_2_1, val}, ..., {param_2_m, val}}
  //    ...
  //     { section_N  : { {param_N_1, val}, ..., {param_N_m, val}}
  //    }
  // So each element of a confTable is of type confSection {string, valTable}
  // And a valTable is a mapping from a string (token) to value represented by
  // a confVal object. Or in simple terms, a conTable represents a key/value
  // lookup table. The confTable is just a set of lookup tables, one per named
  // section (or namespace).
  typedef std::map<std::string, confVal>       valTable;
  typedef std::multimap<std::string, valTable> confTable;
  typedef std::pair<std::string, valTable>     confSection;

  void get_conf(confTable &ctable,
                const std::string &fname,
                const std::string &gname="global",
                wuscan::numMode mode=wuscan::defNumMode);

  void print_vtbl(const valTable &vtbl, std::ostream &os=std::cout, const std::string &pre="\t");
  void print_conf(const confTable &ctable, std::ostream &os=std::cout);

  // The permissive version of the argument processor: if an argument is
  // preceeded by a '-' or '--' then it is assumed to be valid and added to
  // the vtbl argument.
  valTable &mkparams(int argc, char **argv,
                     valTable &params,
                     wuscan::numMode mode=wuscan::defNumMode);

  // Takes an argument vector (for example argc/argv from the main function)
  // and processes the arguments as key/value pairs used to create a valTable.
  // The expectation is this will be used to define the 'global' key/value
  // pairs which may over-ride section values.
  //
  // Strict version, you must supply a list of the valid command line
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
  valTable &mkparams(int *argc, char **argv,
                     valTable &params,
                     const KeyInfo_t *opts,
                     wuscan::numMode mode=wuscan::defNumMode);


  confTable &mkconf(confTable &ctbl,
                    const std::string &fname,
                    const std::string &gname,
                    const valTable &gparams);
};
  std::ostream &operator<<(std::ostream &os, const wuconf::confTable &ct);
  std::ostream &operator<<(std::ostream &os, const wuconf::valTable &vt);

#endif
