/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/stringUtils.cc,v $
 * $Author: fredk $
 * $Date: 2006/04/17 22:56:55 $
 * $Revision: 1.1 $
 * $Name:  $
 *
 * File:         ctoken.cc
 * Created:      Thu Sep 23 2004 12:14:28 CDT
 * Author:       Fred Kuhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#include <string>
#include <vector>

using std::string;
using std::vector;

#include <wuPP/stringUtils.h>

// returns the number of fields added to the vector.
int string2flds(vector<string> &flds, const string &rec, const string& fs, const string& ws)
{
  string fld;
  string::size_type fend = 0, end  = rec.size();
  size_t cnt = 0;

  for (string::size_type indx = 0; indx <= end; indx = fend + 1) {
    // skip over any initial white space
    indx = rec.find_first_not_of(ws, indx);

    if (indx == string::npos) {
       // ,[ ]+$
       //      ^
       fld = "";
       fend = end;
     } else if (fs.find_first_of(rec[indx]) < string::npos) {
       // (,[ ]*)*,
       //         ^
       fld = "";
       fend = indx;
     } else {
       // ,[ ]*[^ ]+[,]
       //       ^
       fend = rec.find_first_of(fs, indx);
       if (fend == string::npos) {
         fend = end;
       }
       string::size_type tmp = rec.find_last_not_of(ws, fend-1) + 1;
       fld = rec.substr(indx, tmp - indx);
     }
    flds.push_back(fld);
    ++cnt;
  }
  return cnt;
}
