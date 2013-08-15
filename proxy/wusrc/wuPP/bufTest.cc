/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/bufTest.cc,v $
 * $Author: fredk $
 * $Date: 2006/03/20 22:23:16 $
 * $Revision: 1.6 $
 * $Name:  $
 *
 * File:         try.cc
 * Created:      02/18/2005 05:36:09 PM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <stdint.h>

#include <wulib/wulog.h>
#include "net.h"
#include "buf.h"
//#include "msg.h"

#define HCnt 2
#define HLen (HCnt * sizeof(uint32_t))
uint32_t Hdr[HCnt] = {0xdeadbeef, 0xfeedbeef};

#define PCnt 48
#define PLen (PCnt * sizeof(uint32_t))
uint32_t Data[PCnt] = {
  0x00000000, // 0
  0x11111111,
  0x22222222,
  0x33333333,
  0x44444444,
  0x55555555,
  0x66666666,
  0x77777777,
  0x88888888,
  0x99999999,
  0xaaaaaaaa,
  0xbbbbbbbb,
  0xcccccccc,
  0xdddddddd,
  0xeeeeeeee,
  0xffffffff,
  0x00000000, // 16
  0x01010101,
  0x02020202,
  0x03030303,
  0x04040404,
  0x05050505,
  0x06060606,
  0x07070707,
  0x08080808,
  0x09090909,
  0x0a0a0a0a,
  0x0b0b0b0b,
  0x0c0c0c0c,
  0x0d0d0d0d,
  0x0e0e0e0e,
  0x0f0f0f0f,
  0x00000000, // 32
  0x10101010,
  0x20202020,
  0x30303030,
  0x40404040,
  0x50505050,
  0x60606060,
  0x70707070,
  0x80808080,
  0x90909090,
  0xa0a0a0a0,
  0xb0b0b0b0,
  0xc0c0c0c0,
  0xd0d0d0d0,
  0xe0e0e0e0,
  0xf0f0f0f0, // 47
};

using namespace std;
using namespace wunet;

