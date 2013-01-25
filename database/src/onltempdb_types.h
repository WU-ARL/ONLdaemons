/*
 * Copyright (c) 2009-2013 Charlie Wiseman, Jyoti Parwatikar, John DeHart 
 * and Washington University in St. Louis
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 */

#ifndef _ONLTEMPDB_TYPES_H
#define _ONLTEMPDB_TYPES_H

namespace onl
{
  sql_create_13(requests,1,13,
    mysqlpp::sql_varchar, user,
    mysqlpp::sql_varchar, password,
    mysqlpp::sql_varchar, respadmin,
    mysqlpp::sql_varchar, timezone,
    mysqlpp::sql_varchar, fullname,
    mysqlpp::sql_varchar, email,
    mysqlpp::sql_varchar, position,
    mysqlpp::sql_varchar, institution,
    mysqlpp::sql_varchar, phone,
    mysqlpp::sql_varchar, homepage,
    mysqlpp::sql_varchar, advisor,
    mysqlpp::sql_varchar, advisoremail,
    mysqlpp::sql_varchar, description)

  sql_create_14(rejections,2,14,
    mysqlpp::sql_varchar, user,
    mysqlpp::sql_smallint_unsigned, attempt,
    mysqlpp::sql_varchar, password,
    mysqlpp::sql_varchar, respadmin,
    mysqlpp::sql_varchar, timezone,
    mysqlpp::sql_varchar, fullname,
    mysqlpp::sql_varchar, email,
    mysqlpp::sql_varchar, position,
    mysqlpp::sql_varchar, institution,
    mysqlpp::sql_varchar, phone,
    mysqlpp::sql_varchar, homepage,
    mysqlpp::sql_varchar, advisor,
    mysqlpp::sql_varchar, advisoremail,
    mysqlpp::sql_varchar, description)
};
#endif // _ONLTEMPDB_TYPES_H
