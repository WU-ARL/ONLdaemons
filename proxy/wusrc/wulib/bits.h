/* 
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/bits.h,v $
 * $Author: fredk $
 * $Date: 2007/11/06 21:17:02 $
 * $Revision: 1.15 $
 * $Name:  $
 *
 * File:         
 * Author:       Fred Kuhns <fredk@arl.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 * Description:
 *
 *
 */

#ifndef _WUBITS_H
#define _WUBITS_H
#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdlib.h>

#ifndef WUPARAM
#define WUPARAM
typedef unsigned long wuparam_t;
static inline wuparam_t wuceil(wuparam_t lha, wuparam_t rha);
static inline wuparam_t wubits2size(size_t bits);
static inline wuparam_t wubits2mask(size_t bits);
static inline wuparam_t wualignup(wuparam_t val, size_t bits);

static inline wuparam_t wuceil(wuparam_t lha, wuparam_t rha)
{
  if (rha == 0) return (wuparam_t)-1; // return all 1's
    return (lha + rha - 1) / rha;
}

static inline wuparam_t wubits2size(size_t bits)
{
  return (wuparam_t)1 << bits;
}

static inline wuparam_t wubits2mask(size_t bits)
{
  return wubits2size(bits)-1;
}

static inline wuparam_t wualignup(wuparam_t val, size_t bits)
{
  wuparam_t mask = wubits2mask(bits);
  wuparam_t tmp = val & ~mask;

  if (val & mask)
    tmp += wubits2size(bits);

  return tmp;
}
#endif

typedef uint32_t wumask_t;

/*
 *  XXX_BITS  = define the field's bit width
 *  XXX_SHIFT = 0 for teh first bit field
 *  XXX_MASK  = (1 << XXX_BITS) - 1
 * An example set of bit fields for a 16-bit word
 *    F  F  F  E  E  E  D  D  C  C  C  C  C  B  A  A
 *   -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
 *   15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 *
 *   A_BITS  = 2
 *   A_SHIFT = 0
 *   A_MASK  = ((1 << A_BITS) - 1) << A_SHIFT
 *   B_BITS  = 1
 *   B_SHIFT = A_SHIFT + A_BITS
 *   B_MASK  = ((1 << B_BITS) - 1) << B_SHIFT
 *   C_BITS  = 5
 *   C_SHIFT = B_SHIFT + B_BITS
 *   C_MASK  = ((1 << C_BITS) - 1) << C_SHIFT
 *   D_BITS  = 2
 *   D_SHIFT = C_SHIFT + C_BITS
 *   D_MASK  = ((1 << D_BITS) - 1) << D_SHIFT
 *   E_BITS  = 3
 *   E_SHIFT = D_SHIFT + D_BITS
 *   E_MASK  = ((1 << E_BITS) - 1) << E_SHIFT
 *   F_BITS  = 3
 *   F_SHIFT = E_SHIFT + E_BITS
 *   F_MASK  = ((1 << F_BITS) - 1) << F_SHIFT
 *
 * You should notice a pattern in the above assignemnts.  So we could define
 * some simplifying macros:
 *  #define wubit_mkMask(b,s)  (((0x1U << (b)) - 0x1U) << (s))
 *
 *   A_BITS  = 2
 *   A_SHIFT = 0
 *   A_MASK  = wubit_mkMask(A_BITS, A_SHIFT)
 *   B_BITS  = 1
 *   B_SHIFT = A_SHIFT + A_BITS
 *   B_MASK  = wubit_mkMask(B_BITS, B_SHIFT)
 *   C_BITS  = 5
 *   C_SHIFT = B_SHIFT + B_BITS
 *   C_MASK  = wubit_mkMask(C_BITS, C_SHIFT)
 *   D_BITS  = 2
 *   D_SHIFT = C_SHIFT + C_BITS
 *   D_MASK  = wubit_mkMask(D_BITS, D_SHIFT)
 *   E_BITS  = 3
 *   E_SHIFT = D_SHIFT + D_BITS
 *   E_MASK  = wubit_mkMask(E_BITS, E_SHIFT)
 *   F_BITS  = 3
 *   F_SHIFT = E_SHIFT + E_BITS
 *   F_MASK  = wubit_mkMask(F_BITS, F_SHIFT)
 *
 * Now if we what the
 * 2-bit D field to have the binary value 101 (decimal 4) then
 * 
 *   flags = flags | (0x04 << D_SHIFT)
 *
 * Alternatively if we want to know if the 3-bit E field has the hex value
 * 0x3 then
 *
 *   ((flags & E_MASK) >> E_SHIFT) == 0x03
 *
 * Or, equivalently
 *   ((flags & E_MASK) == (0x03 << E_SHIFT)
 *
 * So adding another simplifying macro:
 *   #define wumask_get(flags,mask,shift) (((wumask_t)(flags)  & ((wumask_t)(mask)))  >> (shift))
 *
 * Now checking a specific filed is simply
 *   wumask_get(flags, E_MASK, E_SHIFT) == 0x03
 *
 * Setting the bits of a field is a bit more involved,
 *   flags = (flags & ~E_MASK) | ((0x03 << E_SHIFT) & E_MASK)
 * 
 * This can also be coded as a macro.
 *
 * None of these macro are particular hard they just help to reduce coding
 * errors and ease maintenance.
 * */

// w = data word, s = shift (left), m = mask, b = bit field
#define wubit_mkMask(b,s)                (((0x1U << (b)) - 0x1U) << (s))
#define wubit_getBits(w,m)               ((w) & (m))
// all bits must be on otherwise returns 0
#define wubit_allBitsOn(w,b)             (wubit_getBits((w),(b)) == (b))
// at least one bit must be on
#define wubit_anyBitOn(w,b)              (wubit_getBits((w),(b)))
// if any bit is on then wubit_bitsoff() will != 0
#define wubit_allBitsOff(w,b)            (wubit_getBits((w),(m)) == 0x0U)

#define wubit_turnon(w,b)                (((wumask_t)(w)) | (wumask_t)(b))
#define wubit_turnoff(w,b)               (((wumask_t)(w)) & ~(b))
#define wubiton(w,b)                     ((wumask_t)(w) & (wumask_t)(b))
#define wubitoff(w,b)                    ((wumask_t)(b) & ~((wumask_t)(w)))

/* Some bit manipulation macros */
#define wuflag2field(flag,shift)  ((wumask_t)(flag) << (shift))

#define wumask_get_field(flags,width,shift)   (((wumask_t)(flags)  & wubit_mkMask((width),(shift))) >> (shift))
#define wumask_get(flags,mask,shift)          (((wumask_t)(flags)  & ((wumask_t)(mask)))  >> (shift))
#define wumask_shift(flags,mask,shift)        (((wumask_t)(flags) << (shift))  & ((wumask_t)(mask)))
#define wumask_set(flags,flag,mask,shift)     (((wumask_t)(flags)  & ~((wumask_t)(mask)))  | wumask_shift((flag),(mask),(shift)))

#define wuturnoff_bit(flags, bit)  (flags = wubit_turnoff((flags), (bit)))
#define wuturnon_bit(flags, bit)   (flags = wubit_turnon((flags), (bit)))

#ifdef __cplusplus
}
#endif
#endif
