/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/valTypes.c,v $
 * $Author: fredk $
 * $Date: 2007/08/01 15:28:42 $
 * $Revision: 1.16 $
 * $Name:  $
 *
 * File:   valTypes.c
 * Author: Fred Kuhns
 * Email:  fredk@cse.wustl.edu
 * Organization: Washington University in St. Louis
 * 
 * Created:  10/04/06 11:13:09 CDT
 * 
 * Description:
 */

#include <wulib/kern.h>
#include <wulib/keywords.h>
#include <wulib/valTypes.h>

static KeyInfo_t valTypeDesc[] = {
  {(wuKeyFlag_t)invType,  "Invalid", "Used for value objects not yet assigned a value"},
  {(wuKeyFlag_t)strType,  "str",     "Null terminated character string (i.e. c-style)"},
  {(wuKeyFlag_t)charType, "char",    "Single (8-bit) character, elements of a string"},
  {(wuKeyFlag_t)dblType,  "dbl",     "Double precision float, native format"},
  {(wuKeyFlag_t)intType,  "int",     "Signed integer, natural int for platform"},
  {(wuKeyFlag_t)dw8Type,  "dw8",     "Unsigned 8-Byte integer, same as uint64_t"},
  {(wuKeyFlag_t)dw4Type,  "dw4",     "Unsigned 4-Byte integer, same as uint32_t"},
  {(wuKeyFlag_t)dw2Type,  "dw2",     "Unsigned 2-Byte integer, same as uint16_t"},
  {(wuKeyFlag_t)dw1Type,  "dw1",     "Unsigned 1-Byte integer, same as uint8_t"},
  {WULIB_FLAG_INVALID, NULL, NULL},
};

void val_print_vtypes(const char *pre)
{
  KeyInfo_t *ptr = &valTypeDesc[1];
  for (; ptr->name; ++ptr) {
    if (pre)
      printf("%s", pre);
    printf("%s : %s\n", ptr->name, ptr->desc);
  }
  return;
}

const char *val_type2str_safe(valType_t vt)
{
  return wukey_flag2str_safe(vt, valTypeDesc);
}

const char *val_type2str(valType_t vt)
{
  return wukey_flag2str(vt, valTypeDesc);
}

// Number of chars we want the display of array values to consume
// This excludes the opening and closing '{'
static size_t valDispCharCnt = 96;
size_t set_val_dispCnt(size_t sz)
{
  size_t tmp = valDispCharCnt;
  valDispCharCnt = sz;
  return tmp;
}

void val_print_data(const void *data, valType_t vt, size_t cnt)
{
  size_t indx, valCnt, valSize = val_type2size(vt);

  // to display each value we need:
  //   dw1: <XX > =                  3 = sizeof()*2 + " "
  //   dw2: <0xXXXX > =              7 = sizeof()*2 + " " + "0x"
  //   dw4: <0xXXXXXXXX > =         11 = sizeof()*2 + " " + "0x"
  //   dw8: <0xXXXXXXXXXXXXXXXX > = 19 = sizeof()*2 + " " + "0x"
  //   char <X > = 2
  //   string NA
  //   double NA (for now)

  if (data == NULL || cnt == 0)
    return;

  if (valSize == 0 || vt == invType) {
    printf("Invalid Type\n");
    return;
  }

  switch (vt) {
    case strType:
      valSize = 1;
      // simple, perhaps too simple
      printf("\"%s\"\n", (const char *)data);
      return;
      break;
    case dw1Type:
      valSize = valSize * 2 + 1;
      break;
    default:
      valSize = valSize * 2 + 3;
      break;
  }
  valCnt = valDispCharCnt / valSize;
  printf("{ ");
  for (indx = 0; indx < cnt; ++indx) {
    if (indx && (indx % valCnt) == 0)
      printf("\n  ");
    switch(vt) {
      case dw1Type:
        printf("%02" PRIx8 " ",   *((dw1_t *)data + indx));
        break;
      case dw2Type:
        printf("0x%04" PRIx16 " ", *((dw2_t *)data + indx));
        break;
      case dw4Type:
        printf("0x%08" PRIx32 " ", *((dw4_t *)data + indx));
        break;
      case dw8Type:
        printf("0x%016" PRIx64 " ", *((dw8_t *)data + indx));
        break;
      case intType:
        printf("%" PRId32 " ", *((int_t *)data + indx));
        break;
      case charType:
        printf("%c ", *((char_t *)data + indx));
        break;
      case dblType:
        printf("%f ", *((dbl_t *)data + indx));
        break;
      default:
        printf("XX ");
        return;
    }
  }
  printf("}\n");
  return;
}

