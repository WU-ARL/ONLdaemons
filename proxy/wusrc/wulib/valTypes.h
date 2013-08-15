/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/valTypes.h,v $
 * $Author: fredk $
 * $Date: 2007/03/06 21:30:14 $
 * $Revision: 1.18 $
 * $Name:  $
 *
 * File:   valTypes.h
 * Author: Fred Kuhns
 * Email:  fredk@cse.wustl.edu
 * Organization: Washington University in St. Louis
 * 
 * Created:  09/25/06 12:26:58 CDT
 * 
 * Description:
 */

#ifndef _WUBASICTYPES_H
#define _WUBASICTYPES_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __KERNEL__
#include <features.h>
#include <endian.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifndef __WUTYPES__
#define __WUTYPES__
// Linux kernel pointer type and my generic flags type
typedef unsigned long wuptr_t;
#define WUPTR_INVALID_ADDRESS 0xFFFFFFFF
typedef unsigned int wuflag_t;
#endif

#ifndef __VALTYPES__
#define __VALTYPES__
typedef char     chr_t;
typedef char     char_t;
typedef double   dbl_t;
typedef int32_t  int_t;
typedef uint64_t dw8_t;
typedef uint32_t dw4_t;
typedef uint16_t dw2_t;
typedef uint8_t  dw1_t;

// I want the enumeration values to be representable by an 8-bit unsigned
// integer. Why? Because I wll use this enum value as an 8-bit value in a
// messaging protocol.
#define VALTYPE_MASK         0xFFFFU
typedef enum valType_t {
#define VALTYPE_INVALID           0U
             invType = VALTYPE_INVALID,
             // string types
#define VALTYPE_STRING_TYPES      (VALTYPE_INVALID+1U)
#define VALTYPE_STRING            VALTYPE_STRING_TYPES
               stringTypes = VALTYPE_STRING_TYPES,
               strType     = VALTYPE_STRING, // null terminated char string
#define VALTYPE_CHAR              (VALTYPE_STRING+1U)
               charType = VALTYPE_CHAR,  // 1-Byte chars (not wide chars)
#define VALTYPE_NUMERIC_TYPES     (VALTYPE_CHAR+1U)
#define VALTYPE_FLOAT_TYPES       VALTYPE_NUMERIC_TYPES
#define VALTYPE_DOUBLE            VALTYPE_NUMERIC_TYPES
               numericTypes = VALTYPE_NUMERIC_TYPES,
               floatTypes   = VALTYPE_FLOAT_TYPES,
               dblType      = VALTYPE_DOUBLE,  // the platform's natural double
#define VALTYPE_INTEGRAL_TYPES    (VALTYPE_DOUBLE+1U)
#define VALTYPE_INTEGRAL_SIGNED   VALTYPE_INTEGRAL_TYPES
             // Integral types:
               // Signed Ints
#define VALTYPE_INT               (VALTYPE_INTEGRAL_SIGNED)
               integralTypes = VALTYPE_INTEGRAL_TYPES,
               signedTypes   = VALTYPE_INTEGRAL_SIGNED,
               intType       = VALTYPE_INT,  // the platform's natural integer
#define VALTYPE_INTEGRAL_UNSIGNED (VALTYPE_INT+1U)
               // Unsigned Ints, I assume two's complement representation of
               // signed ints => no bit-level translation between signed and
               // unsigned. So its sufficient for reading/writing memory
               // locations to stick with the unsigned versions of the int
               // types (1,2,4,8-byte int objects).
#define VALTYPE_DW8               VALTYPE_INTEGRAL_UNSIGNED
               unsignedTypes = VALTYPE_INTEGRAL_UNSIGNED,
               dw8Type       = VALTYPE_DW8,  // 8-Byte 'data-word' unsigned int
#define VALTYPE_DW4               (VALTYPE_DW8+1U)
               dw4Type = VALTYPE_DW4,  // 4-Byte 'data-word' unsigned int
#define VALTYPE_DW2               (VALTYPE_DW4+1U)
               dw2Type = VALTYPE_DW2,  // 2-Byte 'data-word' unsigned int
#define VALTYPE_DW1               (VALTYPE_DW2+1U)
               dw1Type = VALTYPE_DW1,  // 1-Byte 'data-word' unsigned int
#define VALTYPE_COUNT             (VALTYPE_DW1+1)
             typeCnt = VALTYPE_COUNT,
#define VALTYPE_MAX               0xFFFFU
             maxType = VALTYPE_MAX
} valType_t;