int main(int, char**)
{
  wulogInit(NULL, 0, wulogLoud, "Test", wulog_useLocal);

  try {
    // create with minimum memeory allocated
    mbuf mb;

    // set the header size
    mb.hlen(HLen);
    if (mb.hlen() != HLen) 
      printf("mb.hlen() %d != %d\n", mb.hlen(), HLen);
    
    uint32_t *hptr = (uint32_t *)mb.hpullup();
    for (int i = 0; i < HCnt; i++)
      hptr[i] = Hdr[i];

    // Body
    mb.plen(7);
    if (mb.plen() != 7)
      printf("mb.plen(7) error, plen = %d\n", mb.plen());
    mb.plen(17);
    if (mb.plen() != 17)
      printf("mb.plen(17) error, plen = %d\n", mb.plen());
    mb.plen(30);
    if (mb.plen() != 30)
      printf("mb.plen(30) error, plen = %d\n", mb.plen());

    mb.plen(PLen);
    if (mb.plen() != PLen)
      printf("mb.plen(PLen) error, plen = %d, should = %d\n", mb.plen(), PLen);

    mb.hseek(4);
    if (mb.hlen() != (HLen - 4))
       printf("mb.hseek(4) error,  plen = %d, should = %d\n", mb.hlen(), (HLen - 4));
    if (mb.hoffset() != 4)
      printf("mb.hseek(4) error, hoffset() %d not equal to 4\n", mb.hoffset());

    mb.hrewind();
    if (mb.hlen() != HLen)
      printf("mb.hrewind() error,  plen = %d, should = %d\n", mb.hlen(), HLen);
    if (mb.hoffset() != 0)
      printf("mb.hrewind() error, hoffset() %d not equal to 0\n", mb.hoffset());

    mb.pseek(8);
    if (mb.plen() != (PLen - 8))
      printf("mb.pseek(8) Error, plen() %d not equal to %d\n", mb.plen(), (PLen - 8));

    cout << "prewind()\n";
    mb.prewind();
    cout << "MB: \n\t" << mb << "\n";

    cout << "seek(4)\n";
    mb.seek(4);
    cout << "MB: \n\t" << mb << "\n";

    cout << "seek(130)\n";
    mb.seek(130);
    cout << "MB: \n\t" << mb << "\n";

    cout << "pseek(96)\n";
    mb.pseek(96);
    cout << "MB: \n\t" << mb << "\n";

    cout << "clear()\n";
    mb.clear();
    cout << "MB: \n\t" << mb << "\n";

    cout << "reset()\n";
    mb.reset();
    cout << "MB: \n\t" << mb << "\n";

    cout << "pseek(134)\n";
    mb.pseek(134);
    cout << "prewind()\n";
    mb.prewind();
    cout << "prewind :\n" << mb << "\n";

    // -----
    size_t dlen = PCnt*sizeof(uint32_t);
    cout << "pclear()\n";
    mb.pclear();
    cout << "\nBefore copy in\n\t: " << mb << "\n";

    cout << "pcopyin(Data)\n";
    mb.pcopyin((char *)Data, dlen);
    cout << "\nAfter copy in\n\t: " << mb << "\n";

    uint8_t *dptr = (uint8_t *)Data;
    uint8_t *dbuf = (uint8_t *)malloc(dlen);
    if (dbuf == NULL) {
      cerr << "testBuf: malloc failed!\n";
      return 1;
    }
    memset((void *)dbuf, 0, dlen);
    cout << "pcopyout(Data)\n";
    mb.pcopyout((char *)dbuf, dlen);
    cout << " ... comparing ...\n";
    for (size_t i = 0; i < dlen; i++) {
      if (dptr[i] != dbuf[i])
        cerr << "Original Data[" << i << "] (" << dptr[i] << "] != dbuf[" << i << "] (" << dbuf[i] << ")\n";
    }
    cout << "before ppullup() #1\nmb = \n\t" << mb << "\n";;
    mb.ppullup();
    cout << "After ppullup() :\n" << mb << "\n";

    cout << "pcopyout(Data)\n";
    memset((void *)dbuf, 0, dlen);
    mb.pcopyout((char *)dbuf, dlen);
    cout << " ... comparing ...\n";
    for (size_t i = 0; i < dlen; i++) {
      if (dptr[i] != dbuf[i])
        cerr << "Original Data[" << i << "] (" << dptr[i] << "] != dbuf[" << i << "] (" << dbuf[i] << ")\n";
    }


    {
      mbuf mb2(4, 4);
      cout << "Checking append of buffer, first all\n";
      mb2.clear();
      mb2.append((char *)Data, dlen);
      cout << " ... comparing ...\n";
      memset((void *)dbuf, 0, dlen);
      mb2.pcopyout((char *)dbuf, dlen);
      for (size_t i = 0; i < dlen; i++) {
        if (dptr[i] != dbuf[i])
          cerr << "Original Data[" << i << "] (" << dptr[i] << "] != dbuf[" << i << "] (" << dbuf[i] << ")\n";
      }
      cout << "\nAfter Append 1:\n\t" << mb2 << "\n";
      cout << "Checking append of same buffer, element by element\n";
      mb2.clear();
      for (size_t indx = 0; indx < PCnt; ++indx)
        mb2.append((char *)&Data[indx], sizeof(uint32_t));
      cout << " ... comparing ...\n";
      memset((void *)dbuf, 0, dlen);
      mb2.pcopyout((char *)dbuf, dlen);
      for (size_t i = 0; i < dlen; i++) {
        if (dptr[i] != dbuf[i])
          cerr << "Original Data[" << i << "] (" << dptr[i] << "] != dbuf[" << i << "] (" << dbuf[i] << ")\n";
      }
      cout << "\nAfter Append 2:\n\t" << mb2 << "\n";
    }

    //
    cout << "preset()\n";
    mb.preset();
    cout << "\nAfter preset:\n\t" << mb << "\n";
    // increase size then do pullup and make sure data is copied properly
    cout << "psize(2048)\n";
    mb.psize(2048);
    cout << "plen(1024)\n";
    mb.plen(1024);
    cout << "After size increase:\n\t" << mb << "\n";
    cout << "pseek(256)\n";
    mb.pseek(256);
    cout << "After pseek \n\t" << mb << "\n";

    cout << "ppullup() #2\n";
    mb.ppullup();
    cout << "After pullup:\n\t" << mb << "\n";

    cout << "prewind()\n";
    mb.prewind();

    cout << "pcopyout(Data) dlen = " << dlen << endl;
    mb.pcopyout((char *)dbuf, dlen);
    cout << " ... comparing ...\n";
    for (size_t i = 0; i < dlen; i++) {
      if (dptr[i] != dbuf[i])
        cerr << "Original Data[" << (int)i << "] (" << (int)dptr[i] << "] != dbuf[" << (int)i << "] (" << (int)dbuf[i] << ")\n";
    }

    cout << "\nDone\n\n";

  } catch (wunet::netErr& e) {
    cout << "Exception: \n" << e.toString() << "\n";
  }
  return 0;
}