void *valFromLong(void *data, unsigned long val, valType_t vt)
{
  if (data == NULL)
    return NULL;

  switch(vt) {
    case strType:
    case charType:
      *(char_t *)data = (char_t)val;
      break;
    case dblType:
      *(dbl_t *)data = (dbl_t)val;
      break;
    case intType:
      *(int_t *)data = (int_t)val;
      break;
    case dw8Type:
      *(dw8_t *)data = (dw8_t)val;
      break;
    case dw4Type:
      *(dw4_t *)data = (dw4_t)val;
      break;
    case dw2Type:
      *(dw2_t *)data = (dw2_t)val;
      break;
    case dw1Type:
      *(dw1_t *)data = (dw1_t)val;
      break;
    default:
#ifndef __KERNEL__
      wulog(wulogLib, wulogError, "valFromLong: Unrecognized type %d\n", (int) vt);
#endif
      return NULL;
  }
  return data;
}

void *valset(void *data, const void *val, valType_t vt)
{
  if (data == NULL || val == NULL)
    return NULL;
  switch(vt) {
    case strType:
    case charType:
      *(char_t *)data = *(char_t *)val;
      break;
    case dblType:
      *(dbl_t *)data = *(dbl_t *)val;
      break;
    case intType:
      *(int_t *)data = *(int_t *)val;
      break;
    case dw8Type:
      *(dw8_t *)data = *(dw8_t *)val;
      break;
    case dw4Type:
      *(dw4_t *)data = *(dw4_t *)val;
      break;
    case dw2Type:
      *(dw2_t *)data = *(dw2_t *)val;
      break;
    case dw1Type:
      *(dw1_t *)data = *(dw1_t *)val;
      break;
    default:
#ifndef __KERNEL__
      wulog(wulogLib, wulogError, "valset: Unrecognized type %d\n", (int) vt);
#endif
      return NULL;
  }
  return data;
}

void *valset_ntoh(void *data, const void *val, valType_t vt)
{
  if (data == NULL || val == NULL)
    return NULL;
  switch(vt) {
    case strType:
    case charType:
      *(char_t *)data = *(char_t *)val;
      break;
    case dblType:
      *(dbl_t *)data = (dbl_t)ntohll((int64_t)*(uint64_t *)val);
      break;
    case intType:
      *(int_t *)data = (int_t)ntohl((long)*(int_t *)val);
      break;
    case dw8Type:
      *(uint64_t *)data = (uint64_t)ntohll((int64_t)*(uint64_t *)val);
      break;
    case dw4Type:
      *(dw4_t *)data = (dw4_t)ntohl((long)*(int_t *)val);
      break;
    case dw2Type:
      *(dw2_t *)data = (dw2_t)ntohs((short)*(dw2_t *)val);
      break;
    case dw1Type:
      *(dw1_t *)data = *(dw1_t *)val;
      break;
    default:
#ifndef __KERNEL__
      wulog(wulogLib, wulogError, "valset: Unrecognized type %d\n", (int) vt);
#endif
      return NULL;
  }
  return data;
}

void *valset_hton(void *data, const void *val, valType_t vt)
{
  if (data == NULL || val == NULL)
    return NULL;
  switch(vt) {
    case strType:
    case charType:
      *(char_t *)data = *(char_t *)val;
      break;
    case dblType:
      *(dbl_t *)data = (dbl_t)htonll((int64_t)*(uint64_t *)val);
      break;
    case intType:
      *(int_t *)data = (int_t)htonl((long)*(int_t *)val);
      break;
    case dw8Type:
      *(uint64_t *)data = (uint64_t)htonll((int64_t)*(uint64_t *)val);
      break;
    case dw4Type:
      *(dw4_t *)data = (dw4_t)htonl((long)*(int_t *)val);
      break;
    case dw2Type:
      *(dw2_t *)data = (dw2_t)htons((short)*(dw2_t *)val);
      break;
    case dw1Type:
      *(dw1_t *)data = *(dw1_t *)val;
      break;
    default:
#ifndef __KERNEL__
      wulog(wulogLib, wulogError, "valset: Unrecognized type %d\n", (int) vt);
#endif
      return NULL;
  }
  return data;
}

void *val_memset(void *data, const void *val, valType_t vt, size_t cnt)
{
  void *ptr = data;
  if (data == NULL || val == NULL)
    return NULL;

  while (cnt--)
    valset(ptr++, val, vt);

  return data;
}

void *val_memcpy(void *to, const void *val, valType_t vt, size_t cnt)
{
  size_t blen = val_type2size(vt) * cnt;
  if (to == NULL || val == NULL) return to;
  return memcpy(to, val, blen);
}

valType_t val_str2type(const char *str)
{
  wuKeyFlag_t flag = str2flag(str, valTypeDesc);
  if (flag == WULIB_FLAG_INVALID)
    return invType;
  return (valType_t)flag;
}