static inline size_t val_type2size(valType_t vt);
static inline size_t val_type2size(valType_t vt)
{
  size_t result = 0;
  switch(vt) {
    case strType:
      result = sizeof(char);
      break;
    case charType:
      result = sizeof(char);
      break;
    case dblType:
      result = sizeof(double);
      break;
    case intType:
      result = sizeof(int);
      break;
    case dw8Type:
      result = sizeof(uint64_t);
      break;
    case dw4Type:
      result = sizeof(uint32_t);
      break;
    case dw2Type:
      result = sizeof(uint16_t);
      break;
    case dw1Type:
      result = sizeof(uint8_t);
      break;
    default:
      break;
  }
  return result;
}

#ifndef __KERNEL__
  const char *val_type2str_safe(valType_t vt);
  const char *val_type2str(valType_t vt);
  void        val_print_data(const void *data, valType_t vt, size_t cnt);
  size_t      set_val_dispCnt(size_t sz);
  void        val_print_vtypes(const char *pre);

  void *val_memcpy(void *to, const void *val, valType_t vt, size_t cnt);
  void *val_memset(void *data, const void *val, valType_t vt, size_t cnt);
  void *valset(void *data, const void *val, valType_t vt);
  void *valset_ntoh(void *data, const void *val, valType_t vt);
  void *valset_hton(void *data, const void *val, valType_t vt);
  void *valFromLong(void *data, unsigned long val, valType_t vt);
  valType_t   val_str2type(const char *str);
  static inline size_t  val_str2size(const char *str);
  static inline size_t  val_str2size(const char *str)
  {
    return val_type2size(val_str2type(str));
  }
#endif

static inline int val_isValid(valType_t vt);
static inline int val_isNumeric(valType_t vt);
static inline int val_isFloat(valType_t vt);
static inline int val_isIntegral(valType_t vt);
static inline int val_isUnsigned(valType_t vt);
static inline int val_isSigned(valType_t vt);
static inline int64_t htonll(int64_t);
static inline int64_t ntohll(int64_t);

static inline int val_isValid(valType_t vt)    { return (vt > invType        && vt < typeCnt); }
static inline int val_isNumeric(valType_t vt)  { return (vt >= numericTypes  && vt < typeCnt); }
static inline int val_isFloat(valType_t vt)    { return (vt >= floatTypes    && vt < integralTypes); }
static inline int val_isIntegral(valType_t vt) { return (vt >= integralTypes && vt < typeCnt); }
static inline int val_isUnsigned(valType_t vt) { return (vt >= unsignedTypes && vt < typeCnt); }
static inline int val_isSigned(valType_t vt)   { return (vt >= signedTypes   && vt < unsignedTypes); }

#if __BYTE_ORDER == __LITTLE_ENDIAN
static inline int64_t ntohll(int64_t x)
{
  return ((int64_t)ntohl((long)(x >> 32)) & 0xFFFFFFFF) | (((int64_t)ntohl((long)(x & 0xFFFFFFFF))) << 32);
}
static inline int64_t htonll(int64_t x) {return ntohll(x);}
#else
static inline int64_t htonll(int64_t x) {return x;}
static inline int64_t ntohll(int64_t x) {return x;}
#endif

#endif /* __VALTYPES__ */
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _WUBASICTYPES_H */
