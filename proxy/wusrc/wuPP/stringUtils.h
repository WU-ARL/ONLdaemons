/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/stringUtils.h,v $
 * $Author: fredk $
 * $Date: 2006/04/17 22:56:55 $
 * $Revision: 1.1 $
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

#ifndef _STRINGUTILS_H
#define _STRINGUTILS_H

// Utility Function
int string2flds(std::vector<std::string> &flds,
                const std::string &rec,
                const std::string& fs,
                const std::string& ws=" \t\n\r");

#endif /* _STRINGUTILS_H */
